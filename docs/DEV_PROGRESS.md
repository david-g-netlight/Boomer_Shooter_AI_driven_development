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
| 0 | Project setup, CMake, raygui, Bullet static libs | ✅ | 2026-11-06 | 2026-11-06 | [devlog](project_setup_and_build_system_devlog.md) |
| 1 | Core: Engine, Application, Time, Window | ✅ | 2026-11-06 | 2026-11-06 | [devlog](core_devlog.md) |
| 2 | Input | ✅ | 2026-11-06 | 2026-11-06 | [devlog](input_devlog.md) |
| 3 | Resources and rendering | ✅ | 2026-11-06 | 2026-11-06 | [devlog](resources_and_rendering_devlog.md) |
| 4 | Audio | ✅ | 2026-11-06 | 2026-11-06 | [devlog](audio_devlog.md). Moved before Scene/Physics (D9) |
| 5 | Scene: GameActor, Component, Level | ✅ | 2026-11-06 | 2026-11-07 | [devlog](scene_devlog.md) |
| 6 | Physics (Bullet 3.25) | ✅ | 2026-11-07 | 2026-11-07 | [devlog](physics_devlog.md) |
| 7 | FX: particles and lines | ✅ | 2026-11-07 | 2026-11-07 | [devlog](fx_devlog.md) |
| 8 | Events, modes, menus | ✅ | 2026-11-07 | 2026-11-08 | [devlog](devlogs/events_modes_menus_devlog.md) |
| 9 | Editor mode | ✅ | 2026-11-08 | 2026-11-08 | [devlog](devlogs/editor_mode_devlog.md) |
| 10 | Game: state and player | 🟡 | 2026-11-08 | | |
| 11 | Game: scaling mechanic | ⬜ | | | |
| 12 | Game: level flow, UI, music | ⬜ | | | |
| 13 | Polish and release | ⬜ | | | |

**Current focus:** Phase 10
**Branch:** `game_state_and_player` (from `integration`)

---

## Phase checklists

### Phase 0 — Project setup and build system ✅
- [x] Root `CMakeLists.txt`: `engine` static lib + `my_app` exe, C++20
- [x] Remove the `Makefile` for good; update `README.md` build instructions
- [x] Vendor `raygui.h` into `include/raylib/` (raygui 5.1-dev, upstream `1abccb3a53`)
- [x] `tools/build_bullet.sh` (bullet3 tag **3.25**, arm64, static, single precision, demos/extras off)
- [x] Commit `lib/libBulletDynamics.a`, `lib/libBulletCollision.a`, `lib/libLinearMath.a` + `include/bullet/` + Bullet `LICENSE.txt`
- [x] Bullet include path PRIVATE to `engine` (+ static libs linked)
- [x] `assets/` folder + post‑build copy
- [x] `ENG_EDITOR` compile definition (Debug = 1, Release = 0, `-DENG_EDITOR_IN_RELEASE=ON` to force 1) + `eng::IsEditorBuild()`
- [x] **Done when:** the old free‑camera `main.cpp` runs from CMake, and a throw‑away `btDiscreteDynamicsWorld` links

### Phase 1 — Core ✅
- [x] `eng::Application` (Init / Update / FixedUpdate / DrawHUD / Destroy + mode hooks)
- [x] `eng::Engine` singleton, `SetApplication` ownership transfer, `Init / Run / Shutdown`
- [x] Window flags (MSAA, VSync, resizable, HighDPI), `SetExitKey(KEY_NULL)`, `ChangeDirectory(GetApplicationDirectory())`
- [x] `Time` (clamped dt, fixed‑step accumulator), `WindowInfo`, `World`, `Log`
- [x] `main.cpp` reduced to the reference pattern; temporary `ScaleGame` draws a cube
- [x] **Done when:** window opens via `Engine::Run`, cube renders, clean shutdown *(validated by the developer)*

### Phase 2 — Input ✅
- [x] `InputManager::Poll()` (down / pressed / released, mouse delta, wheel)
- [x] `InputMap` actions and axes (+ debug overlay in `ScaleGame`)
- [x] Cursor lock / unlock (+ temporary `C` toggle and mouse‑look fly camera in `ScaleGame`)
- [x] **Done when:** debug overlay shows action states; mouse look works with the cursor locked *(validated by the developer)*

