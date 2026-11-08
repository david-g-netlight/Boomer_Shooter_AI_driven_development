# Architecture Plan — Raylib Engine + "Build to Scale" FPS

A smaller game engine built on **raylib 5.6 (static `lib/libraylib.a`)** that has the same structure as the
reference OpenGL engine (`p_Game_Engine_Cpp_OpenGL`, *"The Sleipnir Engine"*). On top of that it adds:

- an **Editor mode** you can enter and leave at runtime,
- a **Main Menu** with Start / Pause / Resume,
- everything the gameplay of **Build to Scale** needs (the GMTK Jam Unity project).

**The game in one line:** a first‑person puzzle game. Left click **stretches** (scales up) the object under
the crosshair, right click **shrinks** it, and the mouse wheel picks the **scale factor** (x0.25 / x0.5 / x0.75).
Reach the exit trigger to win, and don't fall into the kill zone.

---

## 0. Guiding decisions

| Decision | Choice | Why |
|---|---|---|
| Engine and game split | Two CMake targets: `engine` (static lib) + `my_app` (executable) | Mirrors the reference `engine/` library + game executable |
| Engine and Application contract | Kept from the reference: `Engine` singleton **owns** `Application*`; `Init / Update / Destroy` | Same mental model as the reference |
| Graphics backend | raylib replaces GLFW + GLEW + GraphicsAPI + ShaderProgram + VertexLayout + Mesh buffers | The task says raylib provides these |
| Math | `raymath.h` (`Vector3`, `Matrix`, `Quaternion`, C++ operators) replaces GLM | Ships with raylib |
| Immediate GUI | `raygui.h` (single header) replaces Dear ImGui | Same ecosystem, no backend glue needed |
| Scene model | `GameActor` hierarchy (from the reference) **plus** light `Component`s | Unity scripts like `ScalingController` attach to many different props, so they map 1:1 to components |
| Physics | **Bullet Physics 3.25** (`bulletphysics/bullet3`): `btDiscreteDynamicsWorld`, rigid bodies, ghost‑object triggers, ray tests, **dynamic capsule** player (`btCapsuleShape` rigid body) | Replaces Unity PhysX: real rigid bodies (Unity props had `Rigidbody`), rotated colliders, runtime shape scaling |
| Physics isolation | Bullet headers are included **only in `src/engine/physics/*.cpp`**; the game sees `eng::` types only | The game never depends on Bullet, so the backend can be swapped |
| Bullet packaging | Prebuilt **static** `lib/libBulletDynamics.a`, `libBulletCollision.a`, `libLinearMath.a` + headers in `include/bullet/`, produced by `tools/build_bullet.sh` | Same "clone and run" setup as `lib/libraylib.a` |
| Update model | Variable `Update(dt)` + fixed `FixedUpdate(1/50 s)`; Bullet stepped once per fixed tick | The Unity scripts rely on `FixedUpdate` (scaling, cooldown) |
| Game states | Engine `ModeMachine` (Menu/Play/Pause/Editor) + game `GameState` (Started/Won/Lost) | Engine modes are generic; win and lose are game rules |
| Events | Small `EventBus` (typed pub/sub, dispatched at end of frame) | Replaces the ScriptableObject listener lists (`GameStartListener`, …) |
| Levels | Text `.lvl` files saved and loaded by the editor | Replaces Unity scenes and `SceneRef` |
| Language | C++20 (the reference needs `requires std::derived_from`) | Bump from the current C++17 |

**Naming rule (important with raylib):** raylib defines global C types named `Camera`, `Mesh`, `Material`,
`Model`, `Shader`, `Transform`, `Ray`. Engine types live in `namespace eng`. **Never write `using namespace eng;`**
in headers or game code (the reference `Game.h` does this, and here it would cause ambiguities). Write `eng::Level`
etc., and `::Mesh` when you mean the raylib type. raylib's own `::Transform` (translation + quaternion + scale)
is exactly the reference `Transform`, so the engine reuses it directly.

---

## 1. Repository layout (the existing arrangement is kept)

`include/`, `lib/`, `src/`, `docs/` stay where they are. The engine and the game become sub-folders of `src/`.

```
b-RayLib_static_starter_project_MacOS_M2/
├── CMakeLists.txt              # engine (static lib) + my_app (exe) + asset copy
├── README.md                   # update build instructions (Makefile -> CMake)
├── include/
│   ├── raylib/                 # unchanged: raylib.h, raymath.h, rlgl.h, rcamera.h ...
│   │   └── raygui.h            # NEW: vendored single-header GUI
│   └── bullet/                 # NEW: Bullet 3.25 headers (copied from bullet3/src)
│       ├── btBulletDynamicsCommon.h
│       ├── btBulletCollisionCommon.h
│       ├── BulletCollision/ BulletDynamics/ LinearMath/
├── lib/
│   ├── libraylib.a             # unchanged
│   ├── libBulletDynamics.a     # NEW (Release, arm64, single precision)
│   ├── libBulletCollision.a    # NEW
│   └── libLinearMath.a         # NEW
├── tools/
│   └── build_bullet.sh         # NEW: clone bullet3@3.25 -> build static libs -> copy into lib/ + include/bullet/
├── assets/                     # NEW: copied next to the executable after build
│   ├── levels/                 #   level_01.lvl, level_02.lvl ...
│   ├── models/                 #   gun.glb, crate.glb ...
│   ├── shaders/                #   lit.vs / lit.fs (optional basic lighting)
│   ├── audio/                  #   stretch.wav, shrink.wav, beam_*.wav, theme_menu.ogg, theme_game.ogg
│   └── fonts/
├── src/
│   ├── main.cpp                # Engine::GetInstance().SetApplication(new ScaleGame); Init; Run; Shutdown
│   ├── engine/                 # namespace eng  (library, no game knowledge)
│   │   ├── eng.h               # public umbrella header (like the reference eng.h)
│   │   ├── core/               # Engine, Application, Time, WindowInfo, World, EventBus, Log
│   │   ├── modes/              # EngineMode, ModeMachine
│   │   ├── input/              # InputManager, InputMap
│   │   ├── resources/          # ResourceManager
│   │   ├── render/             # Camera, ActiveCamera, Material, MeshRenderer, RenderQueue,
│   │   │                       # RenderTarget, LineRenderer, DebugDraw
│   │   ├── level/              # GameActor, Component, Level, LevelSerializer, ComponentRegistry
│   │   ├── physics/            # PhysicsWorld, Collider, RigidBody, TriggerVolume, CharacterMotor,
│   │   │                       # RaycastHit, PhysicsLayers, BulletConvert, ActorMotionState,
│   │   │                       # PhysicsDebugDrawer   (only these .cpp files include Bullet)
│   │   ├── audio/              # AudioManager, AudioSource, MusicPlayer
│   │   ├── fx/                 # ParticleEmitter, ParticleSystem
│   │   ├── ui/                 # UIScreen, UIStack, MainMenuScreen, PauseMenuScreen,
│   │   │                       # MessageScreen, UIStyle, RayGuiImpl.cpp
│   │   └── editor/             # Editor, EditorCamera, Picker, TransformGizmo,
│   │                           # HierarchyPanel, InspectorPanel, EditorToolbar, LevelSnapshot
│   └── game/                   # the Application layer ("Build to Scale")
│       ├── ScaleGame.h/.cpp
│       ├── state/              # GameState, PlayerState, SoundFXBank, LevelList
│       ├── managers/           # GameplayManager, GameUI, MusicManager, PlayerInput
│       ├── player/             # PlayerActor, FirstPersonController, FirstPersonLook, GunView, GunHUD
│       ├── scaling/            # ScalingGun, Scalable, RayBeamSpawner, RayBeam, MuzzleFX
│       └── triggers/           # TriggerZone, WinTrigger, LoseTrigger, NextLevelTrigger
└── docs/
    ├── ARCHITECTURE_PLAN.md    # this file
    └── DEV_PROGRESS.md         # phase checklist, decision log, session log
```

---

## 2. Reference engine → raylib engine mapping

