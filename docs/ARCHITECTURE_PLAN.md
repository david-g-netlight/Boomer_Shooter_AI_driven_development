# Architecture Plan — Raylib Engine + "Build to Scale" FPS

A smaller game engine built on **raylib 5.6 (static `lib/libraylib.a`)** that has the same structure as the
reference OpenGL engine (`p_Game_Engine_Cpp_OpenGL`, *"The Sleipnir Engine"*). On top of that it adds:

- an **Editor mode** you can enter and leave at runtime,
- a **Main Menu** with Start / Pause / Resume,
- everything the gameplay of **Build to Scale** needs (the GMTK Jam Unity project).

**Status (2026-11-08):** every phase (0–13) is implemented. This plan describes the architecture **as built**; where the
code differs from the first plan it says so, and §11 lists every difference with the reason.

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
| Weak actor references | `ActorHandle` (id lookup), never a raw `GameActor*` across frames (D11) | Props and beams are destroyed at any time |
| Components from files | `ComponentRegistry` (type name → factory); the game registers its own; `PropertyBag` carries the fields (D12) | Levels and the editor's Add Component create components by name |
| Release | Release build is the default, editor off, macOS 11 / arm64, `tools/package_release.sh` | A binary that runs on another Mac |
| Language | C++20 (the reference needs `requires std::derived_from`) | Bump from the current C++17 |

**Naming rule (important with raylib):** raylib defines global C types named `Camera`, `Mesh`, `Material`,
`Model`, `Shader`, `Transform`, `Ray`. Engine types live in `namespace eng`. **Never write `using namespace eng;`**
in headers or game code (the reference `Game.h` does this, and here it would cause ambiguities). Write `eng::Level`
etc., and `::Mesh` when you mean the raylib type. raylib's own `::Transform` (translation + quaternion + scale)
is exactly the reference `Transform`, so the engine reuses it directly.

---

## 1. Repository layout (as built)

`include/`, `lib/`, `src/`, `docs/` stayed where they were. The engine and the game are sub-folders of `src/`.

```
b-RayLib_static_starter_project_MacOS_M2/
├── CMakeLists.txt              # engine (static lib) + my_app (exe) + copy_assets target; macOS 11 / arm64
├── README.md                   # the game: controls, editor shortcuts, build, packaging, layout
├── include/
│   ├── raylib/                 # raylib.h, raymath.h, rlgl.h, raygui.h (only what the engine includes)
│   └── bullet/                 # Bullet 3.25 headers + LICENSE.txt (copied from bullet3/src)
├── lib/
│   ├── libraylib.a             # prebuilt, arm64
│   ├── libBulletDynamics.a     # Release, arm64, single precision, macOS 11
│   ├── libBulletCollision.a
│   └── libLinearMath.a
├── tools/
│   ├── build_bullet.sh             # clone bullet3@3.25 -> build static libs -> copy into lib/ + include/bullet/
│   ├── generate_placeholder_audio.py   # synthesizes the placeholder music and SFX in assets/audio
│   └── package_release.sh          # Release build -> dist/BuildToScale/ (+ zip)
├── assets/                     # copied next to the executable after every build
│   ├── levels/                 #   level_01.lvl, level_02.lvl, level_03.lvl
│   ├── shaders/                #   lit.vs / lit.fs (sun + ambient + specular)
│   ├── audio/                  #   music_menu.wav, music_game.wav, sfx_stretch/shrink/ping.wav, CREDITS.txt
│   └── README.md
├── src/
│   ├── main.cpp                # Engine::SetApplication(new ScaleGame); Init; Run; Shutdown
│   ├── engine/                 # namespace eng  (library, no game knowledge)
│   │   ├── eng.h               # public umbrella header
│   │   ├── core/               # Engine, Application, Time, WindowInfo, World, EventBus, Log,
│   │   │                       # EngineInfo, MathUtils
│   │   ├── modes/              # EngineMode, ModeMachine
│   │   ├── input/              # InputManager, InputMap
│   │   ├── resources/          # ResourceManager
│   │   ├── render/             # Camera, ActiveCamera, Material, MeshRenderer, RenderQueue (+ RenderCommand,
│   │   │                       # LineCommand, SceneLighting), RenderTarget, LineRenderer, DebugDraw
│   │   ├── level/              # GameActor, Component, Level, ActorHandle, PropertyBag, ComponentRegistry,
│   │   │                       # LevelSerializer
│   │   ├── physics/            # PhysicsWorld, Collider, RigidBody, TriggerVolume, CharacterMotor,
│   │   │                       # RaycastHit, PhysicsLayers, BulletConvert, ActorMotionState,
│   │   │                       # PhysicsDebugDrawer, PhysicsWorldImpl   (only these .cpp files include Bullet)
│   │   ├── audio/              # AudioManager, AudioSource, MusicPlayer
│   │   ├── fx/                 # ParticleEmitter, ParticleSystem
│   │   ├── ui/                 # UIScreen, UIStack, MenuScreens (Main menu, Pause menu, Message dialog,
│   │   │                       # editor banner), UIStyle, RayGuiImpl.cpp
│   │   └── editor/             # Editor, EditorCamera, Picker, TransformGizmo, HierarchyPanel,
│   │                           # InspectorPanel, InspectorLayout, EditorToolbar, EditorUI, ActorEdit,
│   │                           # LevelSnapshot
│   └── game/                   # the Application layer ("Build to Scale")
│       ├── ScaleGame.h/.cpp
│       ├── state/              # GameState, PlayerState, Settings, SoundFXBank, LevelList
│       ├── player/             # PlayerInput, PlayerActor, FirstPersonController, FirstPersonLook,
│       │                       # ScalingGun, GunView
│       ├── props/              # Scalable
│       ├── fx/                 # RayBeamSpawner, RayBeam, MuzzleFX
│       ├── flow/               # GameplayManager, GameContext, TriggerZone (+ Win / Lose / NextLevel triggers)
│       ├── ui/                 # GameUI, GunHUD, SettingsScreen
│       └── audio/              # MusicManager
└── docs/
    ├── ARCHITECTURE_PLAN.md    # this file (kept in sync with the code; see §11)
    ├── DEV_PROGRESS.md         # phase checklist, decision log, session log
    ├── devlogs/                # one devlog per phase branch
    └── tasks/                  # the developer's task files
```

Differences from the original layout plan: no `assets/models` or `assets/fonts` (nothing used them); the
game's `managers/`, `scaling/` and `triggers/` folders became `flow/`, `fx/`, `props/`, `ui/` and `audio/`;
the engine gained `level/ActorHandle`, `PropertyBag` and the editor's `InspectorLayout`, `EditorUI`, `ActorEdit`.

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
| `RenderQueue` + `RenderCommand{mesh, material, modelMatrix}` | Same, drawn inside `BeginMode3D` (`RenderQueue::Draw`) with `DrawMesh(mesh, mat, matrix)`, then a transparent line pass | `std::vector` instead of fixed array |
| `FrameBuffer` (color + depth, resize) | `eng::RenderTarget` (`RenderTexture2D`) | Kept as a utility; the shipped game uses none (the sandbox minimap that used it was removed) |
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
| Post‑build copy of debug assets / shaders | `copy_assets` CMake target copies `assets/` | `ChangeDirectory(GetApplicationDirectory())` at init |

---

## 3. Architecture diagram

### 3.1 Layers

```
+=====================================================================================+
|                         APPLICATION LAYER   (src/game, "Build to Scale")             |
|                                                                                     |
|   ScaleGame : eng::Application  (registers components, wires and owns the systems)   |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
|   | state/         |  | flow/          |  | player/         |  | props/ fx/      |  |
|   | GameState      |  | GameplayManager|  | PlayerInput     |  | Scalable        |  |
|   | PlayerState    |  | GameContext    |  | PlayerActor     |  | RayBeamSpawner  |  |
|   | Settings       |  | TriggerZone ->  |  | FirstPerson-    |  | RayBeam         |  |
|   | SoundFXBank    |  |  Win / Lose /  |  |  Controller/Look|  | MuzzleFX        |  |
|   | LevelList      |  |  NextLevel     |  | ScalingGun      |  |                 |  |
|   +----------------+  +----------------+  | GunView         |  +-----------------+  |
|   +---------------------------------+      +-----------------+  +-----------------+  |
|   | ui/ GameUI  GunHUD  SettingsScreen|                         | audio/          |  |
|   +---------------------------------+                          | MusicManager    |  |
|                                                                +-----------------+  |
+==========================================^==========================================+
                                           |  #include <eng.h>   (public API only)
+==========================================|==========================================+
|                               ENGINE   (src/engine, namespace eng)                   |
|                                                                                     |
|   +------------------------------ CORE -----------------------------------------+   |
|   |  Engine (singleton, owns Application, owns every subsystem by value)         |   |
|   |  Time (dt, fixed step)  WindowInfo  World  EventBus  Log  EngineInfo         |   |
|   +------------------------------------------------------------------------------+   |
|   +-------------------------- ModeMachine --------------------------------------+   |
|   |      MainMenu  <-->  Play  <-->  Paused            Play <--TAB--> Editor     |   |
|   +------------------------------------------------------------------------------+   |
|                                                                                     |
|   +---------+ +-----------+ +-----------+ +-------------+ +---------+ +-------+     |
|   | Input   | | Resources | | Render    | | Level       | | Physics | | Audio |     |
|   | Manager | | Manager   | | Camera    | | GameActor   | | World   | | Mgr   |     |
|   | InputMap| | (cache,   | | Material  | | Component   | | RigidBdy| | Source|     |
|   |         | |  unload,  | | RenderQ   | | ActorHandle | | Collider| | Music |     |
|   |         | |  default  | | RTarget   | | Serializer  | | Trigger | |       |     |
|   |         | |  shader)  | | LineRend. | | Registry    | | CharMot.| |       |     |
|   |         | |           | | DebugDraw | | PropertyBag | +----|----+ +-------+     |
|   +---------+ +-----------+ +-----------+ +-------------+ (pimpl, .cpp only)        |
|                                                                                     |
|   +-----------+ +---------------------------+ +---------------------------------+   |
|   | FX        | | UI (raygui)               | | Editor  (ENG_EDITOR=1 builds)   |   |
|   | Particles | | UIStack, Main / Pause     | | EditorCamera, Picker, Gizmo,    |   |
|   |           | | menu, MessageScreen,      | | Hierarchy, Inspector (+ Add     |   |
|   |           | | UIStyle                   | | Component), Toolbar, Snapshot   |   |
|   +-----------+ +---------------------------+ +---------------------------------+   |
+==========================================^==========================================+
                                           |
+==========================================|=========================|================+
|   raylib 5.6 (static libraylib.a)                         |  Bullet 3.25 (static)      |
|   + raymath.h + rlgl.h + raygui.h                         |  libBulletDynamics.a       |
|   rcore: window/input/time | rmodels: mesh/model          |  libBulletCollision.a      |
|   raudio | rtext | rshapes                                |  libLinearMath.a           |
+===========================================================+============================+
|   macOS (Apple silicon, 11+):  Cocoa  IOKit  CoreVideo  OpenGL  GLUT  frameworks      |
+=====================================================================================+
```

