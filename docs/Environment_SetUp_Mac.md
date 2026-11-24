# Environment Setup (macOS, Apple silicon)

Step-by-step setup for a **new Mac** with no developer tools installed. When you finish, you can clone the
repository, build it and run the game.

Run every command in **Terminal** (`⌘ Space`, type *Terminal*, press Enter). Copy one block at a time.

## What you need

| Tool | Why | Installed in |
|---|---|---|
| macOS 11 or newer, Apple silicon (M1/M2/M3/M4) | The bundled libraries are `arm64` for macOS 11.0 | — |
| Xcode Command Line Tools | Apple `clang++` (C++20), the linker, `make`, `git`, the macOS SDK and frameworks (Cocoa, OpenGL, IOKit, CoreVideo, GLUT) | Step 1 |
| Homebrew | Installs CMake and the editors | Step 2 |
| CMake 3.21 or newer | Configures the build (`CMakePresets.json` needs 3.21) | Step 3 |
| Visual Studio Code + extensions | Editor, build and debugger | Step 5 |
| CLion (optional) | Alternative IDE | Step 7 |

**You do not install** raylib or Bullet Physics. Both are committed as static libraries (`lib/*.a`) with their
headers (`include/`), so a clone builds as is. The full Xcode app is **not** needed.

---

## Step 1: Xcode Command Line Tools

```bash
xcode-select --install
```

A dialog opens. Click **Install** and accept the license. It takes a few minutes.

**Validate:**

```bash
xcode-select -p                 # expected: /Library/Developer/CommandLineTools
/usr/bin/clang++ --version      # expected: Apple clang version 15 or newer
git --version                   # expected: git version 2.x
make --version | head -1        # expected: GNU Make 3.81
xcrun --show-sdk-path           # expected: a path ending in MacOSX.sdk
```

> If `xcode-select --install` says *already installed*, the tools are there; run the checks above.

## Step 2: Homebrew

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

The installer asks for your Mac password. When it finishes, add Homebrew to your shell (Apple silicon installs to
`/opt/homebrew`):

```bash
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
```

**Validate:**

```bash
brew --version                  # expected: Homebrew 4.x or newer
brew doctor                     # expected: "Your system is ready to brew."
```

## Step 3: CMake

```bash
brew install cmake
```

**Validate:**

```bash
cmake --version                 # expected: cmake version 3.21 or newer
```

## Step 4: Clone, build and run from the Terminal

This proves the toolchain works before any editor is involved.

```bash
mkdir -p ~/dev && cd ~/dev
git clone https://github.com/DavidGiraldoCode/b-RayLib_static_starter_project_MacOS_M2.git
cd b-RayLib_static_starter_project_MacOS_M2
```

Debug build (level editor and developer console on):

```bash
cmake --preset debug
cmake --build --preset debug
./build/debug/my_app
```

Release build (optimized, editor and console off):

```bash
cmake --preset release
cmake --build --preset release
./build/release/my_app
```

**Validate:**

```bash
file build/debug/my_app         # expected: Mach-O 64-bit executable arm64
ls build/debug/assets           # expected: the assets folder was copied next to the executable
otool -L build/debug/my_app     # expected: only /System/Library/... and /usr/lib/... entries
```

When the game starts, a window opens and the log's first engine line prints the build flags, for example
`Raylib Engine 0.0.1 | ... | editor ON | console ON | audio ON`. Close the window to quit.

## Step 5: Visual Studio Code

Install the editor and its `code` command:

```bash
brew install --cask visual-studio-code
```

Install the extensions (C/C++ IntelliSense, CMake integration, LLDB debugger):

```bash
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension vadimcn.vscode-lldb
```

> If `code: command not found`, open VS Code once, press `⌘⇧P`, run **Shell Command: Install 'code' command in
> PATH**, then open a new Terminal and run the three commands again.

**Validate:**

```bash
code --version
code --list-extensions | grep -E "cpptools|cmake-tools|vscode-lldb"
# expected: ms-vscode.cmake-tools, ms-vscode.cpptools, vadimcn.vscode-lldb
```

### Configure the workspace

`.vscode/` is git-ignored, so each developer creates it once. From the repository root:

```bash
mkdir -p .vscode

cat > .vscode/settings.json <<'EOF'
{
  "cmake.useCMakePresets": "always",
  "cmake.cmakePath": "/opt/homebrew/bin/cmake",
  "cmake.configureOnOpen": true,
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "C_Cpp.default.cppStandard": "c++20"
}
EOF

cat > .vscode/launch.json <<'EOF'
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug my_app (LLDB)",
      "type": "lldb",
      "request": "launch",
      "program": "${command:cmake.launchTargetPath}",
      "cwd": "${workspaceFolder}"
    }
  ]
}
EOF

code .
```

### Build, run and debug in VS Code

`cmake.cmakePath` points CMake Tools at Homebrew's CMake. Without it, a VS Code started from the Dock or Finder
does not have `/opt/homebrew/bin` on its `PATH`, cannot find `cmake`, and every build fails with
*Unable to configure the project*.