| Reference (OpenGL / GLFW) | Raylib engine | Notes |
|---|---|---|
| `Engine` singleton (Meyer's), owns `Application*` | `eng::Engine`, same API: `GetInstance / SetApplication / Init / Run / Shutdown` | Subsystems held **by value**, accessed via getters |
| `Application` (`Init/Update/Destroy`, `NeedsToBeClosed`) | `eng::Application`, plus `FixedUpdate`, `DrawHUD`, mode hooks | See §6 |
| GLFW window + callbacks | `InitWindow`, `SetConfigFlags`, polling | No callbacks; `InputManager::Poll()` each frame |
| GLEW, `GraphicsAPI` | removed | raylib/rlgl own the GL context |
| `ShaderProgram` | `::Shader` loaded through `ResourceManager` | `LoadShader`, `SetShaderValue` |
| `VertexLayout`, `Mesh` (VAO/VBO/EBO) | `::Mesh` / `::Model` (`GenMeshCube`, `LoadModel`) | raylib uploads buffers |
| `Material` (shader + float param map) | `eng::Material`: `::Material` + named params → `SetShaderValue` on bind | Same API shape |
| `RenderQueue` + `RenderCommand{mesh, material, modelMatrix}` | Same, flushed inside `BeginMode3D` with `DrawMesh(mesh, mat, matrix)` | `std::vector` instead of fixed array |
| `FrameBuffer` (color + depth, resize) | `eng::RenderTarget` (`RenderTexture2D`) | Used for the editor viewport and an optional gun HUD texture |
| `Camera`, `ActiveCamera`, `PerspectiveParams`, `ECameraType` | `eng::Camera` (wraps `Camera3D`), `eng::ActiveCamera` | FPS camera in Play, `EditorCamera` in Editor |
| `InputManager` (key arrays, mouse delta, `friend Engine`) | Same class, filled by polling raylib | Adds wheel, "pressed this frame", action map |
| Mouse re-centering in `MousePositionCallback` | `DisableCursor()` + `GetMouseDelta()` | raylib handles re-centering |
| `GameActor`, `Transform`, parent/children, `MarkForDestroy` | Same + component list; `::Transform` reused | Deferred destroy at end of frame |
| `Level` (template `CreateGameActor<T>`) | Same, owns `std::vector<std::unique_ptr<GameActor>>` | Removes the fixed‑size TODO |
| `SetPlayMode(bool)` toggled with TAB | `ModeMachine`: **Play ⇄ Editor** with TAB | Becomes a full editor (§8) |
| `World` (up/right/forward), `Window` struct | `eng::World`, `eng::WindowInfo` | Renamed `WindowInfo` for clarity |
| `std::chrono` delta time | `eng::Time` (`GetFrameTime`, clamp, fixed‑step accumulator) | |
| Dear ImGui | raygui | Editor panels + menus |
| tinyobjloader, stb_image | `LoadModel` (OBJ/GLTF/GLB), `LoadTexture` | |
| *(no physics in the reference)* | `eng::PhysicsWorld` over Bullet `btDiscreteDynamicsWorld` | New subsystem, replaces Unity PhysX + `CharacterController` |
| Post‑build copy of debug assets / shaders | Post‑build copy of `assets/` | `ChangeDirectory(GetApplicationDirectory())` at init |

---

## 3. Architecture diagram

### 3.1 Layers

```
+=====================================================================================+
|                         APPLICATION LAYER   (src/game, "Build to Scale")             |
|                                                                                     |
|   ScaleGame : eng::Application                                                      |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
|   | GameplayManager|  | GameState      |  | PlayerState     |  | GameUI          |  |
|   | (levels flow)  |  | (start/win/lose)| | (factors, cd)   |  | (win/lose UI)   |  |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
|   | PlayerActor    |  | ScalingGun     |  | Scalable        |  | Win/Lose/Next   |  |
|   | FPController   |  | RayBeamSpawner |  | (smoothstep)    |  | TriggerZones    |  |
|   | FPLook, GunHUD |  | RayBeam, FX    |  |                 |  | MusicManager    |  |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
+==========================================^==========================================+
                                           |  #include <eng.h>   (public API only)
+==========================================|==========================================+
|                               ENGINE   (src/engine, namespace eng)                   |
|                                                                                     |
|   +------------------------------ CORE -----------------------------------------+   |
|   |  Engine (singleton, owns Application, owns every subsystem by value)         |   |
|   |  Time (dt, fixed step)   WindowInfo   World   EventBus   Log                 |   |
|   +------------------------------------------------------------------------------+   |
|   +-------------------------- ModeMachine --------------------------------------+   |
|   |      MainMenu  <-->  Play  <-->  Paused            Play <--TAB--> Editor     |   |
|   +------------------------------------------------------------------------------+   |
|                                                                                     |
|   +---------+ +-----------+ +-----------+ +-----------+ +---------+ +-------+       |
|   | Input   | | Resources | | Render    | | Level     | | Physics | | Audio |       |
|   | Manager | | Manager   | | Camera    | | GameActor | | World   | | Mgr   |       |
|   | InputMap| | (cache,   | | Material  | | Component | | RigidBdy| | Source|       |
|   |         | |  unload)  | | RenderQ   | | Serializer| | Collider| | Music |       |
|   |         | |           | | RTarget   | | Registry  | | Trigger | |       |       |
|   |         | |           | |           | |           | | CharMot.| |       |       |
|   +---------+ +-----------+ +-----------+ +-----------+ +----|----+ +-------+       |
|                                                     (pimpl, .cpp only)             |
|   +-----------+ +------------------------+ +------------------------------------+   |
|   | FX        | | UI (raygui)            | | Editor                             |   |
|   | Particles | | UIStack, MainMenu,     | | EditorCamera, Picker, Gizmo,       |   |
|   | LineRend. | | PauseMenu, MessageScr. | | Hierarchy, Inspector, Toolbar      |   |
|   +-----------+ +------------------------+ +------------------------------------+   |
+==========================================^==========================================+
                                           |
+==========================================|=========================|================+
|   raylib 5.6 (static libraylib.a)                         |  Bullet 3.25 (static)      |
|   + raymath.h + rlgl.h + raygui.h                         |  libBulletDynamics.a       |
|   rcore: window/input/time | rmodels: mesh/model          |  libBulletCollision.a      |
|   raudio | rtext | rshapes                                |  libLinearMath.a           |
+===========================================================+============================+
|   macOS (M2):  Cocoa  IOKit  CoreVideo  OpenGL  GLUT  frameworks                     |
+=====================================================================================+
```

### 3.2 Ownership

```
main()
  └─ Engine (static singleton) ───────────────── owns ──> Application* (ScaleGame)   [delete in ~Engine]
       ├─ InputManager        (by value, ctor private, friend Engine)
       ├─ ResourceManager     (by value) ── owns ──> ::Model / ::Shader / ::Texture / ::Sound / ::Music
       ├─ RenderQueue         (by value) ── holds NON-owning RenderCommand pointers
       ├─ ActiveCamera        (by value) ── points to the Camera of the current mode
       ├─ PhysicsWorld        (by value) ── owns (pimpl) btDefaultCollisionConfiguration, btCollisionDispatcher,
       │                                    btDbvtBroadphase, btSequentialImpulseConstraintSolver,
       │                                    btDiscreteDynamicsWorld, btGhostPairCallback, PhysicsDebugDrawer
       │                                    NON-owning: btCollisionObject* (added/removed by components)
       ├─ AudioManager        (by value)
       ├─ ParticleSystem      (by value) ── owns pooled particles
       ├─ UIStack             (by value) ── owns std::unique_ptr<UIScreen>
       ├─ ModeMachine         (by value)
       ├─ EventBus            (by value)
       └─ Editor              (by value, compiled in with ENG_EDITOR=1)

ScaleGame
  └─ eng::Level  ── owns ──> std::unique_ptr<GameActor>  ── owns ──> std::unique_ptr<Component>
                                     │  parent/children pointers are NON-owning
                                     │
                                     └─ RigidBody / TriggerVolume / CharacterMotor components
                                          own their btCollisionShape, btMotionState, btRigidBody /
                                          btPairCachingGhostObject / capsule btRigidBody (player)
```

Rules:
1. Only `Engine` deletes `Application` (same as the reference).
2. Only `ResourceManager` unloads GPU and audio resources. It does so in `Engine::Shutdown()` **before** `CloseAudioDevice()` / `CloseWindow()`.
3. `MarkForDestroy()` never deletes right away. `Level::FlushDestroyed()` runs at the end of the frame.
4. Components register with subsystems in `OnEnable` and unregister in `OnDisable` or the destructor, so no dangling pointers are left behind.
5. Bullet objects are always **removed from the `btDiscreteDynamicsWorld` before they are deleted**, and deleted in this order: body/ghost → motion state → shape. `PhysicsWorld` is destroyed **after** `Level::Clear()`.
6. Every `btCollisionObject` stores its owning `eng::Collider*` in `setUserPointer()`. That is how ray hits and trigger overlaps get back to a `GameActor`.

### 3.3 Engine modes (state machine)

```
                 +------------+
   Boot ───────> |  MainMenu  | <──────────────────────────────┐
                 +------------+                                │
                  │ Start                                      │ "Main Menu"
                  v                                            │
                 +------------+   ESC / focus lost   +------------+
                 |    Play    | ───────────────────> |   Paused   |
                 |            | <─────────────────── |            |
                 +------------+   ESC / "Resume"     +------------+
                  ^        │                                │ "Restart"
             TAB  │        │ TAB                            └──> reload level ─> Play
                  │        v
                 +------------+
                 |   Editor   |   (simulation frozen, free camera, panels)
                 +------------+

   Game-level overlay (inside Play):  GameState.Won  -> GameUI pushes "Level Complete" (Next / Menu)
                                      GameState.Lost -> GameUI pushes "You fell!"     (Retry / Menu)
```

| Mode | `Update` / `FixedUpdate` | Camera | Cursor | UI drawn | Music |
|---|---|---|---|---|---|
| MainMenu | no (menu backdrop only) | slow orbit of level | visible | `MainMenuScreen` | menu theme |
| Play | **yes** | FPS camera | **locked** (`DisableCursor`) | `Application::DrawHUD` | gameplay theme |
| Paused | no | frozen | visible | frozen frame + `PauseMenuScreen` | ducked |
| Editor | no (editor tools only) | `EditorCamera` | visible | editor panels + gizmos | ducked |

---

## 4. Game loop diagram

```
Engine::Run()
│
│  while (!WindowShouldClose() && !app->NeedsToBeClosed())        // ESC is NOT the exit key (SetExitKey(KEY_NULL))
│  │
│  ├─ 1. TIME        time.BeginFrame()      dt = min(GetFrameTime(), 0.1f); accumulator += dt (Play only)
│  │
│  ├─ 2. INPUT       input.Poll()           keys, mouse buttons, wheel, GetMouseDelta(), "pressed this frame"
│  │
│  ├─ 3. MODES       modes.HandleInput()    ESC -> Pause/Resume,  TAB -> Editor/Play,  focus lost -> Pause
│  │                 modes.ApplyPendingTransition()  -> calls app->OnPause()/OnResume()/OnEnterEditor()...
│  │
│  ├─ 4. SIMULATE    switch (modes.Current())
│  │   │
│  │   ├─ PLAY ──┬─ while (accumulator >= FIXED_DT /*0.02*/)         // FIXED STEP
│  │   │         │      level.FixedUpdate(FIXED_DT)   -> Scalable (setLocalScaling), ScalingGun cooldown,
│  │   │         │                                       FirstPersonController (setWalkDirection / jump)
│  │   │         │      app->FixedUpdate(FIXED_DT)
│  │   │         │      physics.PushKinematics()      -> kinematic/edited actors -> btTransform
│  │   │         │      physics.Step(FIXED_DT)        -> btDiscreteDynamicsWorld::stepSimulation(FIXED_DT, 1, FIXED_DT)
│  │   │         │                                       (character controller is an action: updated inside the step)
│  │   │         │      physics.PullDynamics()        -> ActorMotionState::setWorldTransform -> actor ::Transform
│  │   │         │      physics.DispatchTriggers()    -> ghost overlaps diffed vs last tick -> OnTriggerEnter / Exit
│  │   │         │      accumulator -= FIXED_DT
│  │   │         ├─ level.Update(dt)                  -> FirstPersonController, RayBeam, ...
│  │   │         ├─ app->Update(dt)
│  │   │         ├─ particles.Update(dt)
│  │   │         └─ activeCamera.Update(dt)
│  │   │
│  │   ├─ EDITOR ── editor.Update(dt)                 -> EditorCamera fly, picking, gizmo drag, shortcuts
│  │   │
│  │   └─ MENU / PAUSED ── (no simulation)
│  │
│  ├─ 5. AUDIO       audio.Update(dt)       UpdateMusicStream() every frame, volume fades / ducking
│  │
│  ├─ 6. LATE        events.Dispatch()      queued GameStarted / GameWon / GameLost / LevelLoaded ...
│  │                 level.FlushDestroyed() actors marked for destroy are deleted here
│  │                 if (IsWindowResized()) renderTargets.Resize(...)
│  │
│  └─ 7. RENDER      BeginDrawing()
│                      ClearBackground(world.skyColor)
│                      BeginMode3D(activeCamera.Get())
│                        level.SubmitRenderables(renderQueue)  -> MeshRenderer -> RenderCommand
│                        renderQueue.Flush()                   -> DrawMesh(mesh, material, matrix)
│                        lineRenderers / particles.Draw()      -> beams, sparks
│                        if EDITOR: DrawGrid, physics.DebugDraw() (btIDebugDraw -> DrawLine3D),
│                                   selection, gizmo
│                      EndMode3D()
│                      if PLAY:   app->DrawHUD()               -> crosshair, "x0.25", cooldown
│                      if EDITOR: editor.DrawPanels()          -> raygui hierarchy / inspector / toolbar
│                      ui.Draw()                               -> top-most UIScreen (menus, win/lose)
│                    EndDrawing()                              -> swap buffers + PollInputEvents
│
└─ Shutdown():  app->Destroy() -> level.Clear() (removes all bodies) -> physics.Shutdown()
                -> resources.UnloadAll() -> CloseAudioDevice() -> CloseWindow()
```

---

**As implemented (Phases 1–8):** modes are applied at the *start* of the frame (`ModeMachine::HandleInput` + `ApplyPendingTransition`, right after `Input.Poll`); `ParticleSystem::Update` runs after `app->Update`; `UIStack::Update` runs every frame and `UIStack::Draw` after `DrawHUD`; physics steps inside the fixed loop as `SyncTriggers → stepSimulation → DispatchTriggers`; `EventBus::Dispatch` runs before `Level::FlushDestroyed`. Shutdown: UI cleared → `app->Destroy()` → `PhysicsWorld::Shutdown` → `ReleaseVoices` → `ResourceManager::UnloadAll` → `AudioManager::Shutdown` → `CloseWindow`.

---

## 5. Implementation plan (in order)

Each phase ends with something you can run. Do not start a phase before the previous one's **Done when** passes. Progress, decisions after this plan (D9–D11) and per‑phase devlogs: [`DEV_PROGRESS.md`](DEV_PROGRESS.md).

> Order changed on 2026-11-06 (decision D9): Audio moved before Scene and Physics because it has fewer dependencies. Phases 4–6 are now Audio, Scene, Physics.

### Phase 0 — Project setup and build system
- Switch to **CMake as the only build** (the `Makefile` is already deleted in the working tree); update `README.md`.
- Root `CMakeLists.txt`: `engine` static library + `my_app` executable, C++20, raylib frameworks linked `PUBLIC` on `engine`.
- Vendor `raygui.h` into `include/raylib/`; add `include/raylib` to the include path (`raygui.h` does `#include "raylib.h"`).
- Write `tools/build_bullet.sh` and commit its output (static libs + headers), the same way `libraylib.a` is committed:
  ```bash
  git clone --depth 1 --branch 3.25 https://github.com/bulletphysics/bullet3.git /tmp/bullet3
  cmake -S /tmp/bullet3 -B /tmp/bullet3/build -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_ARCHITECTURES=arm64 -DBUILD_SHARED_LIBS=OFF -DUSE_DOUBLE_PRECISION=OFF \
        -DBUILD_BULLET2_DEMOS=OFF -DBUILD_CPU_DEMOS=OFF -DBUILD_OPENGL3_DEMOS=OFF -DBUILD_EXTRAS=OFF \
        -DBUILD_UNIT_TESTS=OFF -DBUILD_BULLET3=OFF -DBUILD_PYBULLET=OFF -DUSE_GRAPHICAL_BENCHMARK=OFF \
        -DINSTALL_LIBS=OFF
  cmake --build /tmp/bullet3/build --target BulletDynamics BulletCollision LinearMath -j
  cp /tmp/bullet3/build/src/{BulletDynamics,BulletCollision,LinearMath}/lib*.a lib/
  rsync -a --include='*/' --include='*.h' --exclude='*' /tmp/bullet3/src/ include/bullet/
  ```
  Keep the Bullet zlib `LICENSE.txt` in `include/bullet/`.
  *Alternative (not chosen):* a git submodule + `add_subdirectory`. It is easier to upgrade but slower to build and breaks "clone and run".
- Create `src/engine/`, `src/game/`, `assets/`; post‑build step copies `assets/` next to the binary.
- Add `ENG_EDITOR` compile definition (ON for Debug, optional for Release).

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/engine/*.cpp)
add_library(engine STATIC ${ENGINE_SOURCES})
target_include_directories(engine PUBLIC src/engine include include/raylib)
target_include_directories(engine PRIVATE include/bullet)          # Bullet is an engine-private dependency
target_link_libraries(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/libraylib.a
    "-framework CoreVideo" "-framework IOKit" "-framework Cocoa" "-framework GLUT" "-framework OpenGL")
target_link_libraries(engine PRIVATE                               # order matters: Dynamics -> Collision -> LinearMath
    ${CMAKE_SOURCE_DIR}/lib/libBulletDynamics.a
    ${CMAKE_SOURCE_DIR}/lib/libBulletCollision.a
    ${CMAKE_SOURCE_DIR}/lib/libLinearMath.a)
target_compile_definitions(engine PUBLIC $<$<CONFIG:Debug>:ENG_EDITOR=1>)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
add_executable(my_app src/main.cpp ${GAME_SOURCES})
target_include_directories(my_app PRIVATE src/game)
target_link_libraries(my_app PRIVATE engine)
target_compile_options(my_app PRIVATE $<$<CONFIG:Debug>:-g -O0 -Wall> $<$<CONFIG:Release>:-O2 -Wall>)

add_custom_command(TARGET my_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:my_app>/assets)
```
**Done when:** the current `main.cpp` (3D free camera example) builds from the new CMake and still runs, and a throw‑away `.cpp` that creates a `btDiscreteDynamicsWorld` links.

### Phase 1 — Core: Engine, Application, Time, Window
- `eng::Application` (abstract), `eng::Engine` (Meyer's singleton, deleted copy/move, `SetApplication` ownership transfer).
- `Engine::Init`: `SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI)`,
  `InitWindow`, `SetExitKey(KEY_NULL)`, `ChangeDirectory(GetApplicationDirectory())`, `SetTargetFPS(60)`.
- `eng::Time` (dt, clamped dt, total time, fixed‑step accumulator), `eng::WindowInfo`, `eng::World`, `eng::Log`.
- `main.cpp` reduced to the reference pattern; a temporary `ScaleGame` draws the cube from the old example.

**Done when:** the window opens through `Engine::Run`, the cube renders, and closing the window calls `Destroy` + `Shutdown` cleanly.

### Phase 2 — Input
- `eng::InputManager` (private ctor, `friend Engine`), filled by `Poll()`: down/pressed/released keys and buttons, mouse position and delta, wheel.
- `eng::InputMap`: named actions → bindings (`"MoveForward" = W`, `"Fire" = MOUSE_LEFT`, `"AltFire" = MOUSE_RIGHT`, `"Jump" = SPACE`) and axes (`"Move"` WASD, `"Look"` mouse delta, `"Scroll"` wheel).
- Cursor control: `SetCursorLocked(bool)` → `DisableCursor()` / `EnableCursor()`.

**Done when:** a debug overlay prints action states, and mouse look works with the cursor locked.

### Phase 3 — Resources and rendering
- `eng::ResourceManager`: path‑keyed cache for `Model`, `Shader`, `Texture2D`, `Font` (`Sound` / `Music` added in Phase 4 with the audio device, which `AudioManager` owns); primitive generators (`GetCube()`, `GetPlane()`); `UnloadAll()`.
- `eng::Camera` (wraps `Camera3D`, fov/near/far, `Forward/Right/Up`, `Rotate(yaw,pitch)`), `eng::ActiveCamera`.
- `eng::Material` (wraps `::Material`, tint, optional shader, named float params like the reference).
- `eng::RenderCommand`, `eng::RenderQueue` (`Submit`, `Flush` inside `BeginMode3D`).
- `eng::RenderTarget` (`RenderTexture2D`, resize), `eng::DebugDraw` (lines, boxes, spheres, text‑in‑world, active only in Debug/Editor).
- Optional: `assets/shaders/lit.vs/.fs` (one directional light + ambient) so scaled boxes read well.

**Done when:** a hard‑coded scene of 10 cubes with different materials renders through the `RenderQueue`.

### Phase 4 — Audio
- `AudioManager`: `PlaySound(sound, volume, pitch)` with a small pool of `LoadSoundAlias` voices so the same SFX can overlap; master/music/SFX volumes; `Duck(bool)` for Pause/Editor.
- `AudioSource` (sound + volume + pitch, optional 3D attenuation by distance to the camera). A standalone class in this phase; it becomes a `Component` in Phase 5 (decision D10).
- `MusicPlayer`: `LoadMusicStream`, looping, `UpdateMusicStream` each frame, cross‑fade / fade‑in (Unity: max 0.2, speed 0.05/s).

**Done when:** a key plays a pitched SFX and music fades in, and pausing ducks the music (a demo key until Phase 8's Pause calls the same `Duck()`).

### Phase 5 — Scene: GameActor, Component, Level
- `GameActor`: name, `::Transform`, parent/children, `IsActive`, `MarkForDestroy`, `GetLocalMatrix()` / `GetWorldMatrix()` (`MatrixScale * QuaternionToMatrix * MatrixTranslate`, times the parent's).
- `Component`: `owner`, `OnCreate`, `OnEnable/OnDisable`, `Update`, `FixedUpdate`, `OnTriggerEnter/Exit`, `DrawInspector` (editor), `Serialize/Deserialize`.
- `GameActor::AddComponent<T>() / GetComponent<T>()` (`requires std::derived_from<T, Component>`).
- `Level`: `CreateGameActor<T>(name, parent)` (as in the reference), `Update`, `FixedUpdate`, `SubmitRenderables`, `FlushDestroyed`, `Clear`, `FindByName`.
- `MeshRenderer` component (model + material → `RenderCommand`).
- `AudioSource` (Phase 4) becomes a `Component`, positioned from its actor.

**Done when:** a parent actor rotates and its children follow, and destroying an actor mid‑update doesn't crash.

### Phase 6 — Physics (Bullet 3.25)
Bullet is hidden behind `eng::` classes. Public headers forward‑declare `class btRigidBody;` etc. or use pimpl, so `eng.h` never includes Bullet.

- **`BulletConvert`** (internal): `ToBt(Vector3)`, `ToBt(Quaternion)` (raylib `{x,y,z,w}` ↔ `btQuaternion(x,y,z,w)`), `ToBt(::Transform)` → `btTransform`, and back. Both libraries are right‑handed, Y‑up, in meters, so no axis swap is needed.
- **`PhysicsWorld`** (pimpl):
  - `Init()`: `btDefaultCollisionConfiguration` → `btCollisionDispatcher` → `btDbvtBroadphase` (+ `getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback)`) → `btSequentialImpulseConstraintSolver` → `btDiscreteDynamicsWorld`; `setGravity(world.gravity)` (−9.81).
  - `Step(dt)`: `stepSimulation(dt, 1, dt)`. The engine owns the accumulator, so `FixedUpdate` and Bullet ticks stay in lockstep.
  - `Raycast(origin, dir, maxDist, mask) -> std::optional<RaycastHit{actor, collider, point, normal, distance}>` via `btCollisionWorld::ClosestRayResultCallback` (filter by `m_collisionFilterMask`).
  - `OverlapSphere / OverlapBox` via a temporary `btPairCachingGhostObject` or `contactTest`.
  - `DispatchTriggers()`: for each `TriggerVolume`, read `getOverlappingPairs()` and diff against last tick's set → `OnTriggerEnter / OnTriggerExit` on both actors' components.
  - `SetDebugDrawEnabled(bool)`, `DebugDraw()` → `debugDrawWorld()` through `PhysicsDebugDrawer`.
  - `Shutdown()`: asserts the world is empty, then deletes in reverse creation order.
- **`PhysicsLayers`**: collision groups/masks as bit flags (`Ground=1, Prop=2, Player=4, Trigger=8, Beam=16`), passed to `addRigidBody(body, group, mask)` / `addCollisionObject`.
- **`Collider`** component: shape description `Box{halfExtents} | Sphere{r} | Capsule{r, h} | ConvexHull{model} | StaticMesh{model}`; `CreateShape()` builds a **per‑actor** `btCollisionShape` (`btBoxShape`, `btSphereShape`, `btCapsuleShape`, `btConvexHullShape`, `btBvhTriangleMeshShape` for static level geometry). Shapes are not shared, so scaling one prop never scales another. `SetScale(Vector3)` → `shape->setLocalScaling()`.
- **`RigidBody`** component (requires `Collider`): `BodyType {Static, Dynamic, Kinematic}`, mass, friction, restitution, damping, `lockRotation` (angular factor 0). Builds a `btRigidBody` with an `ActorMotionState`. Kinematic bodies get `CF_KINEMATIC_OBJECT` + `DISABLE_DEACTIVATION`. API: `AddImpulse`, `SetLinearVelocity`, `Teleport(::Transform)` (sets the transform, clears velocities, `activate(true)`), `OnScaleChanged()` (recompute inertia with `calculateLocalInertia`, `setMassProps`, `updateInertiaTensor`, `updateSingleAabb`, `activate`).
- **`ActorMotionState : btMotionState`**: `getWorldTransform` reads the actor's world transform; `setWorldTransform` writes position and rotation back into the actor (dynamic bodies only). Scale is **never** taken from Bullet; it lives on the actor and is pushed through `setLocalScaling`.
- **`TriggerVolume`** component (requires `Collider`): `btPairCachingGhostObject` with `CF_NO_CONTACT_RESPONSE`, group `Trigger`, mask `Player | Prop`.
- **`CharacterMotor`** component (uses the actor's capsule `Collider`, layer Player): the player is a **dynamic capsule rigid body** (`btCapsuleShape` r 0.5, height 1.0, mass 80), not Bullet's kinematic controller (decision D7):
  - Body setup: `setAngularFactor(0)` (never tips over), friction 0, restitution 0, `DISABLE_DEACTIVATION`, CCD on, group `Player`, mask `Ground | Prop | Trigger`.
  - Own gravity: the `BT_DISABLE_WORLD_GRAVITY` flag is set before `addRigidBody`, then `setGravity({0, -g, 0})`, so the player can use the snappy jump‑arc gravity while props keep −9.81.
  - Each fixed tick, `SetWalkVelocity(v)` sets the linear velocity to `(v.x, current.y, v.z)`. On the ground, `v` is projected onto the ground plane so slopes don't make the player bounce; in the air, an `airControl` factor (0.5) blends toward `v`.
  - `IsGrounded()`: a `convexSweepTest` with a sphere (0.95·r) from the capsule's bottom sphere down 0.1 m, mask `Ground | Prop`. Ground counts only if `normal.y >= cos(maxSlope = 45°)`. It stores `groundNormal` and `groundBody`.
  - Grounded and not jumping: `vy = -0.05` (Unity grounded gravity) to stay glued on down‑slopes. Standing on a moving dynamic crate adds the `groundBody`'s velocity.
  - `Jump(v0)`: only if grounded; sets `vy = v0`; grounding is ignored for 0.1 s afterwards so the sweep doesn't snap the player back down.
  - `Warp(position)`: sets the body transform + motion state, clears velocities.
  - No automatic step‑up: the rounded capsule bottom climbs edges up to ~0.2 m. Higher steps are ramps in level design.
  - Free physical side effects: the player pushes crates (mass ratio), and a crate **stretched under the player lifts them**, which the puzzles can use.
- **`PhysicsDebugDrawer : btIDebugDraw`**: `drawLine` → `DrawLine3D`, `drawContactPoint` → small sphere; used by the editor and a Debug‑build toggle (F3).
- Fixed‑step loop wired into `Engine::Run` (`FIXED_DT = 0.02`, Unity default) as in §4.

**Done when:** a sandbox level (plane + 20 dynamic crates + a ramp) runs at 60 FPS. Crates fall, stack and settle (go to sleep); the `CharacterMotor` capsule walks up the 30° ramp without sliding back, stops at a 50° slope, jumps onto a crate, and pushes a light crate; a center‑screen ray reports the crate it hits; a `TriggerVolume` fires Enter/Exit exactly once each; F3 shows Bullet wireframes. Deleting a crate mid‑simulation doesn't crash (removed before deleted).

### Phase 7 — FX: particles and lines
- `ParticleEmitter` (burst count, lifetime, speed, spread cone, size, **color gradient over lifetime with alpha fade**) and `ParticleSystem` (pooled, max ~1024, drawn as billboards or small cubes).
- `LineRenderer` (start/end points, start/end colors, start/end width, fade‑out), submitted as a `LineCommand` to the RenderQueue's transparent line pass: an open tapered tube with per‑vertex colors (capped `DrawCylinderEx` segments showed stacked caps through the transparency).

**Done when:** clicking spawns a colored burst and a fading beam line.

### Phase 8 — Events, modes, menus (Main Menu / Pause / Resume)
- `EventBus`: `Subscribe<T>(fn) -> token`, `Unsubscribe`, `Publish<T>` (queued), `Dispatch()` at end of frame.
- `ModeMachine`: `EngineMode { MainMenu, Play, Paused, Editor }`, `Request(mode)`, guarded transitions; on transition it sets the cursor lock, ducks audio, and calls `Application` hooks.
- `UIStack` + `UIScreen` (raygui): `MainMenuScreen` (Start, Editor*, Quit), `PauseMenuScreen` (Resume, Restart Level, Main Menu, Quit), generic `MessageScreen` (title + N buttons, used by the game for win/lose). `UIStyle` loads a raygui style and font.
- Auto‑pause when `!IsWindowFocused()`.

**Done when:** Boot → Main Menu → Start → Play → ESC pauses (world frozen, cursor free) → Resume returns to Play exactly where it was → Main Menu → Quit.

### Phase 9 — Editor mode (enter / exit at runtime)
- `Editor` (only when `ENG_EDITOR`): **TAB** in Play enters Editor and TAB again exits. On enter it takes a `LevelSnapshot`, freezes the simulation, frees the cursor, and copies the FPS camera pose into `EditorCamera`.
- `EditorCamera`: hold RMB + WASD/QE to fly (`UpdateCameraPro`), wheel changes fly speed, `F` focuses the selection.
- `Picker`: LMB → `GetScreenToWorldRay` → `PhysicsWorld::Raycast` first (exact, rotated shapes), then falls back to render bounds for actors without a collider.
- Editing physics actors: the gizmo and inspector write the actor transform and call `RigidBody::Teleport()` (or `CharacterMotor::Warp()`); scale edits call `Collider::SetScale()` + `RigidBody::OnScaleChanged()`. `LevelSnapshot::Restore()` re‑creates bodies, so velocities are reset too.
- `TransformGizmo`: X/Y/Z axis handles; drag projects the mouse ray onto the axis. Modes **W move / R scale**; rotation is edited in the inspector (euler degrees).
- `HierarchyPanel`: tree of actors, select, rename, **Add Cube / Add Trigger / Duplicate / Delete**.
- `InspectorPanel`: transform fields + each component's `DrawInspector()` (e.g., `Scalable.speed`, trigger type).
- `EditorToolbar`: **Play** (exit editor, keep edits), **Reset** (restore snapshot), **Save** (Ctrl+S), **Load**.
- `LevelSerializer` + `ComponentRegistry` (string → factory; the game registers its components in `ScaleGame::Init`). Text format:
  ```
  level "Level One"
  spawn 0 1 0  yaw 0
  actor Crate_01
    transform  0 0.5 6   0 0 0   1 1 1          # position | euler deg | scale
    component MeshRenderer  model=cube  tint=200,120,50
    component Collider      shape=box half=0.5,0.5,0.5 layer=Prop
    component RigidBody     type=dynamic mass=2 friction=0.8
    component Scalable      speed=5
  end
  ```
- Editor overlay: `DrawGrid`, Bullet debug wireframes (`PhysicsWorld::DebugDraw`), trigger volumes (translucent), selection highlight, actor labels.

**Done when:** enter the editor mid‑game, move/scale a crate, add a trigger, save `level_01.lvl`, exit to Play with the edits live; restarting the app and loading the file reproduces the level.

### Phase 10 — Game: state and player
- `ActorHandle` (decision D11): id‑based weak reference to a `GameActor` (Level lookup, null once deleted). Gameplay code stores handles, not raw `GameActor*`.
- `PlayerState` (from `PlayerState.cs`): walk speed 10, scale factors `{0.25, 0.5, 0.75}` (rate 0.25 × 3), index, `ChangeFactor(±1)`, cooldown 1.0 s, `readyToShoot`, ray reach 30.
- `GameState` (from `GameState.cs`): `Reset`, `StartGame`, `WinGame`, `LoseGame`, `ProgressToNextLevel`; publishes `GameStarted / GameWon / GameLost` on the `EventBus`.
- `PlayerInput` (from `InputManager.cs`): reads the engine `InputMap`, exposes `Direction`, `Jump`, `Look`; `giveControl` flag; LMB → `gun.Fire(+1)`, RMB → `gun.Fire(-1)`, wheel → `playerState.ChangeFactor(sign)`.
- `PlayerActor`: actor with `CharacterMotor` (layer `Player`), `FirstPersonController`, `FirstPersonLook`, `ScalingGun`, child `GunView`.
- `FirstPersonController` (from `FirstPersonController.cs`): in `FixedUpdate`, camera‑relative XZ velocity × walk speed → `CharacterMotor::SetWalkVelocity`. The jump arc sets the capsule's own gravity: `t_apex = maxJumpTime/2`, `g = 2h/t_apex²` → `SetGravity(32)`, `v0 = 2h/t_apex` → `Jump(8)` when grounded (h = 1, t = 0.5). The ground check is `IsGrounded()` (sphere sweep), replacing the Unity `CheckSphere`.
- `FirstPersonLook`: yaw on the body, pitch on the camera, clamped ±89°, sensitivity.
- Player spawn read from the `.lvl` `spawn` line.

**Done when:** the player walks, jumps, and looks around in `level_01`, and the wheel cycles x0.25 → x0.5 → x0.75 (printed on screen).

### Phase 11 — Game: the scaling mechanic
- `ScalingGun` (from `ScalingProvider.cs`): cooldown ticked in `FixedUpdate`; `Fire(sign)` → returns if not ready; ray from the **screen center** (`camera.position`, `camera.forward`, reach 30, mask `Ground | Prop`, so the player capsule and triggers are ignored) → `PhysicsWorld::Raycast`; on hit → `RayBeamSpawner.Spawn(sign, hitPoint)`; if the hit actor has `Scalable` → `TriggerScaling(1 + factor * sign)`. Optional juice: a small `RigidBody::AddImpulse` along the beam.
- `Scalable` (from `ScalingController.cs` + `ScalingEnabler.cs` + `ScalingAudioFX.cs`): ignore if already scaling; block stretch when `scale.z >= 1.9`; `FixedUpdate`: `t += speed * fixedDt`, `scale = startScale * SmoothStep(1, target, t)`. Each tick it writes the actor scale, then `Collider::SetScale(scale)` (`setLocalScaling`) + `RigidBody::OnScaleChanged()` (mass scales with volume if `scaleMass=true`; inertia recomputed; body woken). At `t >= 1` it stops, and **destroys the actor if `|scale| < |(0.1,0.1,0.1)|`** (body removed from the world first). It plays the stretch/shrink SFX with pitch `factor ± 0.02`. Bullet scales shapes around the body origin, so a Scalable prop's box is wrapped in a `btCompoundShape` with the child offset up by its half height. The compound's `setLocalScaling` also scales that offset, so **the bottom face stays on the floor** and objects grow upward instead of into it.
- `RayBeamSpawner` (from `RayBeamProvider.cs`): muzzle position = camera + right·0.3 − up·0.25 + forward·0.6; spawns `MuzzleFX` and a `RayBeam` actor.
- `RayBeam` (from `RayBeamController.cs` + `BeamAudioFX.cs` + `BeamImpactParticle.cs`): `LineRenderer` from tail → hit point; tail advances at 20 u/s and the beam self‑destroys when the tail reaches the hit point; colors **stretch = blue→green**, **shrink = yellow→red**; impact particle burst (blue or yellow gradient, alpha 1→0); beam SFX at volume 0.3.
- `MuzzleFX` (from `ExitParticlesController.cs`): sparks + wave bursts at the muzzle, tinted by sign.

**Done when:** left click grows a crate smoothly (capped), right click shrinks it until it disappears, beams and particles match the colors, and the cooldown blocks spam.

### Phase 12 — Game: level flow, UI, music
- `TriggerZone` base component (sits on an actor with `Collider` + engine `TriggerVolume`; filters for the `Player` layer in `OnTriggerEnter`), with `WinTrigger` (`GameState.WinGame`), `LoseTrigger` (kill volume under the map → `LoseGame`), and `NextLevelTrigger` (`GameplayManager.LoadNextLevel`).
- `GameplayManager` (from `GamePlayManager.cs`): `LevelList` (from `SceneRef.cs`) of `.lvl` paths, `LoadLevel(i)`, `LoadNextLevel()`, `ReloadCurrentLevel()`; resets `GameState` on every load; starts the game automatically on levels after the first.
- `GameUI` (from `GUIManager.cs`): subscribes to events; on `GameWon` it pushes `MessageScreen("Level Complete", Next | Main Menu)`, on `GameLost` it pushes `MessageScreen("You fell!", Retry | Main Menu)`; it calls `PlayerInput.SetControl(false)` and frees the cursor while shown.
- `MusicManager` (from `ThemeMusicManager.cs`): `MENU` and `GAME_PLAY` themes, switches on mode change, fade‑in. Keep the original music credits in `assets/audio/CREDITS.txt`.
- Author 2–3 levels in the editor (a gap to bridge by stretching a plank, a wall to shrink, a crate staircase).

**Done when:** a full run works: Menu → Level 1 → win → Level 2 → fall → retry → win → back to Menu.

### Phase 13 — Polish and release
- `GunView` (gun model drawn in a second `BeginMode3D` pass with a fixed camera so it never clips into walls) and `GunHUD` (from `GunDiegeticHUD.cs`): the `"x0.25"` text rendered into a small `RenderTarget` mapped onto the gun's screen material; fallback is plain HUD text. Crosshair and cooldown ring in `DrawHUD`.
- Settings in the pause menu: mouse sensitivity, volumes, fullscreen (`ToggleBorderlessWindowed`).
- Release build with `ENG_EDITOR=0`; check that the static binary runs on a clean Mac with the `assets/` folder next to it; update README (controls, build, editor shortcuts).

---

## 6. Key interfaces (sketch)

```cpp
namespace eng {

class Application {
public:
    virtual ~Application() = default;
    virtual bool Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void FixedUpdate(float fixedDt) {}
    virtual void SubmitRenderables(RenderQueue&) {} // every frame, also while paused (as built in Phase 3)
    virtual void UpdateFrozen(float unscaledDt) {}  // instead of Update outside Play (Phase 8, planned)
    virtual void DrawHUD() {}                       // 2D, after the 3D pass
    virtual EngineMode GetStartMode() const { return EngineMode::MainMenu; }
    virtual void Destroy() = 0;

    // Mode hooks (called by ModeMachine)
    virtual void OnStartRequested() {}              // Main menu "Start"
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnEnterEditor() {}
    virtual void OnExitEditor() {}
    virtual void OnReturnToMainMenu() {}
    virtual void OnRestartRequested() {}

    virtual const char* GetTitle() const { return "Raylib Engine"; }
    virtual Level*      GetActiveLevel() { return nullptr; }   // used by Editor + renderer

    void SetNeedsToBeClosed(bool value);
    bool NeedsToBeClosed() const;
private:
    bool m_needsToBeClosed = false;
};

class Engine {
public:
    static Engine& GetInstance();
    bool  Init(int width = 1280, int height = 720);
    void  Run();
    void  Shutdown();
    void  SetApplication(Application* app);       // ownership transfer, as in the reference

    Application*     GetApplication() const;
    InputManager&    GetInputManager();
    ResourceManager& GetResources();
    RenderQueue&     GetRenderQueue();
    ActiveCamera&    GetMainCamera();
    PhysicsWorld&    GetPhysics();
    AudioManager&    GetAudio();
    ParticleSystem&  GetParticles();
    UIStack&         GetUI();
    ModeMachine&     GetModes();
    EventBus&        GetEvents();
    const Time&      GetTime() const;
    const World&     GetWorld() const;
    const WindowInfo& GetWindow() const;
private:
    Engine() = default;  ~Engine();
    Engine(const Engine&) = delete;  Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;       Engine& operator=(Engine&&) = delete;
    // ... subsystems by value ...
};

class Component {
public:
    virtual ~Component() = default;
    virtual void OnCreate() {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void Update(float dt) {}
    virtual void FixedUpdate(float fixedDt) {}
    virtual void OnTriggerEnter(GameActor& other) {}
    virtual void OnTriggerExit(GameActor& other) {}
    virtual void DrawInspector() {}                 // raygui, editor only
    virtual void Serialize(LevelWriter&) const {}
    virtual void Deserialize(const LevelReader&) {}
    virtual const char* TypeName() const = 0;
    GameActor& Owner();
};

// ---- physics/PhysicsWorld.h  (no Bullet include: pimpl) ----
struct RaycastHit { GameActor* actor; Collider* collider; Vector3 point; Vector3 normal; float distance; };

class PhysicsWorld {
public:
    PhysicsWorld();  ~PhysicsWorld();                       // defined in .cpp (pimpl)
    bool Init(Vector3 gravity);
    void Step(float fixedDt);                               // stepSimulation(fixedDt, 1, fixedDt)
    void DispatchTriggers();
    std::optional<RaycastHit> Raycast(Vector3 origin, Vector3 dir, float maxDist,
                                      uint32_t mask = Layer::All) const;
    int  OverlapBox(Vector3 center, Vector3 halfExtents, Quaternion rot,
                    uint32_t mask, std::vector<GameActor*>& out) const;
    void SetDebugDrawEnabled(bool on);
    void DebugDraw() const;                                 // inside BeginMode3D
    void Shutdown();
private:
    friend class RigidBody; friend class TriggerVolume; friend class CharacterMotor;
    struct Impl;  std::unique_ptr<Impl> m_impl;             // owns all bt* world objects
};

} // namespace eng
```

---

## 7. Engine classes

| # | Class | Folder | Responsibility | Origin |
|---|---|---|---|---|
| 1 | `Engine` | core | Singleton; owns the Application and subsystems; `Init/Run/Shutdown`; the game loop | Reference `Engine` |
| 2 | `Application` | core | Abstract game interface + mode hooks | Reference `Application` |
| 3 | `Time` | core | dt, clamped dt, elapsed, fixed‑step accumulator, time scale | Reference `UpdateDeltaTime` |
| 4 | `WindowInfo` | core | Screen/render size, high‑DPI scale, resize flag | Reference `Window` struct |
| 5 | `World` | core | World up/right/forward, gravity, sky color | Reference `World` struct |
| 6 | `EventBus` | core | Typed pub/sub with queued dispatch | New (replaces SO listeners) |
| 7 | `Log` | core | Leveled logging over `TraceLog` | New |
| 8 | `EngineMode` (enum) | modes | `MainMenu, Play, Paused, Editor` | New |
| 9 | `ModeMachine` | modes | Guarded transitions, cursor, audio ducking, app hooks | Reference `SetPlayMode` → generalized |
| 10 | `InputManager` | input | Poll raylib; key/button down/pressed/released; mouse pos/delta/wheel; cursor lock | Reference `InputManager` |
| 11 | `InputMap` | input | Named actions/axes → bindings | New |
| 12 | `ResourceManager` | resources | Cache + lifetime of models, shaders, textures, sounds, music, fonts; primitives | Replaces `GraphicsAPI` creation + `Model` helper |
| 13 | `Camera` | render | Wraps `Camera3D`; basis vectors; yaw/pitch; perspective params | Reference `Camera` |
| 14 | `ActiveCamera` | render | Points at the camera used for rendering this frame | Reference `ActiveCamera` |
| 15 | `Material` | render | `::Material` + tint + shader params map | Reference `Material` |
| 16 | `MeshRenderer` | render | Component: model/mesh + material → `RenderCommand`; bounds for picking | Reference `Mesh` usage |
| 17 | `RenderCommand` | render | `{mesh, material, Matrix}` | Reference `RenderCommand` |
| 18 | `RenderQueue` | render | Collect + flush draw commands inside `BeginMode3D` | Reference `RenderQueue` |
| 19 | `RenderTarget` | render | `RenderTexture2D` wrapper with resize | Reference `FrameBuffer` |
| 20 | `LineRenderer` | render | Component: thick gradient line between two points | New (Unity `LineRenderer`) |
| 21 | `DebugDraw` | render | Immediate debug shapes and labels (Debug/Editor) | New |
| 22 | `GameActor` | level | Name, `::Transform`, hierarchy, components, active, destroy flag | Reference `GameActor` |
| 23 | `Component` | level | Base behavior/data unit attached to an actor | New (Unity `MonoBehaviour`) |
| 24 | `Level` | level | Owns actors; factory; update/fixed/submit; deferred destroy | Reference `Level` |
| 25 | `LevelSerializer` (+`LevelReader/Writer`) | level | Save/load `.lvl` text files | New (Unity scenes) |
| 26 | `ComponentRegistry` | level | `TypeName` → factory, so the game's components can be loaded from files | New |
| 27 | `PhysicsWorld` | physics | Pimpl over `btDiscreteDynamicsWorld`: init/shutdown, fixed `Step`, raycast, overlaps, trigger dispatch, debug draw | New (Bullet) |
| 28 | `PhysicsLayers` | physics | Collision group/mask bit flags | New (Unity layers) |
| 29 | `Collider` | physics | Component: shape description → per‑actor `btCollisionShape` (box/sphere/capsule/convex hull/static mesh, compound for bottom pivot); `SetScale` | New (Unity `Collider`) |
| 30 | `RigidBody` | physics | Component: static/dynamic/kinematic `btRigidBody`; mass, friction, impulses, `Teleport`, `OnScaleChanged` | New (Unity `Rigidbody`) |
| 31 | `ActorMotionState` | physics | `btMotionState` bridging `btTransform` ↔ actor `::Transform` | New (Bullet glue) |
| 32 | `TriggerVolume` | physics | Component: `btPairCachingGhostObject`, no contact response; overlap set per tick | New (Unity `isTrigger`) |
| 33 | `CharacterMotor` | physics | Component: dynamic capsule `btRigidBody` (no rotation, own gravity); walk velocity, sphere‑sweep grounding, slope limit, jump, warp | New (Unity `CharacterController`) |
| 34 | `RaycastHit` | physics | Hit actor, collider, point, normal, distance | New (Unity `RaycastHit`) |
| 35 | `PhysicsDebugDrawer` | physics | `btIDebugDraw` → raylib `DrawLine3D` | New (Bullet glue) |
| 36 | `BulletConvert` | physics | Inline converters raylib ↔ Bullet math types (internal header) | New (Bullet glue) |
| 37 | `AudioManager` | audio | SFX voice pool (aliases), volumes, ducking | New |
| 38 | `AudioSource` | audio | Component: sound + volume + pitch (+ distance attenuation) | New (Unity `AudioSource`) |
| 39 | `MusicPlayer` | audio | Streaming music, loop, fade/cross‑fade | New |
| 40 | `ParticleEmitter` | fx | Burst config + color/alpha gradient over lifetime | New (Unity `ParticleSystem`) |
| 41 | `ParticleSystem` | fx | Pooled simulation + drawing of all particles | New |
| 42 | `UIScreen` | ui | Abstract raygui screen: `Update`, `Draw`, `BlocksInput` | New |
| 43 | `UIStack` | ui | Push/pop screens; draws the top screen (and below if transparent) | New |
| 44 | `MainMenuScreen` | ui | Start / Editor / Quit | New (required feature) |
| 45 | `PauseMenuScreen` | ui | Resume / Restart / Main Menu / Quit (+ settings) | New (required feature) |
| 46 | `MessageScreen` | ui | Generic title + buttons (win/lose dialogs) | New |
| 47 | `UIStyle` | ui | raygui style + font loading; `RayGuiImpl.cpp` holds `RAYGUI_IMPLEMENTATION` | Replaces ImGui init |
| 48 | `Editor` | editor | Editor-mode coordinator: enter/exit, selection, shortcuts, overlays | Reference play‑mode toggle → full editor |
| 49 | `EditorCamera` | editor | Free‑fly camera (RMB + WASD), focus | Current `main.cpp` free camera |
| 50 | `Picker` | editor | Mouse ray → closest actor by bounds | New |
| 51 | `TransformGizmo` | editor | Axis handles; move/scale dragging | New |
| 52 | `HierarchyPanel` | editor | Actor tree; add/duplicate/delete/rename | New |
| 53 | `InspectorPanel` | editor | Transform + component `DrawInspector()` | New |
| 54 | `EditorToolbar` | editor | Play / Reset / Save / Load | New |
| 55 | `LevelSnapshot` | editor | In‑memory serialized level used by Reset | New |

---

**Added during implementation** (see the devlogs): `MathUtils::DecomposeTransform` (replaces raymath's row‑based `MatrixDecompose`, wrong with non‑uniform scale), `RaycastHit`, `ActorMotionState`, `PhysicsDebugDrawer`, `CcdMode` (Auto below 0.3 m), `SceneLighting`, `LineCommand`, `ModeChangedEvent`, `EditorBannerScreen` (placeholder until Phase 9), `ParticleEmitterSettings`. Sounds and music load through `ResourceManager`; `AudioSource` is a `Component` since Phase 5. **Phases 8–13:** `Editor` (+ `EditorCamera`, `Picker`, `TransformGizmo`, `HierarchyPanel`, `InspectorPanel` / `InspectorLayout`, `EditorToolbar`, `ActorEdit`, `LevelSnapshot`), `LevelSerializer`, `PropertyBag`, `ComponentRegistry`, `SpawnPoint`; `ActorHandle`; game: `GameContext`, `GameplayManager`, `TriggerZone` + `WinTrigger / LoseTrigger / NextLevelTrigger`, `GameUI`, `GunHUD`, `GunView`, `MusicManager`, `Settings` + `SettingsScreen`, `ScalingShotEvent`; engine hooks `Application::DrawViewModel`, `HasSettings / OnSettingsRequested`, `GetLevelPath`, `OnLevelLoaded`. Not built as planned: `LevelWriter / LevelReader` (one `PropertyBag` does both), `BeamImpactParticle` (inside `RayBeam`), `SceneRef` (`LevelList`). Level files never contain the player; the game spawns it at the level's spawn point.

---

## 8. Game classes (Application layer)

| # | Class | Folder | Responsibility | Unity origin |
|---|---|---|---|---|
| 1 | `ScaleGame` | game | `eng::Application`; registers components, owns `Level` + managers, implements mode hooks, `DrawHUD` | — (reference `Game`) |
| 2 | `GameState` | state | Started/Won/Lost/LevelCompleted; `Reset/StartGame/WinGame/LoseGame`; publishes events | `GameState.cs` |
| 3 | `PlayerState` | state | Walk speed, scale factors + index, `ChangeFactor`, cooldown, ready flag, reach | `PlayerState.cs` |
| 4 | `SoundFXBank` | state | Handles for shrink/stretch and shrink‑beam/stretch‑beam sounds | `SoundFX.cs` |
| 5 | `LevelList` | state | Ordered list of `.lvl` paths, current index | `SceneRef.cs` |
| 6 | `GameplayManager` | managers | Load / next / reload level, reset `GameState`, spawn player | `GamePlayManager.cs` |
| 7 | `GameUI` | managers | Win/lose `MessageScreen`s in response to events | `GUIManager.cs`, `Win/Lose/GameStartListener.cs` |
| 8 | `MusicManager` | managers | Menu vs gameplay theme, fade‑in | `ThemeMusicManager.cs` |
| 9 | `PlayerInput` | managers | Actions → movement / jump / look / fire / alt‑fire / scroll; control gate | `InputManager.cs` |
| 10 | `PlayerActor` | player | Builds the player: motor + controller + look + gun + gun view | Player prefab |
| 11 | `FirstPersonController` | player | Camera‑relative movement, jump arc gravity, grounded handling | `FirstPersonController.cs` |
| 12 | `FirstPersonLook` | player | Yaw/pitch from mouse delta, clamp, sensitivity | Cinemachine POV |
| 13 | `GunView` | player | Draws the gun viewmodel in a separate pass; provides the muzzle point | Gun prefab |
| 14 | `GunHUD` | player | `"x0.25"` factor display (on‑gun RenderTarget or HUD) + crosshair + cooldown | `GunDiegeticHUD.cs` |
| 15 | `ScalingGun` | scaling | Cooldown, center‑screen raycast, spawn beam, trigger `Scalable` | `ScalingProvider.cs` |
| 16 | `Scalable` | scaling | SmoothStep scale animation, 1.9 cap, min‑size destroy, pitched SFX | `ScalingController.cs`, `ScalingEnabler.cs`, `ScalingAudioFX.cs` |
| 17 | `RayBeamSpawner` | scaling | Spawn `RayBeam` + `MuzzleFX` at the muzzle | `RayBeamProvider.cs` |
| 18 | `RayBeam` | scaling | Animated gradient beam tail → hit, impact burst, beam SFX, self‑destroy | `RayBeamController.cs`, `BeamAudioFX.cs`, `BeamImpactParticle.cs` |
| 19 | `MuzzleFX` | scaling | Sparks + waves burst tinted by stretch/shrink | `ExitParticlesController.cs` |
| 20 | `TriggerZone` | triggers | Base: player‑only `OnTriggerEnter` → `OnPlayerEnter()` | — |
| 21 | `WinTrigger` | triggers | `GameState.WinGame()` | `WinningTriggerController.cs` |
| 22 | `LoseTrigger` | triggers | `GameState.LoseGame()` (kill volume) | `LosingTriggerController.cs` |
| 23 | `NextLevelTrigger` | triggers | `GameplayManager.LoadNextLevel()` | `NextSceneTriggerController.cs` |

Props (crates, planks, walls) are **not classes**. They are data in `.lvl` files: `MeshRenderer` + `Collider` + `RigidBody` (static for level geometry, dynamic for crates) + `Scalable` when they react to the gun.

### Game controls

| Input | Action |
|---|---|
| WASD | Move |
| Mouse | Look |
| Space | Jump |
| **Left click** | Stretch (scale up by `1 + factor`) |
| **Right click** | Shrink (scale down by `1 − factor`) |
| **Mouse wheel** | Change factor x0.25 / x0.5 / x0.75 |
| ESC | Pause / Resume |
| TAB | Enter / exit Editor (dev builds) |

---

## 9. Main runtime flows

**Fire (left click):**
```
InputManager.Poll ─> PlayerInput("Fire" pressed, giveControl) ─> ScalingGun.Fire(+1)
   ├─ !readyToShoot ? return
   ├─ readyToShoot = false
   ├─ PhysicsWorld.Raycast(camera.pos, camera.forward, 30, Prop|Ground)
   │     └─ hit ─> RayBeamSpawner.Spawn(+1, hit.point) ─> MuzzleFX burst (blue) + RayBeam actor (blue→green, SFX)
   │           └─ hit.actor.GetComponent<Scalable>() ─> TriggerScaling(1 + PlayerState.CurrentFactor())
   └─ FixedUpdate: cooldown -= fixedDt; <= 0 ─> readyToShoot = true
Scalable.FixedUpdate: t += speed*dt; scale = start * SmoothStep(1, target, t)
   ─> Collider.SetScale (btCompoundShape::setLocalScaling) ─> RigidBody.OnScaleChanged (inertia, AABB, wake)
   t>=1 ─> done / destroy if tiny (removeRigidBody, then delete)
RayBeam.Update: tail += 20*dt; tail >= length ─> MarkForDestroy ─> Level.FlushDestroyed (end of frame)
```

**Win:**
```
PhysicsWorld.DispatchTriggers ─> WinTrigger.OnTriggerEnter(player) ─> GameState.WinGame()
   ─> EventBus.Publish(GameWon) ─> [end of frame] GameUI ─> UIStack.Push(MessageScreen "Level Complete")
                                               PlayerInput.SetControl(false), cursor free
   "Next" ─> GameplayManager.LoadNextLevel() ─> Level.Clear + LevelSerializer.Load ─> GameState.StartGame()
```

**Enter / exit editor:**
```
TAB (Play) ─> ModeMachine.Request(Editor) ─> LevelSnapshot.Capture, EnableCursor, audio.Duck, app->OnEnterEditor
           ... edit ...
TAB / "Play" ─> ModeMachine.Request(Play) ─> DisableCursor, audio.Unduck, app->OnExitEditor (edits stay live)
"Reset"      ─> LevelSnapshot.Restore()   ─> level back to the state it had when you entered the editor
```

---

## 10. Risks and notes

- **raylib name collisions**: see §0. Keep `using namespace eng` out of the code; prefix raylib types with `::` inside `eng` when needed.
- **ESC closes raylib windows by default**: call `SetExitKey(KEY_NULL)` in `Engine::Init`, otherwise Pause can't work.
- **Input timing**: raylib polls events inside `EndDrawing()`, so "pressed this frame" refers to the previous `EndDrawing`. Read input once per frame in `InputManager::Poll()` and never inside `FixedUpdate` (it can run 0..N times per frame). Fire requests are latched and consumed by the next fixed step if needed.
- **Bullet build flags must match**: the engine and the prebuilt libs must agree on `BT_USE_DOUBLE_PRECISION` (OFF) and architecture (arm64), or you get silent memory corruption, not link errors. `build_bullet.sh` pins both; do not define `BT_USE_DOUBLE_PRECISION` anywhere in the engine.
- **raymath `MatrixDecompose` reads matrix rows**: wrong rotation/scale for non‑uniformly scaled, rotated actors. Always use `eng::DecomposeTransform` (found in Phase 6).
- **macOS toolchain**: an Xcode linker older than the Command Line Tools SDK fails with `tapi error: malformed file`; use the CLT toolchain or update Xcode (see README).
- **Raw `GameActor*` in game code** can dangle after a late destroy (ASan, Phase 6) → `ActorHandle` in Phase 10.
- **Dynamic capsule player**: the usual problems are sliding down slopes, bouncing on slope changes, sticking to walls in mid‑air, and being launched by props that are scaling. Mitigations: friction 0 + velocity projected onto the ground plane, `vy = -0.05` glue while grounded, zero friction on walls, clamp the player's max vertical velocity after each step (e.g. 15 m/s), and a jump grace period. If it proves unreliable, the fallback is `btKinematicCharacterController` (not chosen; it jitters on slopes and is poorly maintained). That is a change inside `eng::CharacterMotor` only; the game is unaffected.
- **Scaling into the player or into other props**: a fast `setLocalScaling` can create deep penetrations. Bullet then separates bodies with a velocity spike. Keep `Scalable.speed` moderate (5), clamp the max linear velocity after the step, and skip the stretch if an `OverlapBox` at the target size hits a static wall.
- **Shape scaling cost**: `btBvhTriangleMeshShape` (static mesh) must not be scaled at runtime. Scalable props always use primitive or convex hull shapes.
- **Tunnelling**: small, fast props (shrunk crates falling) can pass through thin floors. Enable CCD (`setCcdMotionThreshold`, `setCcdSweptSphereRadius`) on dynamic bodies smaller than 0.3 m.
- **Deletion order**: removing a body from the world before deleting it is mandatory. `Level::FlushDestroyed` runs after the fixed loop, so Bullet never holds a pointer to a dead actor during `stepSimulation`.
- **High‑DPI on M2**: `FLAG_WINDOW_HIGHDPI` makes render size differ from screen size. Use `GetRenderWidth()` for render targets and `GetScreenWidth()` for UI and mouse.
- **Shutdown order**: unload resources before `CloseAudioDevice()` / `CloseWindow()`. Unloading after the GL context is gone crashes.