The engine never includes anything from `src/game`. The game reaches the engine only through `eng.h`, and reaches
Bullet never. The engine gives the game five kinds of hooks: `Application` virtuals (`Update`, `FixedUpdate`,
`DrawViewModel`, `DrawHUD`, mode hooks, `GetLevelPath`, `OnLevelLoaded`, `HasSettings` / `OnSettingsRequested`),
the `EventBus`, the `ComponentRegistry` (components by name), `Component` virtuals, and the `UIStack`.

### 3.2 Ownership

```
main()
  └─ Engine (static singleton) ───────────────── owns ──> Application* (ScaleGame)   [delete in ~Engine]
       ├─ InputManager        (by value, ctor private, friend Engine)
       ├─ ResourceManager     (by value) ── owns ──> ::Model / ::Shader / ::Texture / ::Sound / ::Music / primitives
       ├─ RenderQueue         (by value) ── holds NON-owning RenderCommand pointers
       ├─ ActiveCamera        (by value) ── points to the Camera of the current mode (game camera / EditorCamera)
       ├─ DebugDraw           (by value)
       ├─ PhysicsWorld        (by value) ── owns (pimpl) btDefaultCollisionConfiguration, btCollisionDispatcher,
       │                                    btDbvtBroadphase, btSequentialImpulseConstraintSolver,
       │                                    btDiscreteDynamicsWorld, btGhostPairCallback, PhysicsDebugDrawer
       │                                    NON-owning: btCollisionObject* (added/removed by components)
       ├─ AudioManager        (by value) ── owns the MusicPlayer and the voice pool
       ├─ ParticleSystem      (by value) ── owns the pooled particles
       ├─ UIStack             (by value) ── owns std::unique_ptr<UIScreen>
       ├─ ModeMachine         (by value)
       ├─ EventBus            (by value)
       ├─ ComponentRegistry   (by value) ── type name -> factory
       └─ Editor              (by value, always compiled; only reachable with ENG_EDITOR=1)
                                 owns EditorCamera, TransformGizmo, panels, LevelSnapshot

ScaleGame
  ├─ eng::Level  ── owns ──> std::unique_ptr<GameActor>  ── owns ──> std::unique_ptr<Component>
  │                                │  parent/children pointers are NON-owning
  │                                └─ RigidBody / TriggerVolume / CharacterMotor components
  │                                     own their btCollisionShape, btMotionState, btRigidBody /
  │                                     btPairCachingGhostObject / capsule btRigidBody (player)
  ├─ eng::Camera (the game camera, registered as the active camera)
  ├─ PlayerState, PlayerInput, GameState        (by value; the components get non-owning pointers)
  ├─ GameplayManager, RayBeamSpawner, GameUI, GunHUD, GunView, MusicManager   (by value)
  ├─ ActorHandle m_player                        (weak: the Level owns the player actor)
  └─ process-wide game singletons: Settings::Instance(), SoundFXBank::Instance(), GameContext (GameState*)
       -- for components made by the level loader, which have no constructor arguments
```

Rules:
1. Only `Engine` deletes `Application` (same as the reference).
2. Only `ResourceManager` unloads GPU and audio resources. It does so in `Engine::Shutdown()` **before** `CloseAudioDevice()` / `CloseWindow()`.
3. `MarkForDestroy()` never deletes right away. `Level::FlushDestroyed()` runs at the end of the frame.
4. Components register with subsystems in `OnEnable` and unregister in `OnDisable` or the destructor, so no dangling pointers are left behind.
5. Bullet objects are always **removed from the `btDiscreteDynamicsWorld` before they are deleted**, and deleted in this order: body/ghost → motion state → shape. `PhysicsWorld` is destroyed **after** `Level::Clear()`.
6. Every `btCollisionObject` stores its owning `eng::Collider*` in `setUserPointer()`. That is how ray hits, overlaps and trigger pairs get back to a `GameActor`.
7. **Game code never keeps a raw `GameActor*` across frames.** It keeps an `ActorHandle` (an id resolved through `Level::FindActor`, null once the actor is deleted). A raw pointer from `Get()` is for the current frame only.
8. A level is replaced only between frames: the `GameplayManager` and the editor's Load are *requests* applied at a safe point (`Application::Update`, start of the editor frame), never inside a physics callback, a component update or a UI button.
9. Static destruction: the actor-id registry behind `ActorHandle` is allocated once and never destroyed on purpose, because Levels owned by the Engine singleton are deleted during static destruction.

### 3.3 Engine modes (state machine)

```
                 +------------+
   Boot ───────> |  MainMenu  | <──────────────────────────────┐
                 +------------+                                │
                  │ Start                (Editor button*)      │ "Main Menu"
                  v                                            │
                 +------------+   ESC / focus lost   +------------+
                 |    Play    | ───────────────────> |   Paused   |
                 |            | <─────────────────── |            |
                 +------------+   ESC / "Resume"     +------------+
                  ^        │                                │ "Restart"
             TAB  │        │ TAB                            └──> reload level ─> Play
                  │        v
                 +------------+
                 |   Editor   |   (simulation frozen, free camera, panels)   * editor builds only
                 +------------+

   Dialogs on the UIStack (do not change the mode):
     Settings (from the main or pause menu), "Level Complete", "You fell!" (pushed by GameUI during Play)
   While a dialog is on top of the mode's own screen, ESC belongs to the dialog.
```

| Mode | Level / physics / particles | `Application::Update` | Camera | Cursor | UI drawn | Music |
|---|---|---|---|---|---|---|
| MainMenu | frozen | **runs** (menu backdrop orbit) | slow orbit of the level | visible | `MainMenuScreen` | menu theme |
| Play | **runs** (fixed + variable step) | runs | FPS camera | **locked** (`DisableCursor`) | `DrawViewModel` + `DrawHUD` | gameplay theme |
| Paused | frozen | runs | frozen | visible | frozen frame + `PauseMenuScreen` | ducked |
| Editor | frozen | runs | `EditorCamera` | visible | panels, toolbar, gizmo, banner | ducked |

Only Play advances the fixed-step accumulator, so nothing built up while paused is replayed on resume.

---

## 4. Game loop diagram

