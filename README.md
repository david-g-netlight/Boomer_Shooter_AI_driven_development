# Raylib C++ Example Project

This is a simple C++ project using <a href="https://www.raylib.com/index.html" target="_blank">**Raylib**</a> for graphics, window and OS events. It uses the basic <a href="https://www.raylib.com/examples/core/loader.html?name=core_3d_camera_free" target="_blank">example of a 3D free camera</a> . The project uses a **statically linked Raylib library**, making it portable across macOS machines without requiring system-wide Raylib installation. Instructions on how to run it and changes on the `.gitignore` are bellow.


![alt text](docs/app_running.gif)


## 📂 Project Directory Structure

```
my_project/
├── docs/                # Documentation (architecture plan, progress tracker)
├── include/             # Third-party header files
│   └── raylib/          # Raylib headers
├── lib/                 # Static libraries
│   └── libraylib.a
├── src/                 # Source files
│   ├── main.cpp         # Entry point
│   ├── engine/          # Engine static library (namespace eng)
│   └── game/            # Game (Application layer)
├── build/               # CMake build output (auto-generated, ignored)
├── CMakeLists.txt       # Build instructions
└── README.md            # This file
```


## 🚀 How to Run

The project builds with **CMake** (3.20+). It produces an `engine` static library and the `my_app` executable.

1. Configure and build (Release):

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
```

2. Run the app:

```bash
./build/release/my_app
```

3. To build a debug version with symbols for debugging:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
./build/debug/my_app
```

4. To clean all compiled objects and executables:

```bash
rm -rf build
```

CLion opens the project directly from `CMakeLists.txt`.

> **Linker error `tapi error: malformed file ... unknown architecture`?** The active Xcode linker is older than the Command Line Tools SDK. Fix it with one of:
> - Use the Command Line Tools toolchain (no download): `sudo xcode-select -s /Library/Developer/CommandLineTools`
> - Update Xcode from the App Store so its linker matches the SDK
> - Or point CMake at Xcode's own SDK for one build: `-DCMAKE_OSX_SYSROOT=$(DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcrun --sdk macosx --show-sdk-path)`

## 💻 OS Requirements

* macOS 10.9 or newer
* Xcode Command Line Tools installed
* CMake 3.20 or newer
* Clang / Apple Clang compiler (C++20 compatible)

## ⚠️ Notes on .gitignore

The project .gitignore is configured to ignore build artifacts and temporary files, which is common in C++ projects. The repo includes the static library (lib/libraylib.a) and headers (include/raylib/), so it can be cloned and run immediately.

If you wish to exclude or include additional files for your own fork or contribution, feel free to modify the .gitignore to match your needs.

## 📚 References

* [Raylib Official GitHub](https://github.com/raysan5/raylib)
* [Raylib macOS Build Instructions](https://github.com/raysan5/raylib/wiki/Working-on-macOS)