# Build to Scale

An FPS puzzle game with a small C++ game engine underneath, for macOS on Apple silicon.
Scale things up and down with your gun: **left click** stretches, **right click** shrinks, the **wheel** picks the
scale factor (x0.25, x0.5, x0.75). Grow a crate to climb a ledge, shrink a wall that blocks the way, and reach the
green zone without falling.

The game is a Unity jam game ([build_to_scale](https://github.com/DavidGiraldoCode/p_gmtk_game_jam_build_to_scale)) rebuilt on
[raylib](https://www.raylib.com/) (window, input, rendering, audio, raygui) and
[Bullet Physics](https://github.com/bulletphysics/bullet3) 3.25. The engine's architecture follows
[p_Game_Engine_Cpp_OpenGL](https://github.com/david-g-netlight/p_Game_Engine_Cpp_OpenGL).
Everything is linked statically: a build needs nothing installed besides the Command Line Tools and CMake.

![running](docs/app_running.gif)

## Controls

| Input | Action |
|---|---|
| **W A S D** / arrows | Walk |
| **Mouse** | Look |
| **Space** | Jump |
| **Left click** | Stretch the prop under the crosshair (scale up) |
| **Right click** | Shrink it (scale down); a prop that gets tiny disappears |
| **Mouse wheel** | Scale factor x0.25 / x0.5 / x0.75 |
| **ESC** | Pause / resume |

The crosshair turns green when the aim is on a prop the gun can scale; a ring shows the 1 second cooldown.
Settings (mouse sensitivity, volumes, fullscreen) are in the main menu and the pause menu and are saved to
`~/Library/Application Support/BuildToScale/settings.cfg`.

## Editor shortcuts (development builds)

Debug builds include a level editor (`ENG_EDITOR=1`); release builds do not.

| Input | Action |
|---|---|
| **TAB** | Enter / leave the editor (the game is frozen while editing; leaving keeps your edits) |
| **RMB + W A S D Q E** | Fly the editor camera (Shift = faster, wheel = fly speed) |
| **LMB** | Select; the gizmo handles drag along an axis |
| **W** / **R** | Gizmo: move / scale |
| **F** | Focus the camera on the selection |
| **Delete**, **Ctrl/Cmd + D** | Delete, duplicate the selection |
| **Ctrl/Cmd + S** | Save the level (`assets/levels/level_0N.lvl`, also copied into the source tree) |
| **F2** / **F3** | Debug grid / Bullet wireframes (while playing) |

The panels: *Hierarchy* (add cube / trigger, delete, duplicate, rename, set the spawn at the camera), *Inspector*
(transform and component fields, **Add Component** from the registry: `Scalable`, `WinTrigger`, `LoseTrigger`,
`NextLevelTrigger`, ...) and the toolbar (Play, Reset to the state at entry, gizmo mode, Physics view, Save, Load).

A level is a text file; see `src/engine/level/LevelSerializer.h` for the format.
The first three levels are drafts meant to be tuned in the editor.

## Build and run

Requirements: macOS 11+ on Apple silicon, Xcode Command Line Tools, CMake 3.20+.

```bash
# Development build (editor on)
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
./build/debug/my_app

# Release build (editor off, optimized)
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j
./build/release/my_app
```

CLion opens the project from `CMakeLists.txt`. The build copies `assets/` next to the executable; the engine loads
assets relative to the executable's folder, so the game can be started from anywhere.

> **Linker error `tapi error: malformed file ... unknown architecture`?** The active Xcode linker is older than the Command Line Tools SDK. Fix it with one of:
> - Use the Command Line Tools toolchain: `sudo xcode-select -s /Library/Developer/CommandLineTools`
> - Update Xcode so its linker matches the SDK
> - Or build once against Xcode's own SDK: `-DCMAKE_OSX_SYSROOT=$(DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcrun --sdk macosx --show-sdk-path)`

### Package for another Mac

```bash
tools/package_release.sh
```

builds the release and writes `dist/BuildToScale/` (the executable and `assets/`) and
`dist/BuildToScale-macos.zip`. The executable runs on macOS 11 or newer (Apple silicon) and depends on system
frameworks only. It is not signed or notarized: on a Mac that downloaded the zip, right-click the executable and
choose *Open* the first time, or run `xattr -dr com.apple.quarantine BuildToScale`.

## Project layout

```
├── CMakeLists.txt        # engine static library + the game executable (my_app)
├── src/
│   ├── main.cpp          # entry point: Engine::SetApplication(new ScaleGame)
│   ├── engine/           # the engine (namespace eng): core, input, render, audio, physics, fx,
│   │                     #   level, modes, ui, editor, resources. Public header: eng.h
│   └── game/             # the game: ScaleGame (Application layer) and state/, player/, props/,
│                         #   fx/, flow/, ui/, audio/
├── assets/               # audio/, levels/, shaders/ (copied next to the executable)
├── include/              # third-party headers: raylib/ (raylib, raymath, rlgl, raygui), bullet/
├── lib/                  # prebuilt static libraries: raylib, Bullet (arm64, macOS 11)
├── tools/                # build_bullet.sh, generate_placeholder_audio.py, package_release.sh
└── docs/                 # ARCHITECTURE_PLAN.md, DEV_PROGRESS.md, devlogs/, tasks/
```

The architecture, the class lists and the loop diagrams are in [`docs/ARCHITECTURE_PLAN.md`](docs/ARCHITECTURE_PLAN.md);
what was built, phase by phase, is in [`docs/DEV_PROGRESS.md`](docs/DEV_PROGRESS.md) and `docs/devlogs/`.

## Third-party code

| Library | Version | Use |
|---|---|---|
| [raylib](https://github.com/raysan5/raylib) | 5.6-dev | window, input, rendering, audio (zlib license) |
| [raygui](https://github.com/raysan5/raygui) | 5.1-dev | menus and editor panels (zlib license) |
| [Bullet Physics](https://github.com/bulletphysics/bullet3) | 3.25 | rigid bodies, raycasts, triggers (zlib license; `include/bullet/LICENSE.txt`) |

The sound effects and music in `assets/audio/` are placeholders generated by `tools/generate_placeholder_audio.py`;
see `assets/audio/CREDITS.txt` for the tracks of the original game.
