# Development Progress

Tracks the implementation of [`ARCHITECTURE_PLAN.md`](ARCHITECTURE_PLAN.md). Phase numbers and "Done when" criteria match §5 of the plan.

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch).
- Add a line to the **Session log** at the end of every work session. Add a **Decision** whenever the plan changes.
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Status | Started | Finished | Notes |
|---|---|---|---|---|---|
| 0 | Project setup, CMake, raygui, Bullet static libs | 🟡 | 2026-11-06 | | |
| 1 | Core: Engine, Application, Time, Window | ⬜ | | | |
| 2 | Input | ⬜ | | | |
| 3 | Resources and rendering | ⬜ | | | |
| 4 | Scene: GameActor, Component, Level | ⬜ | | | |
| 5 | Physics (Bullet 3.25) | ⬜ | | | |
| 6 | Audio | ⬜ | | | |
| 7 | FX: particles and lines | ⬜ | | | |
| 8 | Events, modes, menus | ⬜ | | | |
| 9 | Editor mode | ⬜ | | | |
| 10 | Game: state and player | ⬜ | | | |
| 11 | Game: scaling mechanic | ⬜ | | | |
| 12 | Game: level flow, UI, music | ⬜ | | | |
| 13 | Polish and release | ⬜ | | | |

**Current focus:** Phase 0
**Branch:** `project_setup_and_build_system` (from `integration`)

---

## Phase checklists

### Phase 0 — Project setup and build system 🟡
- [x] Root `CMakeLists.txt`: `engine` static lib + `my_app` exe, C++20
- [x] Remove the `Makefile` for good; update `README.md` build instructions
- [x] Vendor `raygui.h` into `include/raylib/` (raygui 5.1-dev, upstream `1abccb3a53`)
- [x] `tools/build_bullet.sh` (bullet3 tag **3.25**, arm64, static, single precision, demos/extras off)
- [x] Commit `lib/libBulletDynamics.a`, `lib/libBulletCollision.a`, `lib/libLinearMath.a` + `include/bullet/` + Bullet `LICENSE.txt`
- [x] Bullet include path PRIVATE to `engine` (+ static libs linked)
- [x] `assets/` folder + post‑build copy
- [x] `ENG_EDITOR` compile definition (Debug = 1, Release = 0, `-DENG_EDITOR_IN_RELEASE=ON` to force 1) + `eng::IsEditorBuild()`
- [ ] **Done when:** the old free‑camera `main.cpp` runs from CMake, and a throw‑away `btDiscreteDynamicsWorld` links

### Phase 1 — Core ⬜
- [ ] `eng::Application` (Init / Update / FixedUpdate / DrawHUD / Destroy + mode hooks)
- [ ] `eng::Engine` singleton, `SetApplication` ownership transfer, `Init / Run / Shutdown`
- [ ] Window flags (MSAA, VSync, resizable, HighDPI), `SetExitKey(KEY_NULL)`, `ChangeDirectory(GetApplicationDirectory())`
- [ ] `Time` (clamped dt, fixed‑step accumulator), `WindowInfo`, `World`, `Log`
- [ ] `main.cpp` reduced to the reference pattern; temporary `ScaleGame` draws a cube
- [ ] **Done when:** window opens via `Engine::Run`, cube renders, clean shutdown

### Phase 2 — Input ⬜
- [ ] `InputManager::Poll()` (down / pressed / released, mouse delta, wheel)
- [ ] `InputMap` actions and axes
- [ ] Cursor lock / unlock
- [ ] **Done when:** debug overlay shows action states; mouse look works with the cursor locked

### Phase 3 — Resources and rendering ⬜
- [ ] `ResourceManager` (cache + `UnloadAll`, primitives)
- [ ] `Camera`, `ActiveCamera`
- [ ] `Material` (tint, shader, params)
- [ ] `RenderCommand`, `RenderQueue`
- [ ] `RenderTarget`, `DebugDraw`
- [ ] Optional lit shader
- [ ] **Done when:** 10 cubes with different materials render through the `RenderQueue`

### Phase 4 — Scene ⬜
- [ ] `GameActor` (transform, hierarchy, world matrix, `MarkForDestroy`)
- [ ] `Component` base + `AddComponent<T> / GetComponent<T>`
- [ ] `Level` (factory, update, fixed update, submit, `FlushDestroyed`)
- [ ] `MeshRenderer`
- [ ] **Done when:** children follow a rotating parent; destroying an actor mid‑update is safe