```
Engine::Run()
│
│  while (!WindowShouldClose() && !app->NeedsToBeClosed())        // ESC is NOT the exit key (SetExitKey(KEY_NULL))
│  │
│  ├─ 1. FRAME START   time.BeginFrame(GetFrameTime())      dt = min(raw, 0.1 s), scaled and unscaled
│  │                   window.Refresh()                      sizes, DPI, resized / focused flags
│  │                   input.Poll()                          keys, buttons, wheel, mouse delta, "pressed this frame"
│  │
│  ├─ 2. MODES         modes.HandleInput()    ESC -> Pause / Resume (not while a dialog is on top),  TAB -> Editor / Play
│  │                   modes.HandleFocus()    focus lost (edge) in Play -> Pause
│  │                   modes.ApplyPendingTransition()  -> app hook (OnStartRequested, OnPause, OnResume, OnEnterEditor, ...),
│  │                                                      cursor lock, audio duck, the mode's UIScreen, ModeChangedEvent
│  │
│  ├─ 3. SIMULATE      (only Play advances the accumulator)
│  │   ├─ if PLAY: time.AdvanceAccumulator()
│  │   ├─ while (time.ConsumeFixedStep())               // FIXED STEP 0.02 s, at most 5 per frame, backlog dropped
│  │   │      level.FixedUpdate(0.02)     -> Scalable (scale animation), FirstPersonController (walk, jump), ScalingGun (cooldown, shot)
│  │   │      app->FixedUpdate(0.02)
│  │   │      physics.Step(0.02)          -> sync triggers to their actors -> btDiscreteDynamicsWorld::stepSimulation(0.02, 1, 0.02)
│  │   │                                     (ActorMotionState writes dynamic poses back to the actors)
│  │   │                                     -> trigger overlaps diffed against the last tick -> OnTriggerEnter / Exit
│  │   ├─ if PLAY:   level.Update(dt)     -> FirstPersonLook (camera), ScalingGun (aim), RayBeam, MeshRenderer ...
│  │   │             particles.Update(dt)
│  │   ├─ if EDITOR: editor.Update(unscaled dt)   -> EditorCamera fly, picking, gizmo drag, hotkeys, pending Load
│  │   ├─ app->Update(dt)                 -> ALWAYS: GameplayManager requests, PlayerInput, menu camera orbit, view model
│  │   └─ ui.Update(unscaled dt)          -> menus and dialogs work while the game is frozen
│  │
│  ├─ 4. AUDIO         audio.SetListener(active camera); audio.Update(unscaled dt)   music streams, fades, ducking
│  │
│  ├─ 5. RENDER        BeginDrawing()
│  │                     ClearBackground(world.skyColor)
│  │                     level.SubmitRenderables(renderQueue); app->SubmitRenderables(renderQueue); lighting uniforms
│  │                     BeginMode3D(activeCamera)
│  │                       renderQueue.Draw()        opaque meshes, then the transparent line pass
│  │                       particles.Draw()          billboards, alpha then additive, no depth write
│  │                       debugDraw.Draw3D(); physics.DebugDraw(); editor.Draw3D()   grid, wireframes, selection, gizmo, spawn
│  │                     EndMode3D()
│  │                     debugDraw.DrawLabels()
│  │                     app->DrawViewModel()        the held gun: own camera, own pass
│  │                     app->DrawHUD()              crosshair, cooldown ring, factor, level label
│  │                     if EDITOR: editor.DrawPanels()   toolbar, hierarchy, inspector
│  │                     ui.Draw()                   top UIScreen (menus, settings, win / lose); lower ones locked
│  │                   EndDrawing()                  swap buffers + PollInputEvents
│  │
│  └─ 6. FRAME END     events.Dispatch()           queued events: GameWon / GameLost / ScalingShot / NextLevelRequested ...
│                      renderQueue.Clear(); debugDraw.EndFrame()
│                      level.FlushDestroyed()       actors marked for destroy are deleted here (bodies leave the world first)
│
└─ Shutdown():  ui.Clear() -> app->Destroy() (level cleared: bodies removed) -> physics.Shutdown() -> audio.ReleaseVoices()
                -> resources.UnloadAll() -> audio.Shutdown() -> CloseWindow()
```

What changed from the first plan: modes are applied at the *start* of the frame (so a menu button pressed last frame takes
effect before anything updates); there is no `PushKinematics / PullDynamics / DispatchTriggers` step in the loop (the
motion states and `PhysicsWorld::Step` do it inside the fixed tick); `Application::Update` runs in every mode, while the
engine freezes only what it owns (level, physics, particles); events are dispatched before `FlushDestroyed`, so handlers
may still look at actors marked this frame; there is no render-target resize step (the game has no render targets).

---

## 5. Implementation plan (in order)

Each phase ends with something you can run. Do not start a phase before the previous one's **Done when** passes. All phases are implemented; **As built** notes record where the code differs from the text above them. Progress, decisions after this plan (D9–D14) and per‑phase devlogs: [`DEV_PROGRESS.md`](DEV_PROGRESS.md).

> Order changed on 2026-11-06 (decision D9): Audio moved before Scene and Physics because it has fewer dependencies. Phases 4–6 are now Audio, Scene, Physics.

### Phase 0 — Project setup and build system ✅
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

- **As built:** CMake also sets `CMAKE_OSX_DEPLOYMENT_TARGET 11.0`, arm64 and Release as the default build type (Phase 13). The asset copy is a `copy_assets` target (asset-only changes are copied too). `ENG_EDITOR` is always defined as 0 or 1 (`#if ENG_EDITOR`; `-DENG_EDITOR_IN_RELEASE=ON` forces 1); `ENG_SOURCE_DIR` is engine-private and used only by the editor's save mirror. `include/raylib` holds four headers (`raylib.h`, `raymath.h`, `rlgl.h`, `raygui.h`); the vendored sources were removed in Phase 13.