### Phase 3 — Resources and rendering ✅
- [x] `ResourceManager` (cache + `UnloadAll`, primitives)
- [x] `Camera`, `ActiveCamera`
- [x] `Material` (tint, shader, params)
- [x] `RenderCommand`, `RenderQueue` (+ Engine 3D pass, `Application::SubmitRenderables`, 10‑cube scene)
- [x] `RenderTarget`, `DebugDraw` (+ minimap and debug shapes in `ScaleGame`, F2 toggle)
- [x] Optional lit shader (`assets/shaders/lit.vs/.fs`: sun + ambient + specular; scene uniforms from `World`)
- [x] **Done when:** 10 cubes with different materials render through the `RenderQueue` *(validated by the developer)*

### Phase 4 — Audio ✅
- [x] `AudioManager` (alias voice pool, volumes, duck) (+ placeholder SFX generator, pitched stretch/shrink demo)
- [x] `AudioSource` (standalone class; becomes a `Component` in Phase 5, D10) (+ listener from the active camera, spatial ping demo on K)
- [x] `MusicPlayer` (stream, loop, fade) (+ cross‑fade, `LoadMusic`, placeholder menu/game themes, M / P demo)
- [x] **Done when:** pitched SFX on key press, music fades in, pause ducks music (demo key until Phase 8) *(validated by the developer)*

### Phase 5 — Scene ✅
- [x] `GameActor` (transform, hierarchy, world matrix, `MarkForDestroy`)
- [x] `Component` base + `AddComponent<T> / GetComponent<T>`
- [x] `Level` (factory, update, fixed update, submit, `FlushDestroyed`) (+ Engine loop wiring)
- [x] `MeshRenderer` (+ demo scene as actors: rotating ring, Cube 0 → Moon, X self‑destruct, R rebuild)
- [x] `AudioSource` becomes a `Component` (position from its actor)
- [x] **Done when:** children follow a rotating parent; destroying an actor mid‑update is safe *(validated by the developer)*