### Phase 5 — Physics (Bullet 3.25) ⬜
- [ ] `BulletConvert` (Vector3 / Quaternion / Transform ↔ bt types)
- [ ] `PhysicsWorld` pimpl: init, `Step`, `Shutdown` (correct delete order)
- [ ] `PhysicsLayers` groups and masks
- [ ] `Collider` (box, sphere, capsule, convex hull, static mesh, compound bottom pivot, `SetScale`)
- [ ] `RigidBody` (static / dynamic / kinematic, `Teleport`, `OnScaleChanged`, impulses)
- [ ] `ActorMotionState`
- [ ] `Raycast` (`ClosestRayResultCallback`, mask filter, user pointer → actor)
- [ ] `TriggerVolume` (ghost object, `btGhostPairCallback`, enter/exit diff)
- [ ] `CharacterMotor` — dynamic capsule: no rotation, own gravity, walk velocity on the ground plane, sphere‑sweep grounding, 45° slope limit, jump + grace time, warp, ground‑body velocity
- [ ] `PhysicsDebugDrawer` + F3 toggle
- [ ] CCD for small dynamic bodies
- [ ] **Done when:** sandbox (plane + 20 crates + ramp) at 60 FPS; crates stack and sleep; capsule climbs a 30° ramp without sliding, stops at 50°, jumps on a crate, pushes a light crate; center ray reports the hit crate; trigger Enter/Exit fire once each; F3 wireframes; deleting a crate mid‑sim is safe

### Phase 6 — Audio ⬜
- [ ] `AudioManager` (alias voice pool, volumes, duck)
- [ ] `AudioSource`
- [ ] `MusicPlayer` (stream, loop, fade)
- [ ] **Done when:** pitched SFX on key press, music fades in, pause ducks music

### Phase 7 — FX ⬜
- [ ] `ParticleEmitter` (burst, gradient, alpha fade) + pooled `ParticleSystem`
- [ ] `LineRenderer` (thick gradient line)
- [ ] **Done when:** click spawns a colored burst and a fading beam

### Phase 8 — Events, modes, menus ⬜
- [ ] `EventBus` (queued dispatch)
- [ ] `ModeMachine` (MainMenu / Play / Paused / Editor; cursor, audio duck, hooks)
- [ ] `UIStack`, `UIScreen`, `UIStyle`, `RayGuiImpl.cpp`
- [ ] `MainMenuScreen`, `PauseMenuScreen`, `MessageScreen`
- [ ] Auto‑pause on focus loss
- [ ] Physics not stepped while Paused / Menu / Editor
- [ ] **Done when:** Menu → Start → Play → ESC pause (frozen, crates stay mid‑air) → Resume exactly where it was → Menu → Quit

### Phase 9 — Editor mode ⬜
- [ ] TAB enter/exit, `LevelSnapshot` capture
- [ ] `EditorCamera`
- [ ] `Picker` (physics raycast first, render bounds fallback)
- [ ] `TransformGizmo` (move / scale) → `RigidBody::Teleport`, `Collider::SetScale`
- [ ] `HierarchyPanel`, `InspectorPanel` (incl. `RigidBody` / `Collider` fields), `EditorToolbar`
- [ ] `LevelSerializer` + `ComponentRegistry` (`.lvl` format)
- [ ] Bullet debug wireframes in the editor overlay
- [ ] **Done when:** edit a level mid‑game, save, exit with edits live; reload reproduces it; Reset restores positions **and** velocities

### Phase 10 — Game: state and player ⬜
- [ ] `PlayerState`, `GameState` (+ events), `SoundFXBank`, `LevelList`
- [ ] `PlayerInput`
- [ ] `PlayerActor`, `FirstPersonController` (capsule gravity 32, jump 8), `FirstPersonLook`
- [ ] Spawn from `.lvl`
- [ ] **Done when:** walk, jump, look in `level_01`; wheel cycles x0.25 / x0.5 / x0.75