1. When VS Code asks *Do you trust the authors?*, click **Yes**.
2. **Select a configure preset before the first build:** `⌘⇧P` › **CMake: Select Configure Preset** › **Debug**
   (or **Release**). Then `⌘⇧P` › **CMake: Configure**. The **Output** panel (channel **CMake/Build**) ends with
   `Build files have been written to: .../build/debug`.
3. Build: `⌘⇧P` › **CMake: Build**, or press **F7**. The output goes to `build/debug/`.
4. Run without the debugger: `⌘⇧P` › **CMake: Run Without Debugging** (`⇧F5`).
5. Debug: open the **Run and Debug** panel, choose **Debug my_app (LLDB)** and press **F5**. Breakpoints in
   `src/` stop the game.

> CodeLLDB may ask to download its platform package the first time; accept it.

**Validate:** set a breakpoint on the first line of `main()` in `src/main.cpp`, press **F5**, and check that
execution stops there.

## Step 6: Done

You now build and run with either:

```bash
cmake --build --preset debug && ./build/debug/my_app
```

or **F7** / **F5** in VS Code. Build flags, the release package and the game controls are in the
[README](../README.md).

---

## Step 7 (optional): CLion

```bash
brew install --cask clion
```

CLion ships its own CMake and uses the Command Line Tools compiler from Step 1, so Step 3 is not required for
CLion alone (it is still needed for the Terminal builds).

**Validate:**

```bash
ls -d /Applications/CLion.app   # expected: /Applications/CLion.app
```

### Open and build the project

1. Start CLion and sign in or start a trial / the free non-commercial license.
2. **File › Open…** and select the repository folder (the one that contains `CMakeLists.txt`). Choose
   **Open as Project** and **Trust Project**.
3. CLion reads `CMakePresets.json` and lists the **debug** and **release** presets as CMake profiles
   (Settings › Build, Execution, Deployment › **CMake**). Enable the ones you want; their build folders are
   `build/debug` and `build/release`, the same as the Terminal builds.
   - CLion may also add its own **Debug** profile that builds into `cmake-build-debug/`. That is fine (it is
     git-ignored); you can disable it to avoid building twice.
4. Check the toolchain: Settings › Build, Execution, Deployment › **Toolchains** › *Default* should detect
   `/usr/bin/clang`, `/usr/bin/clang++` and `/usr/bin/make`, and show the debugger **Bundled LLDB**.
5. In the run-configuration selector (top right) pick **my_app** with the **debug** profile.
6. **Build** with `⌘F9`, **Run** with `⌃R`, **Debug** with `⌃D`.

**Validate:** set a breakpoint in `src/main.cpp`, press `⌃D`, and check that execution stops there.

Extra build flags such as `-DENG_DEV_CONSOLE_IN_RELEASE=ON` go in the profile's **CMake options** field; see
*Build flags* in the [README](../README.md).

---

## Troubleshooting

**`tapi error: malformed file ... unknown architecture` at link time.**
The active Xcode linker is older than the Command Line Tools SDK. This happens when the full Xcode app is
installed and selected. Switch back to the Command Line Tools:

```bash
sudo xcode-select -s /Library/Developer/CommandLineTools
xcode-select -p                 # expected: /Library/Developer/CommandLineTools
```

Then delete the build folder (`rm -rf build/debug`) and build again. The README lists the other fixes.

**VS Code: `Build failed: Unable to configure the project`.**
CMake Tools could not run the configure step. Check, in order:
1. `.vscode/settings.json` has `"cmake.cmakePath": "/opt/homebrew/bin/cmake"` (Step 5), and `ls /opt/homebrew/bin/cmake` finds it.
2. A configure preset is selected: `⌘⇧P` › **CMake: Select Configure Preset** › **Debug**.
3. Reset the extension: `⌘⇧P` › **CMake: Delete Cache and Reconfigure**, then `⌘⇧P` › **Developer: Reload Window**.
4. If it still fails, read the real error in **View › Output** › channel **CMake/Build**.

**`CMake Error: ... CMakePresets.json ... version`** or `Unknown preset`.
CMake is older than 3.21. Run `brew upgrade cmake`.

**The compiler is not Apple clang.**
If you installed `llvm` from Homebrew and put it on your `PATH`, CMake may pick that compiler. Check with
`grep CMAKE_CXX_COMPILER: build/debug/CMakeCache.txt`. To force Apple clang, delete the build folder and run
`CXX=/usr/bin/clang++ cmake --preset debug`.

**`xcrun: error: invalid active developer path`.**
The Command Line Tools are missing, usually after a macOS update. Run `xcode-select --install` again.

**The game window opens but the mouse or keyboard does nothing.**
Click inside the window once to give it focus.

## Optional tools

These are only for maintainers; a normal build never needs them.

| Tool | Used by | Install |
|---|---|---|
| `tools/package_release.sh` | Makes a release zip | Nothing extra (uses `zip` and `otool` from macOS / Step 1) |
| `tools/check_release_console.sh` | Checks a release binary has no console strings | Nothing extra |
| `tools/build_bullet.sh` | Rebuilds the committed Bullet libraries | Nothing extra (needs network access to GitHub) |
| `tools/generate_placeholder_audio.py` | Writes placeholder WAV files | `python3` (the Command Line Tools include it: `python3 --version`) |