### Phase 6 — Physics (Bullet 3.25) ✅
- [x] `BulletConvert` (Vector3 / Quaternion / Transform ↔ bt types)
- [x] `PhysicsWorld` pimpl: init, `Step`, `Shutdown` (correct delete order) (+ Engine: init, step per fixed tick, shutdown before resources)
- [x] `PhysicsLayers` groups and masks (default masks checked symmetric by `static_assert`)
- [x] `Collider` (box, sphere, capsule, convex hull, static mesh, compound bottom pivot, `SetScale`)
- [x] `RigidBody` (static / dynamic / kinematic, `Teleport`, `OnScaleChanged`, impulses) (+ physics sandbox demo; fix: `eng::DecomposeTransform` replaces raymath's row‑based `MatrixDecompose`)
- [x] `ActorMotionState` (world ↔ parent‑local, scale kept, optional rotation write‑back)
- [x] `Raycast` (`ClosestRayResultCallback`, mask filter, user pointer → actor) (+ crosshair aim, LMB kick, X delete in the sandbox)
- [x] `TriggerVolume` (ghost object, `btGhostPairCallback`, enter/exit diff) (+ exact `contactPairTest`, `Component::OnTriggerEnter/Exit`, sandbox trigger zone)
- [x] `CharacterMotor` — dynamic capsule: no rotation, own gravity, walk velocity on the ground plane, sphere‑sweep grounding, 45° slope limit, jump + grace time, warp, ground‑body velocity (+ grounded gravity along the normal, steep‑slope input cut, playable V mode in the sandbox)
- [x] `PhysicsDebugDrawer` + F3 toggle (wireframes + contact normals in the Engine 3D pass)
- [x] CCD for small dynamic bodies (`CcdMode` Auto / On / Off; Auto below 0.3 m, re‑checked on scale changes; sandbox pellets at a 5 cm wall)
- [x] **Done when:** *(validated by the developer)* sandbox (plane + 20 crates + ramp) at 60 FPS; crates stack and sleep; capsule climbs a 30° ramp without sliding, stops at 50°, jumps on a crate, pushes a light crate; center ray reports the hit crate; trigger Enter/Exit fire once each; F3 wireframes; deleting a crate mid‑sim is safe

### Phase 7 — FX ✅
- [x] `ParticleEmitter` (burst, gradient, alpha fade) + pooled `ParticleSystem` (+ LMB / RMB impact bursts in the sandbox)
- [x] `LineRenderer` (thick gradient line) (+ RenderQueue transparent line pass: open tube, per‑vertex gradient; LMB / RMB beams that fade in 0.35 s)
- [x] **Done when:** click spawns a colored burst and a fading beam *(validated by the developer)*

### Phase 8 — Events, modes, menus ✅
- [x] `EventBus` (queued dispatch)
- [x] `ModeMachine` (MainMenu / Play / Paused / Editor; cursor, audio duck, hooks) (+ ESC / TAB, `ModeChangedEvent`, `Application::GetStartMode`; removed the C / P demo keys)
- [x] `UIStack`, `UIScreen`, `UIStyle`, `RayGuiImpl.cpp`
- [x] `MainMenuScreen`, `PauseMenuScreen`, `MessageScreen` (+ ModeMachine shows the mode's screen, editor placeholder banner, Restart rebuilds the sandbox, EventBus → dialog demo)
- [x] Auto‑pause on focus loss (`ModeMachine::HandleFocus`, focus‑lost edge only; never auto‑resumes)
- [x] Physics not stepped while Paused / Menu / Editor (+ fixed accumulator, `Level::Update` and particles gated on Play; sandbox clicks ignored outside Play)
- [x] **Done when:** Menu → Start → Play → ESC pause (frozen, crates stay mid‑air) → Resume exactly where it was → Menu → Quit

### Phase 9 — Editor mode ✅
- [x] TAB enter/exit, `LevelSnapshot` capture (+ `Editor` coordinator owned by the Engine; snapshot = actor state, deleted actors can't be restored until the serializer)
- [x] `EditorCamera` (RMB look + WASD/QE fly, Shift fast, wheel speed, `FocusOn` for F; swaps the active camera on enter/exit)
- [x] `Picker` (physics raycast first, render bounds fallback) (+ Editor selection: LMB picks, orange bounds highlight, F focuses the camera, selection dropped when the actor goes)
- [x] `TransformGizmo` (move / scale) → `RigidBody::Teleport`, `Collider::SetScale` (W move / R scale, constant screen size, drag projects the mouse ray onto the axis, works with parented actors)
- [x] `HierarchyPanel` (tree, select, rename, Add Cube / Add Trigger / Delete; Duplicate waits for the serializer) (+ panel layout helpers, hotkeys quiet while typing, clicks over panels never pick, banner shows the controls)
- [x] `InspectorPanel` (incl. `RigidBody` / `Collider` fields) (+ `InspectorLayout` widgets, `Component::DrawInspector`, `ActorEdit` shared by gizmo and inspector; collider edits re‑create the body)
- [x] `EditorToolbar` (Play / Reset / Move / Scale; Save / Load / Ctrl+S disabled until the serializer task)
- [x] `LevelSerializer` + `ComponentRegistry` (`.lvl` format) (+ `PropertyBag`, `Component::Serialize / Deserialize` for MeshRenderer, Collider, RigidBody, TriggerVolume, CharacterMotor; toolbar Save / Load / Ctrl+S, Duplicate (Ctrl+D) enabled; `Application::GetLevelPath / OnLevelLoaded`; saves are mirrored into the source `assets/`)
- [x] Bullet debug wireframes in the editor overlay (+ forced on while editing and restored on exit, toolbar Physics toggle, editor grid, translucent trigger boxes from the collider)
- [x] **Done when:** edit a level mid‑game, save, exit with edits live; reload reproduces it; Reset restores positions **and** velocities

### Phase 10 — Game: state and player 🟡
- [x] `ActorHandle`: id‑based weak reference to a `GameActor` (`Level` lookup, null once deleted); game code stores handles instead of raw pointers (D11) (+ `GameActor::GetId`, `Level::FindActor`; the sandbox's player, crates and pellets are handles now)
- [x] `PlayerState`, `GameState` (+ events), `SoundFXBank`, `LevelList` (+ `src/game/state/`; the sandbox uses them: wheel factor, scaling SFX, Idle → Playing on Start / Restart, status line in the HUD)
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
| D2 | 2026-11-06 | `GameActor` + light `Component`s | Unity scripts map 1:1 to components | §0, Phase 5 |
| D3 | 2026-11-06 | Engine `ModeMachine` (Menu/Play/Paused/Editor); game `GameState` (win/lose) | Engine modes are generic; win/lose are game rules | §3.3 |
| D4 | 2026-11-06 | **Physics: Bullet 3.25** instead of a custom AABB layer | Review feedback; real rigid bodies, rotated shapes, runtime scaling | §0, Phase 6 |
| D5 | 2026-11-06 | Bullet shipped as prebuilt static libs in `lib/` + headers in `include/bullet/` (not a submodule) | Keeps the repo's "clone and run" setup | Phase 0 |
| D6 | 2026-11-06 | Bullet hidden behind `eng::PhysicsWorld` pimpl; game never includes Bullet | Backend swappable, faster game builds | Phase 6 |
| D7 | 2026-11-06 | ~~Player uses `btKinematicCharacterController`; fallback = dynamic capsule~~ **Superseded by D8** | Closest match to Unity `CharacterController` | Phase 6, §10 |
| D8 | 2026-11-06 | **Player uses a dynamic capsule `btRigidBody`** (no rotation, own gravity, sweep grounding); fallback = `btKinematicCharacterController` | Review feedback; avoids the kinematic controller's slope jitter; pushes crates and rides stretched props for free | Phase 6, Phase 10, §10 |
| D9 | 2026-11-06 | **Phase order changed by the developer:** 4 Audio, 5 Scene, 6 Physics (was 4 Scene, 5 Physics, 6 Audio) | Audio is simpler and has fewer dependencies. Devlogs written before this keep the old numbers | §5, tracker |
| D10 | 2026-11-06 | `AudioSource` is a standalone class in Phase 4 and becomes a `Component` in Phase 5; "pause ducks music" is shown with a demo key until Phase 8 | `Component` and Pause don't exist yet when Audio is built | Phase 4, 5, 8 |
| D11 | 2026-11-07 | `ActorHandle` (id‑based weak actor reference) added as the first Phase 10 task | Developer decision after the Phase 6 ASan finding: gameplay code (player, triggers, beams) must not hold raw `GameActor*` that can dangle | Phase 10, known issues |
| D12 | 2026-11-08 | Level files use one `PropertyBag` (key=value) for both `Serialize` and `Deserialize` instead of separate `LevelWriter` / `LevelReader`; actors with an unregistered component are not saved (transient beams, timers); mesh colliders and model meshes are not serializable yet | Simpler API, same format as plan §8; avoids saving runtime-only actors | Phase 9 |
| D13 | 2026-11-08 | `ResourceManager::SetDefaultLitShader` carries the game's lit shader (`lit=1` in level files, editor cubes) | Level loading must work in release builds, so it can't live in the editor class | Phase 9 |

---

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| Dynamic capsule: slope sliding / bouncing / wall sticking | 6 / 10 | open | Ground‑plane projection, friction 0, grounded glue; see plan §10 |
| Player launched by a crate scaling under them | 11 | open | Clamp player vertical velocity after the step |
| Game code holding raw `GameActor*` dangles if the actor is destroyed after the game pruned its list that frame (ASan use‑after‑free in a Phase 6 test that deleted after the prune) | 6 → 10 | mitigated (`ActorHandle`; raw pointers from `Get()` must not outlive the frame) | Engine side is safe. `ActorHandle` scheduled as the first Phase 10 task (D11) |
| Scaling causes deep penetration → velocity spikes | 11 | open | Clamp velocity, overlap check before stretch |
| Bullet precision / arch mismatch | 0 | mitigated | Libs verified arm64, macOS 11.0, single precision |
| Xcode 26 linker can't read the Command Line Tools SDK 27.2 (`tapi error: malformed file`) | 0 | resolved | Switched to the Command Line Tools toolchain (`xcode-select -s /Library/Developer/CommandLineTools`, ld‑27106); plain `cmake` builds with no SDK override. README lists the fixes |

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-08 | 9 | Developer validated Phase 9; devlog written; merged into `integration` | Start Phase 10 on `game_state_and_player` | — |
| 2026-11-08 | 9 | All 10 Phase 9 tasks committed on `editor_mode` (Editor + LevelSnapshot, EditorCamera, Picker, TransformGizmo, Hierarchy / Inspector panels, toolbar, LevelSerializer, wireframe overlay); headless tests for snapshot, camera, picker, gizmo drag, serializer round trip (identical text on a second pass); real sandbox saved, edited, reloaded and played | Developer runs the Phase 9 **Done when** check | — |
| 2026-11-08 | 8 | Developer validated Phase 8; devlog written; merged into `integration` | Start Phase 9 on `editor_mode` | — |
| 2026-11-08 | 8 | All 6 Phase 8 tasks committed on `events_modes_menus`: focus‑loss auto‑pause and simulation freeze outside Play (closes the known gap); app smoke run starts clean in MainMenu | Developer runs the Phase 8 **Done when** check | — |
| 2026-11-08 | 8 | 4 of 6 Phase 8 tasks committed on `events_modes_menus` (EventBus, ModeMachine, UI stack, menu screens); screenshots confirm main menu, pause menu and dialog. **Known gap:** the simulation still runs behind the pause menu until the last task | Auto‑pause on focus loss; freeze simulation outside Play | — |
| 2026-11-07 | 7 | Developer validated Phase 7 (and deleted the accidental `list` branch); devlog written; merged into `integration` | Start Phase 8 on `events_modes_menus` | — |
| 2026-11-07 | 7 | Both Phase 7 tasks committed on `fx`; particle pool test passed; ASan screenshot runs: 80 particles + 2 beams on fire, both beams gone after the fade; replaced capped cylinders (stacked hexagon artifacts) with an open per‑vertex‑colored tube | Developer runs the Phase 7 **Done when** check | — |
| 2026-11-07 | 6 | Developer validated Phase 6; `ActorHandle` scheduled in Phase 10 (D11); devlog written; merged into `integration` | Start Phase 7 on `fx` | — |
| 2026-11-07 | 6 | All 11 Phase 6 tasks committed on `physics`; headless tests (convert, collider, motion state, rigid body, raycast, trigger, character motor, CCD), most under ASan, all pass; found and fixed raymath `MatrixDecompose` (row‑based) bug, rotation‑lock bypass, convex hull margin, motor slope sliding; final ASan sandbox run: 0 of 15 pellets tunnelled, 59 FPS, 20/20 crates asleep | Developer runs the Phase 6 **Done when** check | — |
| 2026-11-07 | 5 | Developer validated Phase 5; devlog written; merged into `integration` | Start Phase 6 on `physics` | — |
| 2026-11-06 | 5 | All 5 Phase 5 tasks committed on `scene`; headless GameActor (12 checks), Component lifecycle (6) and Level destroy‑mid‑update (11, under AddressSanitizer) tests passed; full game built with ASan: self‑destruct, ring rebuild and destroy‑mid‑ping runs clean | Developer runs the Phase 5 **Done when** check | — |
| 2026-11-06 | 4 | Developer validated Phase 4; devlog written; merged into `integration` | Start Phase 5 on `scene` | — |
| 2026-11-06 | 4 | All 3 Phase 4 tasks committed on `audio`; voice pool check (3 overlapping plays → own voices), AudioSource gain/pan headless check, timed fade/cross‑fade/duck check all passed; clean shutdown order verified | Developer runs the Phase 4 **Done when** check | — |
| 2026-11-06 | 3 | Developer validated Phase 3 and moved Audio to Phase 4; plan, tracker and code comments renumbered (D9, D10); devlog written; merged into `integration` | Start Phase 4 on `audio` | — |
| 2026-11-06 | 3 | All 6 Phase 3 tasks committed on `resources_and_rendering`; camera math check passed; screenshot check found and fixed a material‑parameter leak between materials sharing a shader; self‑closing run confirmed clean shutdown (3 resources unloaded) | Developer runs the Phase 3 **Done when** check | — |
| 2026-11-06 | 2 | Developer validated Phase 2; devlog written; merged into `integration` | Start Phase 3 on `resources_and_rendering` | — |
| 2026-11-06 | 2 | All 3 Phase 2 tasks committed on `input`; 4 s smoke run clean (no unbound‑input warnings, no errors) | Developer runs the Phase 2 **Done when** check | — |
| 2026-11-06 | 1 | Developer validated Phase 1; devlog written; merged into `integration` | Start Phase 2 on `input` | — |
| 2026-11-06 | 1 | All 5 Phase 1 tasks committed on `core`; `Time` fixed-step unit check passed; 4 s smoke run: cwd switches to the app dir, HighDPI 1280×720 → 2560×1440, no warnings | Developer runs the Phase 1 **Done when** check (incl. closing the window → Destroy + Shutdown logs) | — |
| 2026-11-06 | 0 | Toolchain switched to Command Line Tools (fixes `tapi` linker error); developer validated Phase 0; devlog written; merged into `integration` | Start Phase 1 on `core` | — |
| 2026-11-06 | 0 | Branch `integration` + `project_setup_and_build_system`; all 8 Phase 0 tasks committed; Bullet link probe passed (box falls 10 → 5.19 m in 1 s); game target confirmed unable to include Bullet | Developer runs the Phase 0 **Done when** check | — |
| 2026-11-06 | — | Player motor switched to a dynamic capsule (D8) | Start Phase 0 | — |
| 2026-11-06 | — | Architecture plan written; physics layer switched to Bullet 3.25; progress tracker created | Start Phase 0 | — |