### Phase 11 — Game: scaling mechanic ⬜
- [ ] `ScalingGun` (cooldown, center raycast with `Ground | Prop` mask)
- [ ] `Scalable` (SmoothStep, 1.9 cap, min‑size destroy, `setLocalScaling`, inertia update, pitched SFX)
- [ ] `RayBeamSpawner`, `RayBeam`, `MuzzleFX`
- [ ] Stretch blocked when the target size overlaps a static wall
- [ ] **Done when:** LMB grows smoothly (capped), RMB shrinks until it disappears, correct colors, cooldown works, scaled crates still rest on the floor and react to gravity

### Phase 12 — Game: level flow, UI, music ⬜
- [ ] `TriggerZone`, `WinTrigger`, `LoseTrigger`, `NextLevelTrigger`
- [ ] `GameplayManager` (load / next / reload)
- [ ] `GameUI` (win / lose screens)
- [ ] `MusicManager` + `assets/audio/CREDITS.txt`
- [ ] Levels 1–3 authored in the editor
- [ ] **Done when:** Menu → L1 win → L2 fall → retry → win → Menu

### Phase 13 — Polish and release ⬜
- [ ] `GunView` (separate pass), `GunHUD` (factor text, crosshair, cooldown)
- [ ] Settings (sensitivity, volumes, fullscreen)
- [ ] Release build `ENG_EDITOR=0`, test on a clean Mac
- [ ] README: controls, build, editor shortcuts
- [ ] **Done when:** release binary + `assets/` runs on a machine with no dev tools

---

## Decision log

| # | Date | Decision | Reason | Plan section |
|---|---|---|---|---|
| D1 | 2026-11-06 | raylib replaces GLFW/GLEW/GraphicsAPI/ImGui; raygui for UI | Task requirement; one ecosystem | §0, §2 |
| D2 | 2026-11-06 | `GameActor` + light `Component`s | Unity scripts map 1:1 to components | §0, Phase 4 |
| D3 | 2026-11-06 | Engine `ModeMachine` (Menu/Play/Paused/Editor); game `GameState` (win/lose) | Engine modes are generic; win/lose are game rules | §3.3 |
| D4 | 2026-11-06 | **Physics: Bullet 3.25** instead of a custom AABB layer | Review feedback; real rigid bodies, rotated shapes, runtime scaling | §0, Phase 5 |
| D5 | 2026-11-06 | Bullet shipped as prebuilt static libs in `lib/` + headers in `include/bullet/` (not a submodule) | Keeps the repo's "clone and run" setup | Phase 0 |
| D6 | 2026-11-06 | Bullet hidden behind `eng::PhysicsWorld` pimpl; game never includes Bullet | Backend swappable, faster game builds | Phase 5 |
| D7 | 2026-11-06 | ~~Player uses `btKinematicCharacterController`; fallback = dynamic capsule~~ **Superseded by D8** | Closest match to Unity `CharacterController` | Phase 5, §10 |
| D8 | 2026-11-06 | **Player uses a dynamic capsule `btRigidBody`** (no rotation, own gravity, sweep grounding); fallback = `btKinematicCharacterController` | Review feedback; avoids the kinematic controller's slope jitter; pushes crates and rides stretched props for free | Phase 5, Phase 10, §10 |

---

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| Dynamic capsule: slope sliding / bouncing / wall sticking | 5 / 10 | open | Ground‑plane projection, friction 0, grounded glue; see plan §10 |
| Player launched by a crate scaling under them | 11 | open | Clamp player vertical velocity after the step |
| Scaling causes deep penetration → velocity spikes | 11 | open | Clamp velocity, overlap check before stretch |
| Bullet precision / arch mismatch | 0 | mitigated | Libs verified arm64, macOS 11.0, single precision |
| Xcode 26 linker can't read the Command Line Tools SDK 27.2 (`tapi error: malformed file`) | 0 | workaround | Local toolchain issue. Configure with Xcode's SDK as `CMAKE_OSX_SYSROOT` (see README) or update Xcode |

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-06 | 0 | Branch `integration` + `project_setup_and_build_system`; all 8 Phase 0 tasks committed; Bullet link probe passed (box falls 10 → 5.19 m in 1 s); game target confirmed unable to include Bullet | Developer runs the Phase 0 **Done when** check | Waiting for green light to merge |
| 2026-11-06 | — | Player motor switched to a dynamic capsule (D8) | Start Phase 0 | — |
| 2026-11-06 | — | Architecture plan written; physics layer switched to Bullet 3.25; progress tracker created | Start Phase 0 | — |
