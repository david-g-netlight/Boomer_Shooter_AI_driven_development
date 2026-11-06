# Devlog — `project_setup_and_build_system`

**Phase:** 0 — Project setup and build system ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `f84ac43`
**Date:** 2026-11-06
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/project_setup_and_build_system`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `6ce8fab` | Root `CMakeLists.txt` | Two targets: `engine` (static lib, globs `src/engine/**/*.cpp`) and `my_app` (`src/main.cpp` + `src/game/**/*.cpp`). C++20. raylib and the macOS frameworks are linked `PUBLIC` on `engine`, so the game gets them through `engine`. Public include paths: `src/engine`, `include`, `include/raylib`. Added `src/engine/eng.h` (umbrella header) and `core/EngineInfo.h/.cpp` (name and version), because a static library needs at least one source file. |
| 2 | `2984470` | Remove `Makefile`, update README | `Makefile` deleted. README build/run/debug/clean steps rewritten for CMake (`build/release`, `build/debug`), directory tree updated (`src/engine`, `src/game`), requirements now CMake 3.20+ and C++20. |
| 3 | `8b41e37` | Vendor `raygui.h` | `include/raylib/raygui.h`, raygui **5.1-dev**, upstream `raysan5/raygui@1abccb3a53` (2026-10-20). Checked it compiles and links as C++20 against raylib 5.6-dev. Its only warning is `-Wunused-function` (`GuiFontIconBaking`) inside the implementation, to silence in Phase 8's `RayGuiImpl.cpp`. |
| 4 | `1b5ed15` | `tools/build_bullet.sh` | Clones `bulletphysics/bullet3` tag `3.25` (shallow), builds only `BulletDynamics`, `BulletCollision`, `LinearMath` as static Release libs, then copies the libs to `lib/` and the headers of those three modules to `include/bullet/`. Flags: arm64, `CMAKE_OSX_DEPLOYMENT_TARGET=11.0`, `USE_DOUBLE_PRECISION=OFF`, all demos/extras/pybullet/enet/clsocket off. Env overrides: `BULLET_TAG`, `BULLET_WORK_DIR`, `BULLET_OSX_SYSROOT`. |
| 5 | `b36a8c3` | Commit Bullet libs + headers | `lib/libBulletDynamics.a` (0.9 MB), `lib/libBulletCollision.a` (1.2 MB), `lib/libLinearMath.a` (0.2 MB); `include/bullet/` (234 headers, 1.9 MB) + `LICENSE.txt` (zlib) + `BULLET_VERSION.txt`. Verified: arm64, minos 11.0, no `BT_USE_DOUBLE_PRECISION`. |
| 6 | `04605a2` | Bullet include path PRIVATE | `target_include_directories(engine PRIVATE include/bullet)` + Bullet libs linked `PRIVATE`. CMake still forwards the static libs to the `my_app` link line. |
| 7 | `74b0891` | `assets/` + post‑build copy | `assets/{levels,models,shaders,audio,fonts}/` (each with `.gitkeep`) + `assets/README.md`. A `POST_BUILD` step copies `assets/` to `$<TARGET_FILE_DIR:my_app>/assets`. |
| 8 | `e396ed9` | `ENG_EDITOR` definition | Always defined as `0` or `1` (`#if ENG_EDITOR`), `PUBLIC` on `engine` so the game sees the same value. Debug = 1, other configs = 0, `-DENG_EDITOR_IN_RELEASE=ON` forces 1. Added `eng::IsEditorBuild()`. |
| 9 | *(this commit)* | — | This devlog; README linker‑error tip lists the three fixes; tracker marks the toolchain issue resolved and Phase 0 done. |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P0‑1 | Base branch is `ai-generated-game-engine` (TASK_02 says `ai-generated-engine`, which doesn't exist; the developer had renamed the branch) | Validated with the developer |
| P0‑2 | `TASK_02.md` committed on the base branch before creating `integration` (`f84ac43`) | The workflow file belongs to the shared base |
| P0‑3 | The `Makefile` deletion goes into the "Remove the Makefile" checkbox commit, not into the earlier "new files" commit | One concern per commit |
| P0‑4 | Engine stub `core/EngineInfo` added in commit 1 | `add_library(STATIC)` fails with no sources. It is real API (window title, logs), not a throw‑away |
| P0‑5 | Engine and game headers live next to their `.cpp` files under `src/`; `include/` stays for third‑party headers only | Keeps the existing repo arrangement and mirrors the reference `engine/source` |
| P0‑6 | raygui taken from upstream `master` (5.1-dev), not a release tag | raylib here is 5.6-dev; the latest raygui release targets older raylib |
| P0‑7 | Bullet deployment target pinned to macOS **11.0** | Same `minos` as `lib/libraylib.a`, which avoids "built for newer macOS" linker warnings |
| P0‑8 | Only `BulletDynamics`, `BulletCollision`, `LinearMath` built and shipped | Everything the plan uses (rigid bodies, ghosts, ray tests, character fallback); avoids Bullet3/OpenCL/robotics code |
| P0‑9 | Bullet's version file is named `BULLET_VERSION.txt`, **not** `VERSION` | On case‑insensitive APFS, `include/bullet/VERSION` shadowed libc++'s `<version>` header and broke every compile. Commits 4 and 5 were rewritten (local, unmerged) to keep one commit per checkbox |
| P0‑10 | `ENG_EDITOR` always defined (0/1) instead of defined/undefined | `#if ENG_EDITOR` works everywhere and the compiler warns on typos with `-Wundef` |
| P0‑11 | Toolchain: the developer switched to the Command Line Tools (`xcode-select -s /Library/Developer/CommandLineTools`, ld‑27106) | Xcode 26.3's linker (ld‑1230) can't read the CLT SDK 27.2 `.tbd` files (`tapi error: malformed file ... unknown architecture`). Nothing in the repo hard‑codes an SDK; the README lists the fixes |

---

## Verification

| Check | Result |
|---|---|
| Debug + Release builds, plain `cmake`, no SDK override (CLT toolchain) | ✅ builds; only warning is `-Wmissing-braces` in the original example `main.cpp` |
| Old free‑camera `main.cpp` runs | ✅ validated by the developer |
| Bullet link probe (temporary `.cpp` in `src/engine`, not committed): box dropped from y=10 for 50 × 0.02 s | ✅ `y = 5.193` (analytic 5.095; semi‑implicit Euler lands slightly higher). Passed with both the Xcode SDK and the CLT toolchain |
| Game target can't include Bullet (`src/game/*.cpp` with `#include <btBulletDynamicsCommon.h>`) | ✅ `file not found`, as intended |
| `ENG_EDITOR` value per config (engine and game `flags.make`) | ✅ Debug=1, Release=0, Release + `ENG_EDITOR_IN_RELEASE=ON`=1 |
| `assets/` copied next to the binary | ✅ |

## Notes for later phases
- Phase 1 must call `ChangeDirectory(GetApplicationDirectory())` so `assets/` resolves when the app is launched from another folder.
- Phase 8: put `RAYGUI_IMPLEMENTATION` in `src/engine/ui/RayGuiImpl.cpp` and silence `-Wunused-function` there.
- CLion: reset the CMake cache once (`Tools → CMake → Reset Cache and Reload Project`), because `cmake-build-debug/` was configured with the old Xcode toolchain.
