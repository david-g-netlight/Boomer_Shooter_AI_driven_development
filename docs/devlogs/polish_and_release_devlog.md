# Devlog — `polish_and_release`

**Phase:** 13 — Polish and release ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `a85c027`
**Date:** 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/polish_and_release`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `76b2e40` | Cleanup (developer-approved for this phase) | Vendored raylib sources, `external/` and build files removed from `include/raylib/` (19 MB → 2.7 MB), duplicate `libraylib.a`, stale `build/`, root `my_app`, unused `assets/fonts`, `assets/models` and `.gitkeep` files. |
| 2 | `1951d7d` | `GunHUD` | Crosshair (green on a `Scalable` prop in reach, dimmed with a cooldown ring), factor text with pips, hints. Also removes the Phase 3–6 sandbox from `ScaleGame` and the `Rotator`, `SelfDestructTimer`, `TriggerCounter` components. |
| 3 | `c24d2f2` | `GunView` | The held gun in its own pass through the new `Application::DrawViewModel()` hook: own camera, depth off, kick, bob, factor light. |
| 4 | `b7f3f20` | Settings | `Settings` (sensitivity, master / music / effects volume, fullscreen) saved in `~/Library/Application Support/BuildToScale/`, `SettingsScreen` from both menus through `Application::HasSettings / OnSettingsRequested`; ESC belongs to a dialog on top of the mode screen. |
| 5 | `2001727` | Release build | Deployment target 11.0 / arm64, Release by default, `tools/package_release.sh` (executable + assets + zip), editor-only source-tree save, obsolete `sandbox.lvl` removed. |
| 6 | `519bb90` | README | Rewritten for the game; `assets/README.md`; the plan's as-built notes extended. |
| 7 | `dd826f9` | — | `.gitignore` for CLion build directories. |
| 8 | *(this commit)* | — | This devlog; tracker marks Phase 13 done. |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P13-1 | Held items go through `Application::DrawViewModel()` with their own camera and no depth test | The gun must not clip into walls and must ignore the world camera's pitch and yaw |
| P13-2 | Settings live in the user's application-support folder | The game folder may be read-only or replaced by an update |
| P13-3 | The menus ask the game for settings (`HasSettings`) instead of the engine owning a settings screen | Which options exist is game content |
| P13-4 | The deployment target is set in CMake, not left to the SDK | The first package required the build machine's macOS (27.2) |
| P13-5 | Raylib sources were removed from the repo | Only four headers and the prebuilt library are used; the sources were 16 MB of dead weight |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug and Release), no warnings | ✅ |
| HUD, gun and settings screens (screenshots); settings round trip, clamping, audio apply (headless) | ✅ |
| Release package: system frameworks only, minos 11.0, arm64, no source path in the binary | ✅ |
| Release from another folder with an empty environment and a fake HOME: editor off, level loads, TAB refused | ✅ |
| Scripted playthroughs win levels 1–3 with the optimized build | ✅ |
| Release binary + `assets/` runs on a machine with no dev tools | ✅ validated by the developer |