### Phase 1 — Core: Engine, Application, Time, Window ✅
- `eng::Application` (abstract), `eng::Engine` (Meyer's singleton, deleted copy/move, `SetApplication` ownership transfer).
- `Engine::Init`: `SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI)`,
  `InitWindow`, `SetExitKey(KEY_NULL)`, `ChangeDirectory(GetApplicationDirectory())`, `SetTargetFPS(60)`.
- `eng::Time` (dt, clamped dt, total time, fixed‑step accumulator), `eng::WindowInfo`, `eng::World`, `eng::Log`.
- `main.cpp` reduced to the reference pattern; a temporary `ScaleGame` draws the cube from the old example.

**Done when:** the window opens through `Engine::Run`, the cube renders, and closing the window calls `Destroy` + `Shutdown` cleanly.

- **As built:** the clamp is 0.1 s and at most 5 fixed steps run per frame (the backlog is dropped); `Time` never calls raylib (the Engine feeds it). Added `EngineInfo` (`IsEditorBuild()`) and `MathUtils.h`.

### Phase 2 — Input ✅
- `eng::InputManager` (private ctor, `friend Engine`), filled by `Poll()`: down/pressed/released keys and buttons, mouse position and delta, wheel.
- `eng::InputMap`: named actions → bindings (`"MoveForward" = W`, `"Fire" = MOUSE_LEFT`, `"AltFire" = MOUSE_RIGHT`, `"Jump" = SPACE`) and axes (`"Move"` WASD, `"Look"` mouse delta, `"Scroll"` wheel).
- Cursor control: `SetCursorLocked(bool)` → `DisableCursor()` / `EnableCursor()`.

**Done when:** a debug overlay prints action states, and mouse look works with the cursor locked.

- **As built:** the engine owns no bindings: the game binds its actions (`PlayerInput::BindDefaults`, F2 / F3). `InputManager::SetCursorLocked` skips the first delta after a change.

### Phase 3 — Resources and rendering ✅
- `eng::ResourceManager`: path‑keyed cache for `Model`, `Shader`, `Texture2D`, `Font` (`Sound` / `Music` added in Phase 4 with the audio device, which `AudioManager` owns); primitive generators (`GetCube()`, `GetPlane()`); `UnloadAll()`.
- `eng::Camera` (wraps `Camera3D`, fov/near/far, `Forward/Right/Up`, `Rotate(yaw,pitch)`), `eng::ActiveCamera`.
- `eng::Material` (wraps `::Material`, tint, optional shader, named float params like the reference).
- `eng::RenderCommand`, `eng::RenderQueue` (`Submit`, `Flush` inside `BeginMode3D`).
- `eng::RenderTarget` (`RenderTexture2D`, resize), `eng::DebugDraw` (lines, boxes, spheres, text‑in‑world, active only in Debug/Editor).
- Optional: `assets/shaders/lit.vs/.fs` (one directional light + ambient) so scaled boxes read well.

**Done when:** a hard‑coded scene of 10 cubes with different materials renders through the `RenderQueue`.

- **As built:** `RenderQueue::Draw` (not `Flush`) draws opaque commands, then a transparent line pass (Phase 7); `SceneLighting` feeds the lit shader's uniforms from `World`; `ResourceManager::GetPrimitive(Primitive)` and `SetDefaultLitShader` (D13). The game no longer uses `RenderTarget`.

### Phase 4 — Audio ✅
- `AudioManager`: `PlaySound(sound, volume, pitch)` with a small pool of `LoadSoundAlias` voices so the same SFX can overlap; master/music/SFX volumes; `Duck(bool)` for Pause/Editor.
- `AudioSource` (sound + volume + pitch, optional 3D attenuation by distance to the camera). A standalone class in this phase; it becomes a `Component` in Phase 5 (decision D10).
- `MusicPlayer`: `LoadMusicStream`, looping, `UpdateMusicStream` each frame, cross‑fade / fade‑in (Unity: max 0.2, speed 0.05/s).

**Done when:** a key plays a pitched SFX and music fades in, and pausing ducks the music (a demo key until Phase 8's Pause calls the same `Duck()`).

- **As built:** `AudioBus {Sfx, Music}`, master volume, `Duck`, `PauseAll / ResumeAll`; the listener follows the active camera.

### Phase 5 — Scene: GameActor, Component, Level ✅
- `GameActor`: name, `::Transform`, parent/children, `IsActive`, `MarkForDestroy`, `GetLocalMatrix()` / `GetWorldMatrix()` (`MatrixScale * QuaternionToMatrix * MatrixTranslate`, times the parent's).
- `Component`: `owner`, `OnCreate`, `OnEnable/OnDisable`, `Update`, `FixedUpdate`, `OnTriggerEnter/Exit`, `DrawInspector` (editor), `Serialize/Deserialize`.
- `GameActor::AddComponent<T>() / GetComponent<T>()` (`requires std::derived_from<T, Component>`).
- `Level`: `CreateGameActor<T>(name, parent)` (as in the reference), `Update`, `FixedUpdate`, `SubmitRenderables`, `FlushDestroyed`, `Clear`, `FindByName`.
- `MeshRenderer` component (model + material → `RenderCommand`).
- `AudioSource` (Phase 4) becomes a `Component`, positioned from its actor.

**Done when:** a parent actor rotates and its children follow, and destroying an actor mid‑update doesn't crash.

- **As built:** `GameActor::GetId`, `GetComponents()`, `Component::OnDestroy / SubmitRenderables / IsEnabled`, `Level::FindActor` and `SpawnPoint` came later (Phases 9–10).

### Phase 6 — Physics (Bullet 3.25) ✅
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

- **As built:** `OverlapBox(center, half, mask, ignore, out)` (axis-aligned) arrived in Phase 11; there is no `OverlapSphere`. Trigger dispatch happens inside `Step` (sync triggers → `stepSimulation` → diff overlaps), not as a public call. `RigidBody::Teleport(transform, keepVelocity)`, `CcdMode`, `CharacterMotorSettings`. Mesh colliders are not saved in level files.

### Phase 7 — FX: particles and lines ✅
- `ParticleEmitter` (burst count, lifetime, speed, spread cone, size, **color gradient over lifetime with alpha fade**) and `ParticleSystem` (pooled, max ~1024, drawn as billboards or small cubes).
- `LineRenderer` (start/end points, start/end colors, start/end width, fade‑out), submitted as a `LineCommand` to the RenderQueue's transparent line pass: an open tapered tube with per‑vertex colors (capped `DrawCylinderEx` segments showed stacked caps through the transparency).

**Done when:** clicking spawns a colored burst and a fading beam line.

- **As built:** as planned.

### Phase 8 — Events, modes, menus (Main Menu / Pause / Resume) ✅
- `EventBus`: `Subscribe<T>(fn) -> token`, `Unsubscribe`, `Publish<T>` (queued), `Dispatch()` at end of frame.
- `ModeMachine`: `EngineMode { MainMenu, Play, Paused, Editor }`, `Request(mode)`, guarded transitions; on transition it sets the cursor lock, ducks audio, and calls `Application` hooks.
- `UIStack` + `UIScreen` (raygui): `MainMenuScreen` (Start, Editor*, Quit), `PauseMenuScreen` (Resume, Restart Level, Main Menu, Quit), generic `MessageScreen` (title + N buttons, used by the game for win/lose). `UIStyle` loads a raygui style and font.
- Auto‑pause when `!IsWindowFocused()`.

**Done when:** Boot → Main Menu → Start → Play → ESC pauses (world frozen, cursor free) → Resume returns to Play exactly where it was → Main Menu → Quit.

- **As built:** modes apply at the start of the frame; `ModeChangedEvent` is published on every change; focus loss pauses on the loss *edge* only and never resumes by itself; ESC belongs to a dialog that is on top of the mode's screen; outside Play the engine freezes the level, physics, particles and the accumulator, while `Application::Update` still runs (`UpdateFrozen` was not needed). `UIStack`: only the top screen is interactive, changes made while drawing are deferred, an opaque screen cuts off those below. Menus gained a Settings button in Phase 13.

### Phase 9 — Editor mode (enter / exit at runtime) ✅
- `Editor` (only when `ENG_EDITOR`): **TAB** in Play enters Editor and TAB again exits. On enter it takes a `LevelSnapshot`, freezes the simulation, frees the cursor, and copies the FPS camera pose into `EditorCamera`.
- `EditorCamera`: hold RMB + WASD/QE to fly (`UpdateCameraPro`), wheel changes fly speed, `F` focuses the selection.
- `Picker`: LMB → `GetScreenToWorldRay` → `PhysicsWorld::Raycast` first (exact, rotated shapes), then falls back to render bounds for actors without a collider.
- Editing physics actors: the gizmo and inspector write the actor transform and call `RigidBody::Teleport()` (or `CharacterMotor::Warp()`); scale edits call `Collider::SetScale()` + `RigidBody::OnScaleChanged()`. `LevelSnapshot::Restore()` re‑creates bodies, so velocities are reset too.
- `TransformGizmo`: X/Y/Z axis handles; drag projects the mouse ray onto the axis. Modes **W move / R scale**; rotation is edited in the inspector (euler degrees).
- `HierarchyPanel`: tree of actors, select, rename, **Add Cube / Add Trigger / Duplicate / Delete**.
- `InspectorPanel`: transform fields + each component's `DrawInspector()` (e.g., `Scalable.speed`, trigger type).
- `EditorToolbar`: **Play** (exit editor, keep edits), **Reset** (restore snapshot), **Save** (Ctrl+S), **Load**.
- `LevelSerializer` + `ComponentRegistry` (string → factory; the game registers its components in `ScaleGame::Init`). Text format (as built; `#` starts a comment):
  ```
  lvl 1
  spawn 0 0 16  yaw 0                            # where the game puts the player (feet)
  actor "Crate A"
    parent 0                                     # index of an earlier actor; omitted for roots
    active 0                                     # omitted when active
    transform 0 0.35 -5.4   0 0 0   0.7 0.7 0.7  # position | euler degrees | scale (local)
    component MeshRenderer mesh=cube tint=235,205,80,255 lit=1 specular=0.4
    component Collider shape=box half=0.5,0.5,0.5 layer=Prop
    component RigidBody type=dynamic mass=30 friction=0.6 restitution=0 ccd=auto
    component Scalable speed=5 scaleMass=1 keepBottom=1
  end
  ```
- Editor overlay: `DrawGrid`, Bullet debug wireframes (`PhysicsWorld::DebugDraw`), trigger volumes (translucent), selection highlight, actor labels.

**Done when:** enter the editor mid‑game, move/scale a crate, add a trigger, save `level_01.lvl`, exit to Play with the edits live; restarting the app and loading the file reproduces the level.

- **As built:** the format example above is the real one. `LevelSnapshot` keeps *state* (flag, transform, body velocities) instead of a serialized level, so Reset cannot bring back an actor deleted in the editor (open). The inspector also has **Add Component** (any registered type), the hierarchy has **Set Spawn Here**, **Duplicate** (Ctrl+D, through the serializer), the toolbar a **Physics** toggle; Save also writes a copy into the source tree (editor builds), Load is applied at the start of the next editor frame and calls `Application::OnLevelLoaded`. `PropertyBag` replaced `LevelWriter / LevelReader` (D12); an actor with an unregistered component is not saved.

### Phase 10 — Game: state and player ✅
- `ActorHandle` (decision D11): id‑based weak reference to a `GameActor` (Level lookup, null once deleted). Gameplay code stores handles, not raw `GameActor*`.
- `PlayerState` (from `PlayerState.cs`): walk speed 10, scale factors `{0.25, 0.5, 0.75}` (rate 0.25 × 3), index, `ChangeFactor(±1)`, cooldown 1.0 s, `readyToShoot`, ray reach 30.
- `GameState` (from `GameState.cs`): `Reset`, `StartGame`, `WinGame`, `LoseGame`, `ProgressToNextLevel`; publishes `GameStarted / GameWon / GameLost` on the `EventBus`.
- `PlayerInput` (from `InputManager.cs`): reads the engine `InputMap`, exposes `Direction`, `Jump`, `Look`; `giveControl` flag; LMB → `gun.Fire(+1)`, RMB → `gun.Fire(-1)`, wheel → `playerState.ChangeFactor(sign)`.
- `PlayerActor`: actor with `CharacterMotor` (layer `Player`), `FirstPersonController`, `FirstPersonLook`, `ScalingGun`, child `GunView`.
- `FirstPersonController` (from `FirstPersonController.cs`): in `FixedUpdate`, camera‑relative XZ velocity × walk speed → `CharacterMotor::SetWalkVelocity`. The jump arc sets the capsule's own gravity: `t_apex = maxJumpTime/2`, `g = 2h/t_apex²` → `SetGravity(32)`, `v0 = 2h/t_apex` → `Jump(8)` when grounded (h = 1, t = 0.5). The ground check is `IsGrounded()` (sphere sweep), replacing the Unity `CheckSphere`.
- `FirstPersonLook`: yaw on the body, pitch on the camera, clamped ±89°, sensitivity.
- Player spawn read from the `.lvl` `spawn` line.

**Done when:** the player walks, jumps, and looks around in `level_01`, and the wheel cycles x0.25 → x0.5 → x0.75 (printed on screen).

- **As built:** `PlayerActor::Create(...)` is a function that returns the actor; there is no child `GunView` actor (the gun is drawn through `Application::DrawViewModel`, Phase 13). `PlayerInput` buffers jump and fire for 0.1 s and the controller takes the jump once grounded; the wheel clamps at x0.25 and x0.75. `GameState` also publishes `LevelProgressedEvent`. The player is never saved in a level file. `GameContext` gives loader-made components the `GameState`.

### Phase 11 — Game: the scaling mechanic ✅
- `ScalingGun` (from `ScalingProvider.cs`): cooldown ticked in `FixedUpdate`; `Fire(sign)` → returns if not ready; ray from the **screen center** (`camera.position`, `camera.forward`, reach 30, mask `Ground | Prop`, so the player capsule and triggers are ignored) → `PhysicsWorld::Raycast`; on hit → `RayBeamSpawner.Spawn(sign, hitPoint)`; if the hit actor has `Scalable` → `TriggerScaling(1 + factor * sign)`. Optional juice: a small `RigidBody::AddImpulse` along the beam.
- `Scalable` (from `ScalingController.cs` + `ScalingEnabler.cs` + `ScalingAudioFX.cs`): ignore if already scaling; block stretch when `scale.z >= 1.9`; `FixedUpdate`: `t += speed * fixedDt`, `scale = startScale * SmoothStep(1, target, t)`. Each tick it writes the actor scale, then `Collider::SetScale(scale)` (`setLocalScaling`) + `RigidBody::OnScaleChanged()` (mass scales with volume if `scaleMass=true`; inertia recomputed; body woken). At `t >= 1` it stops, and **destroys the actor if `|scale| < |(0.1,0.1,0.1)|`** (body removed from the world first). It plays the stretch/shrink SFX with pitch `factor ± 0.02`. Bullet scales shapes around the body origin, so a Scalable prop's box is wrapped in a `btCompoundShape` with the child offset up by its half height. The compound's `setLocalScaling` also scales that offset, so **the bottom face stays on the floor** and objects grow upward instead of into it.
- `RayBeamSpawner` (from `RayBeamProvider.cs`): muzzle position = camera + right·0.3 − up·0.25 + forward·0.6; spawns `MuzzleFX` and a `RayBeam` actor.
- `RayBeam` (from `RayBeamController.cs` + `BeamAudioFX.cs` + `BeamImpactParticle.cs`): `LineRenderer` from tail → hit point; tail advances at 20 u/s and the beam self‑destroys when the tail reaches the hit point; colors **stretch = blue→green**, **shrink = yellow→red**; impact particle burst (blue or yellow gradient, alpha 1→0); beam SFX at volume 0.3.
- `MuzzleFX` (from `ExitParticlesController.cs`): sparks + wave bursts at the muzzle, tinted by sign.

**Done when:** left click grows a crate smoothly (capped), right click shrinks it until it disappears, beams and particles match the colors, and the cooldown blocks spam.

- **As built:** `ScalingGun` publishes `ScalingShotEvent` instead of calling the spawner; `RayBeamSpawner` (and `GunView`) listen. `Scalable`: the stretch target is clamped to 1.9 × the scale before the first shot (re-measured if the editor changes the scale), destroy happens when the largest axis is below 0.1, mass follows volume, the lowest collider point is kept by shifting the actor (`keepBottom`) rather than through a compound child, and a stretch is refused when the target box overlaps layer Ground (8 cm inset). `Scalable` plays its own sound through `SoundFXBank::Instance()`. The beam sound is a placeholder ping.

### Phase 12 — Game: level flow, UI, music ✅
- `TriggerZone` base component (sits on an actor with `Collider` + engine `TriggerVolume`; filters for the `Player` layer in `OnTriggerEnter`), with `WinTrigger` (`GameState.WinGame`), `LoseTrigger` (kill volume under the map → `LoseGame`), and `NextLevelTrigger` (`GameplayManager.LoadNextLevel`).
- `GameplayManager` (from `GamePlayManager.cs`): `LevelList` (from `SceneRef.cs`) of `.lvl` paths, `LoadLevel(i)`, `LoadNextLevel()`, `ReloadCurrentLevel()`; resets `GameState` on every load; starts the game automatically on levels after the first.
- `GameUI` (from `GUIManager.cs`): subscribes to events; on `GameWon` it pushes `MessageScreen("Level Complete", Next | Main Menu)`, on `GameLost` it pushes `MessageScreen("You fell!", Retry | Main Menu)`; it calls `PlayerInput.SetControl(false)` and frees the cursor while shown.
- `MusicManager` (from `ThemeMusicManager.cs`): `MENU` and `GAME_PLAY` themes, switches on mode change, fade‑in. Keep the original music credits in `assets/audio/CREDITS.txt`.
- Author 2–3 levels in the editor (a gap to bridge by stretching a plank, a wall to shrink, a crate staircase).

**Done when:** a full run works: Menu → Level 1 → win → Level 2 → fall → retry → win → back to Menu.

- **As built:** `GameplayManager` loads are requests applied in `ScaleGame::Update` (a trigger fires inside the physics step, a button inside the UI draw); every load resets `GameState` / `PlayerState`, and the game starts by itself whenever the engine is in Play. `NextLevelTrigger` publishes `NextLevelRequestedEvent`. Levels 1–3 are script-written drafts (same serializer as the editor) meant to be tuned in the editor; scripted playthroughs win all three.

### Phase 13 — Polish and release ✅
- `GunView` (gun model drawn in a second `BeginMode3D` pass with a fixed camera so it never clips into walls) and `GunHUD` (from `GunDiegeticHUD.cs`): the `"x0.25"` text rendered into a small `RenderTarget` mapped onto the gun's screen material; fallback is plain HUD text. Crosshair and cooldown ring in `DrawHUD`.
- Settings in the pause menu: mouse sensitivity, volumes, fullscreen (`ToggleBorderlessWindowed`).
- Release build with `ENG_EDITOR=0`; check that the static binary runs on a clean Mac with the `assets/` folder next to it; update README (controls, build, editor shortcuts).

- **As built:** `GunView` is drawn through the new `Application::DrawViewModel()` hook with its own camera and the depth test off (no clipping), with kick, bob and a factor light; the factor is also in the HUD (`GunHUD`: crosshair that turns green on a scalable prop, cooldown ring, factor text and pips, hints), not on a `RenderTarget`. Settings (sensitivity, master / music / effects volume, fullscreen) are reached from the main *and* the pause menu through `Application::HasSettings / OnSettingsRequested` and stored in `~/Library/Application Support/BuildToScale/settings.cfg`. Release: `tools/package_release.sh`, macOS 11 / arm64, README rewritten. Cleanup: vendored raylib sources, stale build output, the sandbox and its components removed.

---

## 6. Key interfaces (sketch, as built)

```cpp
namespace eng {

class Application {
public:
    virtual ~Application() = default;
    virtual bool Init() = 0;
    virtual void Update(float dt) = 0;                  // every frame, in every mode
    virtual void FixedUpdate(float fixedDt) {}          // only while simulating (Play)
    virtual void SubmitRenderables(RenderQueue&) {}     // every frame, also while paused
    virtual void DrawViewModel() {}                     // 3D after the world: the held gun, own camera
    virtual void DrawHUD() {}                           // 2D, after the 3D pass
    virtual void Destroy() = 0;

    // Mode hooks (called by ModeMachine)
    virtual void OnStartRequested() {}                  // main menu "Start"
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void OnEnterEditor() {}
    virtual void OnExitEditor() {}
    virtual void OnReturnToMainMenu() {}
    virtual void OnRestartRequested() {}

    virtual void        OnLevelLoaded() {}              // the editor replaced the level: rebind / respawn
    virtual std::string GetLevelPath() const { return {}; }    // where the editor saves / loads the active level
    virtual bool        HasSettings() const { return false; }  // menus show a Settings button
    virtual void        OnSettingsRequested() {}               // the game pushes its own settings screen
    virtual EngineMode  GetStartMode() const { return EngineMode::MainMenu; }
    virtual const char* GetTitle() const;
    virtual Level*      GetActiveLevel() { return nullptr; }   // used by the editor and the renderer

    void SetNeedsToBeClosed(bool value);
    bool NeedsToBeClosed() const;
};

class Engine {
public:
    static Engine& GetInstance();
    bool  Init(int width = 1280, int height = 720);
    void  Run();
    void  Shutdown();
    void  SetApplication(Application* app);       // ownership transfer, as in the reference

    Application*       GetApplication() const;
    InputManager&      GetInputManager();
    ResourceManager&   GetResources();
    ActiveCamera&      GetMainCamera();
    RenderQueue&       GetRenderQueue();
    DebugDraw&         GetDebugDraw();
    AudioManager&      GetAudio();
    PhysicsWorld&      GetPhysics();
    ParticleSystem&    GetParticles();
    EventBus&          GetEvents();
    ModeMachine&       GetModes();
    Editor&            GetEditor();
    ComponentRegistry& GetComponentRegistry();
    UIStack&           GetUI();
    Time&              GetTime();
    const WindowInfo&  GetWindow() const;
    const World&       GetWorld() const;
    // ... subsystems by value, ctor private, copy / move deleted ...
};

class Component {
public:
    virtual ~Component() = default;
    virtual void OnCreate() {}                      // owner is set
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void Update(float dt) {}
    virtual void FixedUpdate(float fixedDt) {}
    virtual void SubmitRenderables(RenderQueue&) const {}
    virtual void OnTriggerEnter(GameActor& other) {}
    virtual void OnTriggerExit(GameActor& other) {}
    virtual void DrawInspector(InspectorLayout& ui) {}           // editor: raygui fields
    virtual void Serialize(PropertyBag& out) const {}            // level files
    virtual void Deserialize(const PropertyBag& in) {}
    virtual const char* TypeName() const = 0;       // must equal the name it is registered under
    GameActor* GetOwner() const;
    bool IsEnabled() const;  void SetEnabled(bool);
};

// Weak actor reference: the id, never the address
class ActorHandle {
public:
    ActorHandle(const GameActor* actor = nullptr);
    GameActor* Get() const;                          // null once the actor was deleted (Level::FindActor)
    bool IsValid() const;  bool IsAlive() const;     // IsAlive: valid and not marked for destroy
};

// ---- physics/PhysicsWorld.h  (no Bullet include: pimpl) ----
struct RaycastHit { GameActor* actor; Collider* collider; Vector3 point; Vector3 normal; float distance; };

class PhysicsWorld {
public:
    PhysicsWorld();  ~PhysicsWorld();                       // defined in .cpp (pimpl)
    bool Init(const Vector3& gravity);
    void Step(float fixedDt);                               // sync triggers, stepSimulation(dt, 1, dt), trigger Enter / Exit
    std::optional<RaycastHit> Raycast(const Vector3& origin, const Vector3& dir, float maxDist,
                                      uint32_t mask = Layer::All) const;
    void OverlapBox(const Vector3& center, const Vector3& halfExtents, uint32_t mask,
                    const GameActor* ignore, std::vector<GameActor*>& out) const;   // axis-aligned
    void SetDebugDrawEnabled(bool on);
    void DebugDraw() const;                                 // inside BeginMode3D
    void Shutdown();
private:
    friend class RigidBody; friend class TriggerVolume; friend class CharacterMotor;
    struct Impl;  std::unique_ptr<Impl> m_impl;             // owns all bt* world objects
};

// ---- level/ : files and factories ----
class ComponentRegistry {                                   // the game registers its components in Application::Init
public:
    template<typename T> void Register(const std::string& typeName);   // T default-constructible Component
    bool IsRegistered(const std::string&) const;
    std::vector<std::string> GetTypeNames() const;          // the editor's Add Component list
    Component* Create(const std::string& typeName, GameActor& actor) const;
};
namespace LevelSerializer {                                 // text .lvl files; see LevelSerializer.h for the format
    SaveResult ToText(const Level&);   LoadResult FromText(Level&, const std::string&);
    bool Save(const Level&, const std::string& path);   LoadResult Load(Level&, const std::string& path);
    GameActor* Duplicate(Level&, GameActor&);
}

// ---- the events the game and the engine publish on the EventBus ----
//   engine: ModeChangedEvent
//   game:   ScaleFactorChangedEvent, GameStartedEvent, GameWonEvent, GameLostEvent, LevelProgressedEvent,
//           ScalingShotEvent (gun -> beams, view model), NextLevelRequestedEvent (trigger -> GameplayManager)

} // namespace eng
```

---

## 7. Engine classes (as built)

| # | Class | Folder | Responsibility | Origin |
|---|---|---|---|---|
| 1 | `Engine` | core | Singleton; owns the Application and subsystems; `Init/Run/Shutdown`; the game loop | Reference `Engine` |
| 2 | `Application` | core | Abstract game interface: updates, draw hooks (`DrawViewModel`, `DrawHUD`), mode hooks, level and settings hooks | Reference `Application` |
| 3 | `Time` | core | dt (clamped, scaled / unscaled), elapsed, fixed-step accumulator (Play only), time scale | Reference `UpdateDeltaTime` |
| 4 | `WindowInfo` | core | Screen/render size, high‑DPI scale, resize and focus flags | Reference `Window` struct |
| 5 | `World` | core | World up/right/forward, gravity, sky color, sun and ambient light | Reference `World` struct |
| 6 | `EventBus` | core | Typed pub/sub, queued, dispatched at the end of the frame | New (replaces SO listeners) |
| 7 | `Log` | core | Leveled logging over `TraceLog` | New |
| 8 | `EngineInfo` | core | Engine name / version, `IsEditorBuild()` | New |
| 9 | `MathUtils` | core | `DecomposeTransform` (raymath's `MatrixDecompose` is row-based and wrong with non-uniform scale) | New |
| 10 | `EngineMode`, `ModeChangedEvent` | modes | `MainMenu, Play, Paused, Editor` and the event published on every change | New |
| 11 | `ModeMachine` | modes | Guarded transitions (deferred), ESC / TAB / focus loss, cursor, audio ducking, mode screen, app hooks | Reference `SetPlayMode` → generalized |
| 12 | `InputManager` | input | Poll raylib; key/button down/pressed/released; mouse pos/delta/wheel; cursor lock | Reference `InputManager` |
| 13 | `InputMap` | input | Named actions/axes → bindings (the game binds them) | New |
| 14 | `ResourceManager` | resources | Cache + lifetime of models, shaders, textures, sounds, music, fonts; primitives; the game's default lit shader | Replaces `GraphicsAPI` creation + `Model` helper |
| 15 | `Camera` | render | Wraps `Camera3D`; basis vectors; yaw/pitch; perspective params | Reference `Camera` |
| 16 | `ActiveCamera` | render | Points at the camera used for rendering this frame | Reference `ActiveCamera` |
| 17 | `Material` | render | `::Material` + tint + shader params map | Reference `Material` |
| 18 | `MeshRenderer` | render | Component: mesh/model + material → `RenderCommand`; bounds for picking; level-file fields | Reference `Mesh` usage |
| 19 | `RenderQueue` (+ `RenderCommand`, `LineCommand`, `SceneLighting`) | render | Collect commands, draw opaque then the transparent line pass inside `BeginMode3D`; scene light uniforms | Reference `RenderQueue` |
| 20 | `RenderTarget` | render | `RenderTexture2D` wrapper with resize (a utility; the shipped game does not use one) | Reference `FrameBuffer` |
| 21 | `LineRenderer` | render | Component: thick gradient line (open tube) with fade-out | New (Unity `LineRenderer`) |
| 22 | `DebugDraw` | render | Immediate debug shapes and labels (editor builds) | New |
| 23 | `GameActor` | level | Name, id, `::Transform`, hierarchy, components, active, destroy flag | Reference `GameActor` |
| 24 | `Component` | level | Base behavior/data unit attached to an actor (update, trigger, inspector and file hooks) | New (Unity `MonoBehaviour`) |
| 25 | `Level` | level | Owns actors; factory; update/fixed/submit; deferred destroy; spawn point; id lookup | Reference `Level` |
| 26 | `ActorHandle` | level | Weak actor reference by id | New (decision D11) |
| 27 | `PropertyBag` | level | Ordered `key=value` pairs of one component in a level file | New |
| 28 | `ComponentRegistry` | level | `TypeName` → factory, so components can be created from files and from the editor | New |
| 29 | `LevelSerializer` | level | Save/load/duplicate `.lvl` text files | New (Unity scenes) |
| 30 | `PhysicsWorld` | physics | Pimpl over `btDiscreteDynamicsWorld`: init/shutdown, fixed `Step`, raycast, `OverlapBox`, trigger dispatch, debug draw | New (Bullet) |
| 31 | `PhysicsLayers` | physics | Collision group/mask bit flags (`Ground, Prop, Player, Trigger, Beam`) | New (Unity layers) |
| 32 | `Collider` | physics | Component: shape description → per‑actor `btCollisionShape` (box/sphere/capsule/convex hull/static mesh, compound for center offset / bottom pivot); `SetScale` | New (Unity `Collider`) |
| 33 | `RigidBody` | physics | Component: static/dynamic/kinematic `btRigidBody`; mass, friction, impulses, CCD, `Teleport`, `OnScaleChanged` | New (Unity `Rigidbody`) |
| 34 | `ActorMotionState` | physics | `btMotionState` bridging `btTransform` ↔ actor `::Transform` | New (Bullet glue) |
| 35 | `TriggerVolume` | physics | Component: `btPairCachingGhostObject`, no contact response; overlap set per tick | New (Unity `isTrigger`) |
| 36 | `CharacterMotor` | physics | Component: dynamic capsule `btRigidBody` (no rotation, own gravity); walk velocity, sphere‑sweep grounding, slope limit, jump, warp | New (Unity `CharacterController`) |
| 37 | `RaycastHit` | physics | Hit actor, collider, point, normal, distance | New (Unity `RaycastHit`) |
| 38 | `PhysicsDebugDrawer` | physics | `btIDebugDraw` → raylib `DrawLine3D` | New (Bullet glue) |
| 39 | `BulletConvert`, `PhysicsWorldImpl` | physics | Internal headers: math converters; the pimpl's contents | New (Bullet glue) |
| 40 | `AudioManager` | audio | SFX voice pool (aliases), master / bus volumes, ducking, pause all | New |
| 41 | `AudioSource` | audio | Component: sound + volume + pitch (+ distance attenuation) | New (Unity `AudioSource`) |
| 42 | `MusicPlayer` | audio | Streaming music, loop, fade/cross‑fade | New |
| 43 | `ParticleEmitter` | fx | Component + `ParticleEmitterSettings`: burst config, color/alpha gradient over lifetime | New (Unity `ParticleSystem`) |
| 44 | `ParticleSystem` | fx | Pooled simulation (1024) + drawing of all particles | New |
| 45 | `UIScreen` | ui | Abstract raygui screen: push/pop hooks, `Update`, `Draw`, `IsOpaque` | New |
| 46 | `UIStack` | ui | Push/pop screens (deferred while drawing); only the top screen is interactive; lower ones are drawn locked up to the topmost opaque one | New |
| 47 | `MainMenuScreen`, `PauseMenuScreen`, `MessageScreen`, `EditorBannerScreen` | ui | Start / Editor / Settings / Quit; Resume / Restart / Settings / Main Menu / Quit; generic title + buttons dialog; the editor's top bar (all in `MenuScreens.h`) | New (required feature) |
| 48 | `UIStyle` | ui | raygui style + layout helpers; `RayGuiImpl.cpp` holds `RAYGUI_IMPLEMENTATION` | Replaces ImGui init |
| 49 | `Editor` | editor | Editor-mode coordinator: enter/exit, selection, hotkeys, add / duplicate / delete, save / load, spawn, overlays | Reference play‑mode toggle → full editor |
| 50 | `EditorCamera` | editor | Free‑fly camera (RMB + WASD/QE), focus | Current `main.cpp` free camera |
| 51 | `Picker` | editor | Mouse ray → physics raycast first, then render bounds | New |
| 52 | `TransformGizmo` | editor | Axis handles; move/scale dragging | New |
| 53 | `HierarchyPanel` | editor | Actor tree; add cube / trigger, duplicate, delete, rename, set spawn | New |
| 54 | `InspectorPanel`, `InspectorLayout` | editor | Transform + component `DrawInspector()` fields; widget column; Add Component | New |
| 55 | `EditorToolbar`, `EditorUI` | editor | Play / Reset / gizmo mode / Physics / Save / Load; panel rectangles and text size | New |
| 56 | `ActorEdit` | editor | Transform writes that keep physics in step (shared by the gizmo and the inspector) | New |
| 57 | `LevelSnapshot` | editor | In‑memory copy of every actor's state (flag, transform, body velocity) used by Reset | New |

---

## 8. Game classes (Application layer, as built)

| # | Class | Folder | Responsibility | Unity origin |
|---|---|---|---|---|
| 1 | `ScaleGame` | game | `eng::Application`; registers components, owns the `Level`, the camera and the systems below; mode / level / settings hooks; menu camera; `DrawViewModel`, `DrawHUD` | — (reference `Game`) |
| 2 | `GameState` | state | Idle / Playing / Won / Lost; `Reset / StartGame / WinGame / LoseGame / ProgressToNextLevel` (guarded); publishes events | `GameState.cs` |
| 3 | `PlayerState` | state | Walk speed 10, scale factors x0.25 / x0.5 / x0.75 + `ChangeFactor`, cooldown 1 s, reach 30 | `PlayerState.cs` |
| 4 | `Settings` | state | Mouse sensitivity, three volumes, fullscreen; saved in the user's application-support folder | — (new) |
| 5 | `SoundFXBank` | state | The game's one bank of sound effects (stretch, shrink, beam); `PlayScaling(sign, factor)` | `SoundFX.cs` |
| 6 | `LevelList` | state | Ordered list of `.lvl` paths | `SceneRef.cs` |
| 7 | `PlayerInput` | player | Actions → direction / look / jump / fire / wheel; jump and fire buffered 0.1 s; control gate | `InputManager.cs` |
| 8 | `PlayerActor` | player | `Create(...)`: the player actor (capsule collider, motor, controller, look, gun); never saved in files | Player prefab |
| 9 | `FirstPersonController` | player | Camera‑relative walk, jump arc gravity (32) and speed (8), buffered jump when grounded | `FirstPersonController.cs` |
| 10 | `FirstPersonLook` | player | Yaw (body) / pitch (camera) from the mouse delta, clamp, sensitivity from `Settings`; puts the camera on the eyes | Cinemachine POV |
| 11 | `ScalingGun` | player | Cooldown, centre‑screen raycast (Ground \| Prop), publishes `ScalingShotEvent`, triggers `Scalable`; per-frame aim query for the crosshair | `ScalingProvider.cs` |
| 12 | `GunView` | player | The held gun drawn in its own pass (own camera, depth off): kick, bob, factor light | Gun prefab |
| 13 | `Scalable` | props | SmoothStep scale animation, 1.9 cap, min‑size destroy, mass by volume, keep-bottom, wall check, pitched SFX | `ScalingController.cs`, `ScalingEnabler.cs`, `ScalingAudioFX.cs` |
| 14 | `RayBeamSpawner` | fx | Listens to `ScalingShotEvent`: muzzle burst, `RayBeam` actor, impact burst, beam sound | `RayBeamProvider.cs` |
| 15 | `RayBeam` | fx | Gradient beam whose tail travels to the hit point and removes it; impact burst | `RayBeamController.cs`, `BeamImpactParticle.cs` |
| 16 | `MuzzleFX` | fx | Sparks + wave burst tinted by stretch/shrink | `ExitParticlesController.cs` |
| 17 | `GameplayManager` | flow | Load / next / reload as deferred requests; resets states; auto-start in Play; handles `NextLevelRequestedEvent` | `GamePlayManager.cs` |
| 18 | `GameContext` | flow | The `GameState*` for components created by the level loader | — (new) |
| 19 | `TriggerZone` | flow | Base: player‑only `OnTriggerEnter` → `OnPlayerEnter()` | — |
| 20 | `WinTrigger` / `LoseTrigger` / `NextLevelTrigger` | flow | `WinGame()` / `LoseGame()` (kill volume) / publishes `NextLevelRequestedEvent` | `WinningTriggerController.cs`, `LosingTriggerController.cs`, `NextSceneTriggerController.cs` |
| 21 | `GameUI` | ui | Win / lose `MessageScreen`s from the events; cuts and restores player control | `GUIManager.cs`, `Win/Lose/GameStartListener.cs` |
| 22 | `GunHUD` | ui | Crosshair (green on a scalable prop), cooldown ring, factor text + pips, hints | `GunDiegeticHUD.cs` |
| 23 | `SettingsScreen` | ui | Sliders and fullscreen checkbox; applies live, saves on close | — (new) |
| 24 | `MusicManager` | audio | Menu vs gameplay theme from `ModeChangedEvent`; fade‑in, cross‑fade | `ThemeMusicManager.cs` |

Events defined by the game: `ScaleFactorChangedEvent`, `GameStartedEvent`, `GameWonEvent`, `GameLostEvent`,
`LevelProgressedEvent`, `ScalingShotEvent`, `NextLevelRequestedEvent`.

Props (crates, planks, walls) are **not classes**. They are data in `.lvl` files: `MeshRenderer` + `Collider` + `RigidBody` (static for level geometry, dynamic for crates) + `Scalable` when they react to the gun.

### Game controls

| Input | Action |
|---|---|
| WASD / arrows | Move |
| Mouse | Look |
| Space | Jump |
| **Left click** | Stretch (scale up by `1 + factor`) |
| **Right click** | Shrink (scale down by `1 − factor`) |
| **Mouse wheel** | Change factor x0.25 / x0.5 / x0.75 |
| ESC | Pause / Resume (closes a dialog that is open) |
| TAB | Enter / exit Editor (editor builds) |
| F2 / F3 | Debug grid / Bullet wireframes (editor builds) |

---

## 9. Main runtime flows (as built)

**Fire (left click):**
```
input.Poll ─> PlayerInput.Update (in Application::Update): "Fire" pressed -> buffered 0.1 s (control gate, Play only)
FixedUpdate ─> ScalingGun: state.TickCooldown(dt); ConsumeFire() -> Fire(+1)
   ├─ !state.IsReadyToShoot() ? return
   ├─ state.StartCooldown()                                   // 1 s
   ├─ PhysicsWorld.Raycast(camera.pos, camera.forward, 30, Ground | Prop)
   │     └─ hit.actor has Scalable ─> Scalable.TriggerScaling(1 + factor * sign)
   │            ├─ ignored while scaling; stretch clamped to 1.9 x base; refused if OverlapBox(Ground) at the target size
   │            └─ plays the pitched SFX (SoundFXBank)
   └─ EventBus.Publish(ScalingShotEvent{sign, factor, muzzle, point, normal, target})
Scalable.FixedUpdate: t += speed*dt; scale = start * SmoothStep(1, target, t); keep the lowest point in place
   ─> Collider scale + RigidBody.OnScaleChanged (inertia, AABB, wake) [+ mass by volume]
   t>=1 ─> stop; destroy if the largest axis < 0.1 (MarkForDestroy: the body leaves the world at the end of the frame)
[end of frame] events.Dispatch ─> RayBeamSpawner.Spawn: MuzzleFX burst + RayBeam actor (blue→green / yellow→red) + impact burst + beam SFX
                              ─> GunView: kick + muzzle glow
RayBeam.Update: tail += 20*dt; tail reaches the hit point ─> MarkForDestroy ─> Level.FlushDestroyed
```

**Win, and the next level:**
```
PhysicsWorld.Step (trigger dispatch) ─> WinTrigger.OnTriggerEnter(player) ─> GameState.WinGame()  (guarded: Playing -> Won)
   ─> EventBus.Publish(GameWonEvent) ─> [end of frame] GameUI.ShowWon:
          PlayerInput.SetControl(false); UIStack.Push(MessageScreen "Level Complete")  (OnPush frees the cursor)
   "Next Level" button ─> GameplayManager.LoadNextLevel()           // a request, not a load
   next frame, ScaleGame::Update ─> GameplayManager.Update ─> LoadNow(index):
          GameState.ProgressToNextLevel (when Won) ─> LevelProgressedEvent; Reset GameState + PlayerState
          loader: LevelSerializer.Load (Level.Clear + actors + SpawnPoint) ─> SpawnPlayer
          engine mode is Play ─> GameState.StartGame() ─> GameStartedEvent ─> GameUI: PlayerInput.SetControl(true)
NextLevelTrigger ─> NextLevelRequestedEvent ─> GameplayManager.LoadNextLevel (same path)
LoseTrigger / "Retry" ─> GameState.LoseGame ─> "You fell!" ─> ReloadCurrentLevel
```

**Enter / exit editor, save and load:**
```
TAB (Play) ─> ModeMachine.Request(Editor) ─> [next frame] ApplyPendingTransition:
      Editor.Enter: LevelSnapshot.Capture, swap to EditorCamera, Bullet wireframes on
      cursor free, audio duck, EditorBannerScreen, app->OnEnterEditor
           ... edit (gizmo -> ActorEdit -> RigidBody.Teleport / Collider scale; inspector; hierarchy) ...
TAB / "Play" ─> Editor.Exit: back to the game camera, wireframes restored; edits stay live
"Reset"  ─> LevelSnapshot.Restore: flags, transforms, scales, velocities back; actors added in the editor removed
            (an actor deleted in the editor is NOT brought back: known limitation)
Ctrl+S   ─> LevelSerializer.Save(app->GetLevelPath()) + a copy in the source tree (editor builds)
"Load"   ─> request; next editor frame: Level.Clear + LevelSerializer.Load, app->OnLevelLoaded() (respawn the player),
            new snapshot
```

**Settings:**
```
menu "Settings" (shown when app->HasSettings()) ─> app->OnSettingsRequested ─> UIStack.Push(SettingsScreen)
   sliders / checkbox ─> Settings.Apply (AudioManager volumes, ToggleBorderlessWindowed) while dragging
   Back / ESC ─> UIStack.Remove ─> OnPop ─> Settings.Save (~/Library/Application Support/BuildToScale/settings.cfg)
```

---

## 10. Risks and notes

Status after Phase 13 in brackets.

- **raylib name collisions**: see §0. Keep `using namespace eng` out of the code; prefix raylib types with `::` inside `eng` when needed. [held to the end; `eng::Camera` vs `::Camera` and `eng::Material` vs `::Material` needed care in every scratch test]
- **Include order**: `raymath.h` must come after `raylib.h` (it redefines the vector types otherwise). [seen once, in `LevelSerializer.cpp`]
- **ESC closes raylib windows by default**: `SetExitKey(KEY_NULL)` in `Engine::Init`. [done]
- **Input timing**: raylib polls events inside `EndDrawing()`; input is read once per frame in `InputManager::Poll()` and never in `FixedUpdate`. Jump and fire are *buffered* for 0.1 s in `PlayerInput` and consumed by the next fixed step. [done]
- **Bullet build flags must match** (single precision, arm64). [done; verified when the libraries were built and again for the macOS 11 deployment target]
- **raymath `MatrixDecompose` reads matrix rows**: always use `eng::DecomposeTransform`. [done]
- **macOS toolchain**: an Xcode linker older than the Command Line Tools SDK fails with `tapi error: malformed file`. [README lists the fixes]
- **Deployment target**: without `CMAKE_OSX_DEPLOYMENT_TARGET` the binary requires the macOS version of the build machine (the first release package demanded macOS 27.2). [fixed: 11.0 / arm64 in CMake; a run on a second, clean Mac is still the developer's check]
- **Raw `GameActor*` in game code** can dangle after a late destroy. [fixed: `ActorHandle`; the actor-id registry is never destroyed because Levels die during static destruction]
- **Dynamic capsule player**: slopes, bouncing, wall sticking, being launched by scaling props. [kept the dynamic capsule: ground-plane projection, grounded glue, vertical speed clamp, jump grace; the fallback `btKinematicCharacterController` was not needed]
- **Scaling into the player or into other props**: [mitigated: speed 5, mass by volume, the stretch is refused when the target box overlaps static geometry (axis-aligned test with an 8 cm inset, so rotated props may be refused a little early); props pushing the player are not clamped beyond the motor's vertical speed limit]
- **Shape scaling cost**: static meshes must not be scaled at runtime; scalable props use primitive shapes. [mesh colliders are also not saved in level files]
- **Tunnelling**: CCD on dynamic bodies below 0.3 m (`CcdMode::Auto`). [done]
- **Deletion order**: bodies leave the world before they are deleted; `FlushDestroyed` runs after the fixed loop. [done, ASan-checked in Phase 6]
- **High‑DPI on M2**: `FLAG_WINDOW_HIGHDPI`; render size differs from screen size; UI and the mouse use screen points. [done]
- **Shutdown order**: UI → application → physics → audio voices → resources → audio → window. [done]
- **Level replacement**: a level may only be cleared between frames (requests applied in `Application::Update` / at the start of the editor frame). [done; see ownership rule 8]
- **Editor Reset and deleted actors**: `LevelSnapshot` keeps state, not a serialized copy, so Reset cannot recreate an actor deleted in the editor. [open: the snapshot could use `LevelSerializer` text]
- **Mesh and model colliders / meshes in level files** are not serialized (primitives only). [open until a level needs a model]
- **Audio assets are placeholders** (`tools/generate_placeholder_audio.py`); the original tracks' licenses must be checked before use. [open: `assets/audio/CREDITS.txt`]
- **Unsigned binary**: Gatekeeper asks on the first run of a downloaded copy. [documented in the README]

---

## 11. What changed since the first plan (validation, 2026-11-08)

Every phase is implemented. The layering, the ownership rules, the loop shape, the engine / game split and
the physics isolation are as planned. These are the places where the code differs, and why.

| Area | Original plan | As built | Why |
|---|---|---|---|
| Phase order | Scene, Physics, Audio | Audio, Scene, Physics (D9) | Audio has fewer dependencies |
| Physics | custom AABB layer (first idea) | Bullet 3.25 behind pimpl (D4–D6); player is a dynamic capsule (D8) | Rigid bodies, rotated shapes, runtime scaling; no kinematic-controller jitter |
| Weak references | raw pointers | `ActorHandle` (D11) | ASan use-after-free found in Phase 6 |
| Level files | `LevelWriter` / `LevelReader`, `level "Name"` line, euler format example | one `PropertyBag` (D12): `lvl 1`, `spawn x y z yaw`, `actor` blocks with `parent`, `active`, `transform`, `component Type key=value`; unregistered components make an actor unsaved | Simpler; keeps beams and other transient actors out of files |
| Player in files | player spawned from the spawn line | same, and the player is never saved (its components are not registered) | The game owns the player's wiring |
| Lit shader | owned by the game and the editor | `ResourceManager::SetDefaultLitShader` (D13) | Level loading must work in release builds |
| `LevelSnapshot` | serialized level; Reset re-creates bodies | actor state in memory; Reset restores transforms, scales, flags and velocities, removes new actors | Written before the serializer existed; deleted actors cannot return (open) |
| Editor | panels, gizmo, toolbar | plus Add Component, Set Spawn Here, Duplicate, Physics toggle, source-tree save mirror, `PropertyBag`-driven inspector | Needed to author levels in the editor |
| Frozen modes | `Application::UpdateFrozen` | not built: `Application::Update` always runs and the engine freezes level, physics and particles | One hook is enough; the game decides what its menus do |
| Loop | `PushKinematics / PullDynamics / DispatchTriggers` steps, render-target resize | all inside `PhysicsWorld::Step` and the motion states; no resize step | Fewer moving parts |
| Mode changes | applied after input in the frame | applied at the start of the frame | A menu button takes effect before anything updates |
| Scaling hit | `ScalingGun` calls the beam spawner | `ScalingGun` publishes `ScalingShotEvent`; the spawner and the gun view listen | New listeners (HUD, audio) need no change in the gun |
| `Scalable` | block at `scale.z >= 1.9`; destroy at `\|scale\| < \|(0.1,0.1,0.1)\|`; bottom stays via a compound child | cap relative to the base scale with the target clamped; destroy at largest axis below 0.1; lowest collider point kept by shifting the actor; mass by volume; refused when the target overlaps static geometry | Props are not unit size; the stretch must never overshoot or clip a wall |
| `PhysicsWorld` | `OverlapSphere / OverlapBox(.., rot, ..)`, public `DispatchTriggers` | `OverlapBox(center, half, mask, ignore, out)` axis-aligned; trigger dispatch inside `Step` | Only the wall check needed an overlap query |
| Level flow | `GameplayManager.LoadNextLevel` called by the trigger | triggers publish / call `GameState`; loads are deferred requests applied in `Application::Update` | A trigger fires inside the physics step and a button inside the UI draw: neither may clear the level |
| Game singletons | none | `Settings`, `SoundFXBank::Instance()`, `GameContext` | Components made by the level loader have no constructor arguments |
| `GunView` / `GunHUD` | gun in a second `BeginMode3D`; factor text on a `RenderTarget` on the gun | gun in its own pass via `Application::DrawViewModel` (own camera, depth off); factor shown by the gun's light and the HUD text; no render target | Simpler and robust; no clipping |
| Settings | in the pause menu | `Settings` + `SettingsScreen` from the main and pause menus through `Application::HasSettings / OnSettingsRequested`; stored in `~/Library/Application Support` | The engine does not know which options a game has |
| Sandbox | built over Phases 3–7 | removed in Phase 13 (`ScaleGame` is the Application layer only) | It exercised the engine; the game replaced it |
| Folders | `managers/`, `scaling/`, `triggers/` in `game/` | `flow/`, `fx/`, `props/`, `ui/`, `audio/` | Grouped by what the code does |
| Assets | `models/`, `fonts/`, ogg music | `levels/`, `shaders/`, `audio/` (wav placeholders) only | Nothing used models or fonts |
| Vendored raylib | raylib headers and sources in `include/raylib` | four headers; the prebuilt library is `lib/libraylib.a` | The sources were never compiled |
| Build | Debug has the editor; release unspecified | Release by default; macOS 11 / arm64; `tools/package_release.sh` | A binary that runs on another Mac |

Not built from the original list: `RenderTarget` as a gun texture, `OverlapSphere`, `UpdateFrozen`, `LevelWriter /
LevelReader`, `BeamImpactParticle` (inside `RayBeam`), `SceneRef` (`LevelList`), `Win/Lose/GameStartListener` (`GameUI`
subscribes to the events), `managers/`.
