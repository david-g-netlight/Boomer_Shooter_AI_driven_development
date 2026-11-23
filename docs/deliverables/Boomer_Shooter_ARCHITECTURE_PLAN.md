# Architecture Plan — From "Build to Scale" to a Boomer‑Shooter base

This plan changes the engine and game described in [`ARCHITECTURE_PLAN.md`](ARCHITECTURE_PLAN.md) (all 14 phases
built, status 2026-11-08). It removes the *Build to Scale* rules and the editor's hierarchy and inspector panels, and adds an in‑game developer
console and a placeholder arena. When it is done, the development team has a clean base to build the
Boomer‑Shooter on.

Source task: [`tasks/TASK_03.md`](tasks/TASK_03.md). Progress: [`Boomer_Shooter_DEV_PROGRESS.md`](Boomer_Shooter_DEV_PROGRESS.md).

**The target game in one line:** the player spawns inside an arena and a 5‑minute timer starts. About 3 waves of
enemies come, and the next wave starts each time the room is cleared. The player **wins** by killing every wave
before the time runs out, or by **surviving** the whole 5 minutes.

**Scope of this plan:** remove, simplify and add what the base needs (see §1). The timer, the waves and the enemies
themselves are **not** in this plan. §9 lists where they plug in.

---

## 0. Reading the task: decisions taken

The task has two lines that seem to clash: *"Remove the editor GUI"* and *"Keep editor capabilities"*. This plan
reads them like this:

| # | Decision | Choice | Why |
|---|---|---|---|
| B‑D1 | Editor GUI vs editor capabilities | **Remove the Unity‑style hierarchy tree and component inspector (and the banner). Keep a light editor:** free camera, click‑select, transform gizmo (W move / R scale), a small toolbar (Play · Reset · Move · Scale · Physics · Save, no Load) and a one‑line status. Keep everything the editor *can do* (enter/exit, pick, select, add, duplicate, delete, transform, components and their fields, spawn point, snapshot reset, save, load): component, hierarchy and file work goes through the **developer console** | The editor feels lighter without the tree and the inspector, and the viewport tools (gizmo, click) stay where they are fastest. No editing feature is lost (revised by the developer, B‑D12) |
| B‑D2 | Console build flag | New compile definition **`ENG_DEV_CONSOLE`** (0/1, same pattern as `ENG_EDITOR`): 1 in Debug, 0 in Release, `-DENG_DEV_CONSOLE_IN_RELEASE=ON` forces 1 | The task asks for a precompile directive so the release build has no console |
| B‑D3 | Console vs editor flags | They are independent. Editor commands are registered only when `ENG_EDITOR` is 1. Game commands (`restart`, `win`, …) only need `ENG_DEV_CONSOLE` | A debug build can have the console without the editor (e.g. for playtests) |
| B‑D4 | Order of work | Console **before** slimming the editor GUI (B‑D12 puts the editor phase after the arena) | No step leaves the developer without a way to edit a level |
| B‑D5 | Gun | `ScalingGun` becomes a generic **hitscan `Gun`**: cooldown, centre ray, `ShotFiredEvent`. No damage yet: it reports what it hit | "Keep the gun and the shoot interaction". Damage needs enemies, which is the team's next step (§9) |
| B‑D6 | Shot visuals | Keep the beam as a short **tracer** (one colour) plus the muzzle and impact bursts | They already exist and read well. The stretch/shrink colours go |
| B‑D7 | Levels | Keep `LevelList`, `GameplayManager`, `NextLevelTrigger` and `LevelProgressedEvent`. The list has **one** entry: the arena. `level_02.lvl` and `level_03.lvl` are deleted | "Only one level, but keep the functionality to add more levels and transitions" |
| B‑D8 | Arena geometry | **One render actor** with a new primitive `cube_inverted` (normals and triangle winding flipped, scaled 16×6×16) **plus six static box colliders** (floor, ceiling, 4 walls) as child actors with no renderer | An inverted‑normals cube is seen from the inside. Bullet needs solid slabs (a triangle mesh is not saved in level files, D12) |
| B‑D9 | Arena size vs player size | The arena is **16×6×16** (developer decision; the task's first draft said 8×2×8, which a 2 m player could not stand in). The player keeps its current size: 2 m capsule (radius 0.5, cylinder 1.0), eye 1.7 m, 1 m jump. The size is level data, so changing it later is an edit of `arena_01.lvl` | A 6 m ceiling leaves room for the current controller and for jumping enemies |
| B‑D10 | Game class name | `ScaleGame` → **`ArenaGame`**. Window title, settings folder and package name: **"Boomer Shooter"** / `BoomerShooter` (placeholder until the team names the game) | The old name describes the removed mechanic |
| B‑D11 | Win / lose triggers | Keep `TriggerZone`, `WinTrigger`, `LoseTrigger`, `NextLevelTrigger` registered. The arena uses none of them | They are generic flow tools that future levels will need. Removing them would remove "transitions" |
| B‑D12 | Editor phase scope and order | The editor phase **slims** the editor (B‑D1), it does not remove its GUI. It runs **after** the arena (order B3 arena, B4 slim editor, B5 release). Load is console only (`load [path]`): no toolbar button, no Ctrl+L. No Ctrl+R (toolbar Reset and `reset` stay) | Developer review of the first B3 draft: its "Done when" was too aggressive and editor capabilities are still useful; the goal is only to move away from a Unity‑like tree and inspector |

---

## 1. What changes, at a glance

### 1.1 Remove (and simplify)

| What | Where | Notes |
|---|---|---|
| `Scalable` component | `game/props/` | Folder deleted. Unregistered from the `ComponentRegistry` |
| Scale factor x0.25 / x0.5 / x0.75, `ChangeFactor`, `ScaleFactorChangedEvent` | `game/state/PlayerState` | `PlayerState` keeps walk speed, cooldown, reach |
| `AltFire` (RMB) and `Scroll` (wheel) actions | `game/player/PlayerInput` | `Fire` stays. The fire sign (±1) goes |
| `ScalingGun`, `ScalingShotEvent`, `IsAimingAtScalable` | `game/player/` | Become `Gun`, `ShotFiredEvent`, `GetAimHit()` (B‑D5) |
| Stretch/shrink colours, sign‑tinted muzzle FX | `game/fx/RayBeam*`, `MuzzleFX` | Become a one‑colour tracer (B‑D6). `RayBeamSpawner` → `ShotFXSpawner`, `RayBeam` → `Tracer` |
| Factor light on the gun | `game/player/GunView` | The gun keeps kick, bob and the cooldown dimming |
| Factor text, pips, "green on Scalable" crosshair, scaling hints | `game/ui/GunHUD` | Crosshair + cooldown ring stay. The crosshair turns red on any hit actor in reach (placeholder for "on an enemy") |
| Stretch/shrink sounds, `PlayScaling` | `game/state/SoundFXBank`, `assets/audio/sfx_stretch.wav`, `sfx_shrink.wav` | Bank keeps a `PlayShot()` with the existing ping (placeholder) |
| The editor's prop decorator use (`Add Cube` adds `Scalable`) | `ArenaGame::RegisterComponents` | `Editor::SetPropDecorator` itself stays (generic) |
| `level_02.lvl`, `level_03.lvl`; puzzle content of `level_01.lvl` | `assets/levels/` | Replaced by `arena_01.lvl` (B‑D7, B‑D8) |
| **Editor GUI (part):** `HierarchyPanel`, `InspectorPanel`, `InspectorLayout`, `EditorBannerScreen`, the toolbar's **Load** button | `engine/editor/`, `engine/ui/MenuScreens` | B‑D1. The gizmo, the toolbar and click‑select stay; `EditorUI.h` keeps only the toolbar / status layout. A one‑line status text replaces the banner |
| `Component::DrawInspector(InspectorLayout&)` and its 6 overrides | `engine/level/Component.h`, `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor`, (`Scalable`) | Fields are edited through `PropertyBag` instead (§5.4) |
| "Build to Scale" names | `ScaleGame`, title, `Settings` path, `tools/package_release.sh`, README, `assets/README.md` | B‑D10 |

### 1.2 Keep (unchanged or nearly)

- **Engine:** core, modes (MainMenu / Play / Paused / Editor), input, resources, rendering, level + serializer +
  registry + `ActorHandle`, physics (Bullet, `CharacterMotor`, raycast, `OverlapBox`, triggers), audio, particles,
  UI stack and menus (main / pause / message / settings).
- **First‑person controller:** `PlayerActor`, `FirstPersonController`, `FirstPersonLook`, `PlayerInput` (minus the
  scaling actions), `CharacterMotor`. Unchanged (B‑D9).
- **Gun and shooting:** `Gun` (was `ScalingGun`), `GunView`, `ShotFXSpawner` + `Tracer` + `MuzzleFX`, the fire buffer.
- **In‑game HUD and GUI:** `GunHUD` (crosshair, cooldown ring), level label, `GameUI` (win / lose dialogs),
  `SettingsScreen`, `MusicManager`, main and pause menus.
- **Level flow:** `GameState` (Idle / Playing / Won / Lost), `GameplayManager`, `LevelList`, triggers, `GameContext`.
- **Editor capabilities** (B‑D1): `Editor` (enter/exit, selection, add/duplicate/delete, spawn, save/load),
  `EditorCamera`, `Picker` (click‑select), `ActorEdit`, `LevelSnapshot`, `TransformGizmo` (W / R),
  `EditorToolbar` (without Load), selection highlight, Bullet wireframes, grid.

### 1.3 Add

| What | Where | Notes |
|---|---|---|
| `DevConsole`, `ConsoleCommands`, `ConsoleScreen`, `ConVar` | `engine/console/` (new) | Compiled only when `ENG_DEV_CONSOLE` is 1 (§5) |
| Editor console commands | `engine/console/EditorCommands.cpp` | Registered only when `ENG_EDITOR` is 1 |
| Game console commands | `game/dev/GameCommands.cpp` (new) | `restart`, `win`, `lose`, `tp`, `god`, … (§5.5) |
| `Editor::SetComponentField`, `Editor::AddComponent`, `Editor::RemoveComponent`, `Editor::PickCenter` | `engine/editor/Editor` | What the inspector and the hierarchy did, without the GUI (§5.4) |
| `Component::OnFieldsChanged()` | `engine/level/Component.h` | Lets `Collider` / `RigidBody` rebuild their Bullet objects after a console edit (done before inside `DrawInspector`) |
| `Primitive::CubeInverted` (`mesh=cube_inverted` in level files) | `engine/resources/ResourceManager` | A unit cube with flipped normals and winding |
| `arena_01.lvl` | `assets/levels/` | 16×6×16 inverted cube + 6 collider slabs + spawn |
| `ENG_DEV_CONSOLE` definition and `ENG_DEV_CONSOLE_IN_RELEASE` option | `CMakeLists.txt` | B‑D2 |

---

## 2. Repository layout (target)

Only the parts that change are shown. `-` removed, `+` added, `~` changed.

```
├── CMakeLists.txt                  ~ ENG_DEV_CONSOLE (Debug 1 / Release 0), option ENG_DEV_CONSOLE_IN_RELEASE
├── tools/package_release.sh        ~ dist/BoomerShooter/
├── assets/
│   ├── levels/
│   │   ├── arena_01.lvl            + 16x6x16 inverted cube, 6 static slabs, spawn
│   │   ├── level_01.lvl            - (puzzle)
│   │   ├── level_02.lvl            -
│   │   └── level_03.lvl            -
│   └── audio/                      - sfx_stretch.wav, sfx_shrink.wav   (CREDITS.txt updated)
└── src/
    ├── main.cpp                    ~ new ArenaGame
    ├── engine/
    │   ├── eng.h                   ~ drops the GUI editor headers, adds console/ (guarded)
    │   ├── console/                + DevConsole, ConsoleCommands, ConVar, ConsoleScreen, EditorCommands.cpp,
    │   │                           +   EngineCommands.cpp, ConsoleArgs
    │   ├── level/Component.h       ~ - DrawInspector, + OnFieldsChanged
    │   ├── resources/              ~ + Primitive::CubeInverted
    │   ├── modes/ModeMachine       ~ console open: TAB / ESC belong to the console; no EditorBannerScreen
    │   ├── ui/MenuScreens          ~ - EditorBannerScreen
    │   └── editor/                 ~ keeps Editor, EditorCamera, Picker, ActorEdit, LevelSnapshot, TransformGizmo,
    │                               ~   EditorToolbar (no Load), EditorUI (toolbar + status layout only)
    │                               - HierarchyPanel, InspectorPanel, InspectorLayout
    └── game/
        ├── ArenaGame.h/.cpp        ~ was ScaleGame
        ├── dev/GameCommands.cpp    + (whole file inside #if ENG_DEV_CONSOLE)
        ├── state/                  ~ PlayerState (no factors), SoundFXBank (PlayShot), Settings (new folder name)
        ├── player/                 ~ Gun (was ScalingGun), GunView, PlayerInput, PlayerActor (Gun instead of ScalingGun)
        ├── props/                  - Scalable (folder removed)
        ├── fx/                     ~ ShotFXSpawner (was RayBeamSpawner), Tracer (was RayBeam), MuzzleFX
        ├── flow/                   = GameplayManager, GameContext, TriggerZone (+ Win / Lose / NextLevel)
        ├── ui/                     ~ GunHUD (crosshair + cooldown), GameUI (texts), SettingsScreen
        └── audio/                  = MusicManager
```

---

## 3. Architecture after the change

### 3.1 Layers

```
+=====================================================================================+
|                APPLICATION LAYER   (src/game, "Boomer Shooter" base)                  |
|                                                                                     |
|   ArenaGame : eng::Application  (registers components and commands, owns the systems) |
|   +----------------+  +----------------+  +-----------------+  +-----------------+  |
|   | state/         |  | flow/          |  | player/         |  | fx/             |  |
|   | GameState      |  | GameplayManager|  | PlayerInput     |  | ShotFXSpawner   |  |
|   | PlayerState    |  | GameContext    |  | PlayerActor     |  | Tracer          |  |
|   | Settings       |  | TriggerZone -> |  | FirstPerson-    |  | MuzzleFX        |  |
|   | SoundFXBank    |  |  Win / Lose /  |  |  Controller/Look|  +-----------------+  |
|   | LevelList (1)  |  |  NextLevel     |  | Gun  GunView    |  | audio/ Music-   |  |
|   +----------------+  +----------------+  +-----------------+  |   Manager       |  |
|   | ui/ GameUI  GunHUD  SettingsScreen |  | dev/ GameCommands  [ENG_DEV_CONSOLE] |  |
|   +------------------------------------+  +--------------------------------------+  |
|          ( future, §9:  ArenaTimer · WaveDirector · Enemy · Health · Damage )        |
+==========================================^==========================================+
                                           |  #include <eng.h>
+==========================================|==========================================+
|                               ENGINE   (src/engine, namespace eng)                   |
|   CORE  Engine · Time · WindowInfo · World · EventBus · Log · EngineInfo              |
|   ModeMachine   MainMenu <-> Play <-> Paused          Play <--TAB--> Editor          |
|   Input · Resources (+ cube_inverted) · Render · Level · Physics · Audio · FX · UI    |
|                                                                                     |
|   +-------------------------------------+   +-----------------------------------+   |
|   | Editor (ENG_EDITOR=1)  NO PANELS     |   | DevConsole (ENG_DEV_CONSOLE=1)    |   |
|   | EditorCamera · Picker · ActorEdit ·  |<--| ConsoleScreen (UIStack overlay)   |   |
|   | LevelSnapshot · gizmo · toolbar ·    |   | ConsoleCommands · ConVar · Log    |   |
|   | selection · hotkeys · status line    |   | sink · EngineCommands ·           |   |
|   +-------------------------------------+   | EditorCommands (ENG_EDITOR=1)     |   |
|                                             +-----------------------------------+   |
+=====================================================================================+
|   raylib 5.6 + raymath + rlgl + raygui            |   Bullet 3.25 (static)          |
+=====================================================================================+
```

Rules that do not change: the engine never includes `src/game`; the game never includes Bullet; Bullet stays in
`src/engine/physics/*.cpp`. New rule: **nothing outside `engine/console/` and `game/dev/` names a console type**,
except inside `#if ENG_DEV_CONSOLE` blocks (§5.6).

### 3.2 Ownership (changes only)

```
Engine
  ├─ ... (unchanged subsystems)
  ├─ Editor          (by value, always compiled; only reachable with ENG_EDITOR=1)
  │                    owns EditorCamera, LevelSnapshot, TransformGizmo, EditorToolbar   (- hierarchy, - inspector)
  └─ DevConsole      (by value, only when ENG_DEV_CONSOLE=1)
                       owns the command table, the ConVar table, the scroll-back, the input history
                       NON-owning: the ConsoleScreen on the UIStack (the UIStack owns it while open)

ArenaGame            (was ScaleGame)
  ├─ eng::Level, eng::Camera, ActorHandle m_player
  ├─ PlayerState, PlayerInput, GameState, GameplayManager, ShotFXSpawner, GameUI, GunHUD, GunView, MusicManager
  └─ [ENG_DEV_CONSOLE] the game's command registrations (tokens unregistered in Destroy)
```

New rules:
10. A console command **never clears or replaces the level directly.** Level commands (`level_load`, `restart`, …)
    make the same *requests* the menus make (ownership rule 8): `GameplayManager` requests, `Editor::RequestLoad`.
11. Console edits of physics actors go through `ActorEdit` / `Editor::SetComponentField`, never straight to a
    component's fields, so bodies stay in step with the actor (same rule the gizmo and inspector followed).
12. The game unregisters its console commands in `Destroy()` (the commands capture `this`).

### 3.3 Engine modes

The four modes stay. The console is **not** a mode. It is an overlay screen on the `UIStack`, open in any mode.

```
                 +------------+
   Boot ───────> |  MainMenu  | <──────────────────────────────┐
                 +------------+                                │
                  │ Start                                      │ "Main Menu"
                  v                                            │
                 +------------+   ESC / focus lost   +------------+
                 |    Play    | ───────────────────> |   Paused   |
                 +------------+ <─────────────────── +------------+
                  ^        │ TAB (editor builds)
                  │ TAB    v
                 +------------+
                 |   Editor   |   frozen, free camera, gizmo, toolbar, status line; NO tree / inspector (console instead)
                 +------------+

   ` (grave)  in ANY mode ──> ConsoleScreen pushed on the UIStack   (ENG_DEV_CONSOLE builds)
              while open:  the cursor is free · keys go to the console · TAB = autocomplete · ESC / ` closes it
                           the simulation is frozen (cvar con_pause 1, default) or keeps running (con_pause 0)
```

| Mode | Change |
|---|---|
| MainMenu | The **Editor** button stays (editor builds). No other change |
| Play | Console may be open on top. While it is open the `PlayerInput` control gate is off |
| Paused | No change |
| Editor | No hierarchy, no inspector, no banner. Kept: the gizmo, the toolbar (Play · Reset · Move · Scale · Physics · Save; no Load) and click‑select. The `Editor` draws one status line (mode, selection, hotkeys) with `DrawText`. Hotkeys: LMB pick / gizmo drag, RMB+WASD/QE fly, W move, R scale, F focus, Del delete, Ctrl+D duplicate, Ctrl+S save, F3 wireframes. Load is console only (`load`) |

`ModeMachine::IsSimulating()` becomes `mode == Play && !(console open && con_pause)`. The fixed‑step accumulator
already advances only while simulating, so nothing builds up while the console is open.

---

## 4. Game loop (changes only)

```
Engine::Run()
│  ├─ 1. FRAME START   time / window / input.Poll()
│  ├─ 2. MODES         [ENG_DEV_CONSOLE] console.HandleToggle(input)   ` opens / closes; while open it owns
│  │                                                                     the keyboard (char queue, ESC, TAB)
│  │                   modes.HandleInput()   TAB / ESC ignored while the console is open
│  │                   modes.HandleFocus(); modes.ApplyPendingTransition()
│  ├─ 3. SIMULATE      isSimulating = Play && !(console open && con_pause)
│  │                   level.FixedUpdate -> FirstPersonController (walk, jump), Gun (cooldown, shot)
│  │                   ...
│  │                   if EDITOR: editor.Update()        fly, gizmo, pick, hotkeys, pending load   (no tree / inspector)
│  │                   app->Update()
│  │                   ui.Update()                        ConsoleScreen: text input, history, autocomplete,
│  │                                                      runs the submitted line -> DevConsole::Execute
│  ├─ 5. RENDER        ... app->DrawViewModel(); app->DrawHUD()
│  │                   if EDITOR: editor.DrawPanels()     toolbar only;  editor.DrawStatus()  one text line (was the banner)
│  │                   ui.Draw()                          menus, dialogs, ConsoleScreen on top
│  └─ 6. FRAME END     events.Dispatch()  ShotFiredEvent / GameWon / GameLost / NextLevelRequested ...
```

Commands run inside `ui.Update()`, i.e. after the simulation and before rendering. Commands that change the level
only make requests (rule 10); the request is applied by `GameplayManager::Update` / the editor's next frame, as today.

---

## 5. The developer console

### 5.1 What it is

A drop‑down text console in the Quake style, drawn with raygui/raylib over the top half of the screen:
a scroll‑back of log lines and command output, one input line, history, and autocomplete. Engine, editor and game
register commands by name. **The whole feature is compiled out in release builds.**

### 5.2 Classes

| Class | Folder | Responsibility |
|---|---|---|
| `DevConsole` | console | Owned by `Engine`. Open / close (toggle key `KEY_GRAVE`), `Execute(line)`, scroll‑back (ring buffer, 512 lines), history (64 lines, ↑ / ↓), `Print(fmt, ...)`. Installs a `Log` sink so every `Log::Info/Warning/Error` line appears in it, coloured by level |
| `ConsoleCommands` | console | Table name → `{help, usage, handler, completer}`. `Register(...) -> token`, `Unregister(token)`, `Find`, `Complete(prefix)`. Names are lowercase, `snake_case` |
| `ConsoleArgs` | console | Tokenized line (quotes keep spaces: `select "Crate A"`). `Count`, `GetString(i)`, `GetInt(i, out)`, `GetFloat(i, out)`, `GetVector3(i, out)` (`1 2 3` or `1,2,3`). Handlers print usage on a bad argument |
| `ConVar` | console | Named value bound to a variable (`bool / int / float / string`), with min / max. `name` prints it, `name value` sets it. Examples: `con_pause`, `timescale`, `phys_debug`, `god` |
| `ConsoleScreen` | console | `UIScreen` pushed on the `UIStack` while open. Not opaque (the game shows below). Frees the cursor on push, restores the lock on pop. Reads chars with `GetCharPressed()`, draws with raylib text (no raygui text box, so the console has no focus fights) |
| `EngineCommands.cpp` | console | Registers the engine's commands (§5.3) at `Engine::Init` |
| `EditorCommands.cpp` | console | Registers the editor's commands (§5.4), only inside `#if ENG_EDITOR` |

`Engine` gets `DevConsole& GetConsole()` only inside `#if ENG_DEV_CONSOLE`.

### 5.3 Engine commands

| Command | What it does |
|---|---|
| `help [prefix]` | Lists commands and cvars (with usage) |
| `clear` · `echo <text>` | Clears the scroll‑back · prints |
| `quit` | `Application::SetNeedsToBeClosed(true)` |
| `mode [menu\|play\|pause\|editor]` | Prints or requests a mode (`ModeMachine::Request`, guarded as usual) |
| `timescale <x>` (cvar) | `Time` scale, 0–4 |
| `con_pause 0\|1` (cvar) | Freeze the simulation while the console is open (default 1) |
| `phys_debug 0\|1` · `grid 0\|1` | Bullet wireframes · debug grid (were F3 / F2) |
| `fps` | Toggles an FPS / frame‑time line |
| `exec <file>` | Runs a text file of commands, one per line (`assets/dev/*.cfg`) — handy for test setups |

### 5.4 Editor capabilities as commands (`ENG_EDITOR` builds)

Everything the panels did, one command each. The selection is the editor's selection (one actor), set by click,
by `pick` (screen centre, works in Play too) or by name / id. The gizmo and the toolbar stay (B‑D1), so transform,
Play, Reset and Save work both ways; the tree, the inspector and Load are console only.

| Old GUI | Command | Notes |
|---|---|---|
| TAB / toolbar **Play** | `editor` / `play` | Same `ModeMachine` requests |
| Hierarchy tree | `ls [filter]` | `id  name  (parent)  components` |
| Click in tree / viewport | `select <id\|"name">` · `pick` · `deselect` | `pick` = `Picker` with the camera's centre ray |
| Inspector header | `info` | Transform + every component's `PropertyBag` (`Serialize`) |
| Rename field | `rename "<new name>"` | |
| Transform fields / gizmo W | `pos x y z` · `move dx dy dz` | Through `ActorEdit::SetLocalTransform` (teleports bodies) |
| Rotation fields | `rot pitch yaw roll` | Euler degrees, as the inspector did |
| Scale fields / gizmo R | `scale x y z` · `scale s` | `ActorEdit` → `Collider::SetScale` + `RigidBody::OnScaleChanged` |
| Active checkbox | `active 0\|1` | |
| Parent | `parent <id\|none>` | Keeps the world transform |
| Component fields | `set <Component> key=value [key=value ...]` · `get <Component>` | §5.4.1 |
| **Add Component** | `comp_add <Type>` · `comp_rm <Type>` · `comp_types` | `ComponentRegistry` names; autocompleted |
| Add Cube / Add Trigger | `add cube` · `add trigger` · `add empty` | 4 m in front of the camera, as before |
| Duplicate / Delete | `dup` · `del` | Also Ctrl+D / Del |
| Set Spawn Here | `spawn_here` · `spawn x y z [yaw]` | |
| Reset | `reset` | `LevelSnapshot::Restore` (same limits as today) |
| Save / Load | `save [path]` · `load [path]` | Default path: `Application::GetLevelPath()`; load is a request (rule 10) |
| Physics toggle | `phys_debug` | |
| — (new) | `focus` | Editor camera to the selection (F) |

#### 5.4.1 Editing component fields without the inspector

The inspector wrote fields inside `DrawInspector` and then called e.g. `Collider::Rebuild()`. Without it:

1. `Editor::SetComponentField(actor, type, key, value)` → `component->Serialize(bag)`; `bag.Set(key, value)`;
   `component->Deserialize(bag)`; `component->OnFieldsChanged()`.
2. `Component::OnFieldsChanged()` is a new virtual (default: nothing). `Collider` re‑creates its shape and asks the
   `RigidBody` / `TriggerVolume` / `CharacterMotor` on the actor to rebuild; `RigidBody` re‑applies mass, friction,
   damping, CCD; `MeshRenderer` reloads the mesh and material. This is the code that lived at the end of each
   `DrawInspector` override, moved.
3. A key the component did not write in step 1 is reported as unknown and nothing is changed.

This keeps a single description of every component's fields (`Serialize / Deserialize`) for files, `info`, `set`
and `dup`.

### 5.5 Game commands (`game/dev/GameCommands.cpp`)

| Command | What it does |
|---|---|
| `restart` | `GameplayManager` reload request |
| `level_list` · `level_load <index\|path>` · `level_next` | Level flow by request (keeps "transitions" testable with one level) |
| `win` · `lose` | `GameState::WinGame / LoseGame` (tests `GameUI`) |
| `tp x y z` · `tp_spawn` | `CharacterMotor::Warp` the player |
| `god 0\|1` (cvar) | Placeholder flag on `PlayerState`, read by future damage code (§9) |
| `noclip 0\|1` | Player collider mask none + fly (uses the `EditorCamera` fly code path) — optional, see progress file |
| `gun_cooldown <s>` · `gun_reach <m>` | `PlayerState` tuning |
| `sens <x>` | Mouse sensitivity (`Settings`) |

The team adds `timer_*`, `wave_*`, `enemy_*` commands here as §9 is built.

### 5.6 Compiling it out

```cmake
option(ENG_DEV_CONSOLE_IN_RELEASE "Compile the developer console into non-Debug builds" OFF)
if(ENG_DEV_CONSOLE_IN_RELEASE)
    target_compile_definitions(engine PUBLIC ENG_DEV_CONSOLE=1)
else()
    target_compile_definitions(engine PUBLIC $<IF:$<CONFIG:Debug>,ENG_DEV_CONSOLE=1,ENG_DEV_CONSOLE=0>)
endif()
```

- Every `.cpp` in `engine/console/` and `game/dev/` is wrapped in `#if ENG_DEV_CONSOLE … #endif` (the files stay
  in the `GLOB`, they compile to nothing). Headers too, so a stray include fails loudly in Release.
- Call sites outside those folders (`Engine::Init / Run`, `ModeMachine::HandleInput`, `ArenaGame::Init / Destroy`)
  are one‑line `#if ENG_DEV_CONSOLE` blocks. No macro hides a call.
- `EngineInfo` gains `IsDevConsoleBuild()` (like `IsEditorBuild()`) for logging and the HUD hint.
- **Release check:** `strings dist/BoomerShooter/BoomerShooter | grep -E "comp_add|con_pause|spawn_here"` prints
  nothing; the grave key does nothing; the binary size drops.

---

## 6. The arena level

### 6.1 Inverted cube

`ResourceManager` generates `Primitive::CubeInverted` once: `GenMeshCube(1, 1, 1)`, then for every triangle swap two
indices (winding) and negate every normal, then `UploadMesh`. Level files name it `mesh=cube_inverted`. Backface
culling stays on: from inside you see the walls, from outside you see through the near faces into the far ones
(handy for the menu orbit). The lit shader uses `matNormal`, so the non‑uniform 16×6×16 scale lights correctly.

### 6.2 `arena_01.lvl`

Interior: x −8…8, y 0…6, z −8…8 (floor top at y = 0). Slabs are 0.5 m thick and sit **outside** the interior.

```
lvl 1
spawn 0 0 6  yaw 180                                   # feet on the floor, near the back wall, facing the centre
actor "Arena"
  transform 0 3 0   0 0 0   16 6 16
  component MeshRenderer mesh=cube_inverted tint=150,140,130,255 lit=1
end
actor "Floor"
  transform 0 -0.25 0   0 0 0   17 0.5 17
  component Collider shape=box half=0.5,0.5,0.5 layer=Ground
  component RigidBody type=static
end
actor "Ceiling"   transform 0  6.25 0   0 0 0   17 0.5 17     (Collider + static RigidBody, layer Ground)
actor "Wall N"    transform 0  3 -8.25  0 0 0   17 7 0.5      (same)
actor "Wall S"    transform 0  3  8.25  0 0 0   17 7 0.5
actor "Wall E"    transform  8.25 3 0   0 0 0   0.5 7 17
actor "Wall W"    transform -8.25 3 0   0 0 0   0.5 7 17
```

(The four short blocks are written in full in the real file.) The slabs are root actors, not children of
"Arena", so scaling the render cube in the editor never scales the colliders by accident.

- `LevelList::Default()` = `{ "assets/levels/arena_01.lvl" }`.
- The menu backdrop orbit moves inside the arena: radius 6, height 4, looking at (0, 1, 0).
- No kill volume (the room is closed). No win trigger (the win rules come with the timer, §9).

### 6.3 Player size (risk R1)

The player keeps today's size: a 2.0 m capsule (radius 0.5, cylinder 1.0), eye at 1.7 m, jump height 1 m (apex at
3 m from the floor, well under the 6 m ceiling). No constant changes in `PlayerActor.h`, `FirstPersonLook.h` or
`FirstPersonController.h`. If the arena ever gets lower than ~3.5 m, these have to be revisited (the first draft's
2 m room needed a 1.6 m player).

---

## 7. Class tables (after)

### 7.1 Engine — changes

| Class | Status | Notes |
|---|---|---|
| `DevConsole`, `ConsoleCommands`, `ConsoleArgs`, `ConVar`, `ConsoleScreen` | **new** (`ENG_DEV_CONSOLE`) | §5.2 |
| `Editor` | changed | − hierarchy and inspector members; `DrawPanels` draws the toolbar only; + `DrawStatus`, `PickCenter`, `SetComponentField`, `AddComponent`, `RemoveComponent`, `Rename`, `SetParent` |
| `EditorCamera`, `Picker`, `ActorEdit`, `LevelSnapshot`, `TransformGizmo` | kept | |
| `EditorToolbar` | changed | − Load button (B‑D12) |
| `EditorUI` | changed | Toolbar + status layout only; `IsOverPanels` → `IsOverToolbar` |
| `HierarchyPanel`, `InspectorPanel`, `InspectorLayout` | **removed** | B‑D1 |
| `EditorBannerScreen` | **removed** | Replaced by `Editor::DrawStatus` |
| `Component` | changed | − `DrawInspector`; + `OnFieldsChanged` |
| `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor` | changed | − `DrawInspector`; + `OnFieldsChanged` (rebuild code moved from the inspector) |
| `ResourceManager` | changed | + `Primitive::CubeInverted` |
| `ModeMachine` | changed | No editor banner; TAB / ESC ignored while the console is open; `IsSimulating` honours `con_pause` |
| `EngineInfo` | changed | + `IsDevConsoleBuild()` |
| `Log` | changed | + one optional sink callback (the console) |

### 7.2 Game — after

| # | Class | Folder | Status | Responsibility |
|---|---|---|---|---|
| 1 | `ArenaGame` | game | renamed (`ScaleGame`) | `eng::Application`: registers components and console commands; owns level, camera and systems; menu orbit; `DrawViewModel`, `DrawHUD` |
| 2 | `GameState` | state | kept | Idle / Playing / Won / Lost + events |
| 3 | `PlayerState` | state | trimmed | Walk speed, gun cooldown (0.25 s default for a shooter; tunable), reach (100 m), `god` flag |
| 4 | `Settings` | state | kept | Folder `~/Library/Application Support/BoomerShooter/` |
| 5 | `SoundFXBank` | state | trimmed | `PlayShot()`; ready for enemy / hit sounds |
| 6 | `LevelList` | state | kept | One entry (arena) |
| 7 | `PlayerInput` | player | trimmed | Move, look, jump, fire (buffered 0.1 s), control gate |
| 8 | `PlayerActor` | player | changed | Same 2 m capsule (§6.3); components: motor, controller, look, `Gun` |
| 9 | `FirstPersonController` | player | kept | Walk, 1 m jump |
| 10 | `FirstPersonLook` | player | kept | Eye height 1.7 m |
| 11 | `Gun` | player | renamed (`ScalingGun`) | Cooldown; centre ray (mask `Ground \| Prop`, + `Enemy` later); publishes `ShotFiredEvent{muzzle, point, normal, target, hit}`; `GetAimHit()` for the crosshair |
| 12 | `GunView` | player | trimmed | Kick, bob, cooldown dimming |
| 13 | `ShotFXSpawner` | fx | renamed (`RayBeamSpawner`) | Listens to `ShotFiredEvent`: muzzle burst, tracer, impact burst, shot sound |
| 14 | `Tracer` | fx | renamed (`RayBeam`) | Short one‑colour line that travels to the hit point and removes itself (faster: 80 m/s) |
| 15 | `MuzzleFX` | fx | trimmed | One tint |
| 16 | `GameplayManager`, `GameContext`, `TriggerZone` + `WinTrigger` / `LoseTrigger` / `NextLevelTrigger` | flow | kept | B‑D7, B‑D11 |
| 17 | `GameUI` | ui | changed | Texts: "Arena cleared" / "You died" (placeholders); Next Level only when `LevelList::HasNext` |
| 18 | `GunHUD` | ui | trimmed | Crosshair (red on a hit actor in reach), cooldown ring |
| 19 | `SettingsScreen` | ui | kept | |
| 20 | `MusicManager` | audio | kept | |
| 21 | `GameCommands` | dev | **new** (`ENG_DEV_CONSOLE`) | §5.5 |
| — | `Scalable` | props | **removed** | |

Events defined by the game after the change: `GameStartedEvent`, `GameWonEvent`, `GameLostEvent`,
`LevelProgressedEvent`, `NextLevelRequestedEvent`, `ShotFiredEvent`. Removed: `ScaleFactorChangedEvent`,
`ScalingShotEvent`.

### 7.3 Controls (after)

| Input | Action |
|---|---|
| WASD / arrows · Mouse · Space | Move · Look · Jump |
| **Left click** | Fire |
| ESC | Pause / Resume (closes a dialog or the console) |
| `` ` `` (grave) | Developer console (`ENG_DEV_CONSOLE` builds) |
| TAB | Editor (editor builds); autocomplete while the console is open |
| Editor: LMB · RMB+WASD/QE · W / R · F · Del · Ctrl+D · Ctrl+S | Pick / gizmo drag · fly · gizmo move / scale · focus · delete · duplicate · save (load: console `load`) |
| F2 / F3 | Grid / Bullet wireframes (also `grid`, `phys_debug`) |

---

## 8. Implementation plan (in order)

Same workflow as before ([`tasks/TASK_02.md`](tasks/TASK_02.md)): one branch per phase from `integration`, one
commit per checkbox, the developer runs **Done when** and merges, the finished branch is renamed `done/<name>`.
Phases are numbered **B0–B5** so they never mix with phases 0–13.

### Phase B0 — Baseline (`bs_baseline`)
- Tag the current `integration` as `build-to-scale-final` (the scaling game stays reachable).
- Delete the uncommitted `level_01.lvl` edits.
- CMake: `ENG_DEV_CONSOLE` + `ENG_DEV_CONSOLE_IN_RELEASE` (§5.6); `EngineInfo::IsDevConsoleBuild()`; startup log line shows it.

**Done when:** Debug and Release build; the log says `console ON` / `console OFF`; the game plays exactly as before.

### Phase B1 — Remove the scaling mechanic (`bs_remove_scaling`)
- Delete `Scalable` (`game/props/`), its registration and the editor prop decorator; strip `component Scalable` from the level files.
- `PlayerState`: drop factors, `ChangeFactor`, `ScaleFactorChangedEvent`; `PlayerInput`: drop `AltFire`, `Scroll`, the fire sign.
- `ScalingGun` → `Gun`, `ScalingShotEvent` → `ShotFiredEvent`, `IsAimingAtScalable` → `GetAimHit`.
- `RayBeamSpawner` → `ShotFXSpawner`, `RayBeam` → `Tracer`, `MuzzleFX` one tint; `SoundFXBank::PlayShot`; delete `sfx_stretch/shrink.wav`, update `tools/generate_placeholder_audio.py` and `CREDITS.txt`.
- `GunView`: no factor light. `GunHUD`: crosshair + cooldown ring only.

**Done when:** left click fires a tracer with muzzle and impact bursts and a sound; right click and the wheel do nothing; the crosshair turns red on a crate; `grep -rn "Scal" src/game` finds nothing about scaling; levels load without warnings.

### Phase B2 — Developer console (`bs_dev_console`)
- `DevConsole`, `ConsoleCommands`, `ConsoleArgs`, `ConVar` + the `Log` sink.
- `ConsoleScreen` (toggle, input line, scroll‑back, history, autocomplete); `ModeMachine`: TAB / ESC to the console, `con_pause`.
- Engine commands (§5.3).
- Editor commands (§5.4) **using today's editor**: selection, `ls`, `info`, transform, add / dup / del, spawn, reset, save, load.
- `Component::OnFieldsChanged` + `Editor::SetComponentField` / `AddComponent` / `RemoveComponent`; `set`, `get`, `comp_add`, `comp_rm` (the inspector still exists in this phase, both paths call the same rebuild code).
- Game commands (§5.5) in `game/dev/GameCommands.cpp`, unregistered in `Destroy`.
- Release check: compiled out (§5.6).

**Done when:** in a Debug build the grave key opens the console in every mode; `help` lists the commands; `pick`, `scale 2`, `set RigidBody mass=5`, `comp_add TriggerVolume`, `save`, `load` change the level and survive a reload; `restart`, `win`, `tp 0 0 0` work; ESC closes the console without pausing; the Release binary has no console strings and the grave key does nothing.

### Phase B3 — Arena level (`bs_arena_level`)
- `Primitive::CubeInverted` (`mesh=cube_inverted`).
- `assets/levels/arena_01.lvl` (§6.2); `LevelList` with one entry; delete `level_01–03.lvl`.
- Player size stays the same (§6.3); menu orbit around the arena.
- Level transitions still testable: `level_load` of a second throw‑away file, `NextLevelTrigger` added from the console (no file committed).

**Done when:** Start spawns the player inside a 16×6×16 room whose inner faces are lit and visible; the player walks, jumps without hitting the ceiling, cannot leave the room, and the gun's tracer stops on walls, floor and ceiling; the editor shows the six slabs as wireframes; `level_next` with one level shows the last‑level dialog.

### Phase B4 — Slim the editor (`bs_slim_editor`)
B‑D1 / B‑D12: the editor stays (TAB, free camera, click‑select, gizmo, toolbar); only the Unity‑style tree and
inspector go. The hierarchy and inspector are already not drawn (`Editor::DrawPanels` draws only the toolbar), so
most of this phase deletes dead code.
- Delete `HierarchyPanel`, `InspectorPanel`, `InspectorLayout`, the `Editor` members that own them and their `eng.h` includes.
- Remove `Component::DrawInspector` and the overrides in `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor` (their rebuild code already lives in `OnFieldsChanged` since B2).
- `EditorToolbar` without the **Load** button (Play · Reset · Move · Scale · Physics · Save); `EditorUI.h` trimmed to the toolbar and status layout (`IsOverPanels` → `IsOverToolbar`).
- Delete `EditorBannerScreen`; `ModeMachine` pushes no screen for Editor. `Editor::DrawStatus` draws one line instead: `EDITOR · "Crate A" #12 · W move  R scale  F focus  Del  Ctrl+D  Ctrl+S · `` ` `` console` (`nothing selected` when empty).
- Gizmo (W / R), click‑select, selection highlight and the existing hotkeys stay unchanged. Load stays reachable through the console `load [path]` only.
- README / plan: editor section rewritten around the light editor + console.

**Done when:** TAB enters the editor showing only the toolbar (no Load button), the status line and the gizmo, with no hierarchy or inspector; click‑select, gizmo move / scale (W / R), F, Del, Ctrl+D and Ctrl+S work and the status line names the selection; from the console `select` → `info` → `set RigidBody mass=5` → `comp_add TriggerVolume` → `save` → `load` still round‑trips; `grep -rn "HierarchyPanel\|InspectorPanel\|InspectorLayout\|DrawInspector" src` finds nothing; Debug and Release build clean.

### Phase B5 — Rename, docs and release (`bs_rename_release`)
- `ScaleGame` → `ArenaGame`; title, `Settings` folder, `tools/package_release.sh` (`dist/BoomerShooter/`), README, `assets/README.md`.
- `GameUI` placeholder texts; Next Level button only when there is a next level.
- Update `ARCHITECTURE_PLAN.md` header to point here (that file stays as the history of *Build to Scale*).
- Release package; console and editor absent; scripted run: Start → arena → `win` dialog in a Debug build.

**Done when:** `tools/package_release.sh` builds `dist/BoomerShooter/`, which starts from another folder, plays the arena, has no console or editor; a Debug build has both.

---

## 9. Hand‑off: where the Boomer‑Shooter plugs in

Not part of this plan. Listed so the base is shaped for it.

| Feature | Where it goes | Engine pieces it uses |
|---|---|---|
| **5‑minute timer** (`ArenaTimer`) | `game/flow/`, ticked in `ArenaGame::FixedUpdate` while `GameState` is Playing | `Time`, `EventBus` (`TimerExpiredEvent`) |
| **Waves** (`WaveDirector`: ~3 waves, next wave when the room is clear) | `game/flow/`; wave data in the level file as a component on a "Waves" actor (spawn points = child actors) | `ComponentRegistry`, `PropertyBag`, `ActorHandle` lists |
| **Win conditions** | `GameState::WinGame` on "last wave cleared" **or** `TimerExpiredEvent` while alive; `LoseGame` on player death | Existing `GameWon / GameLost` events and `GameUI` |
| **Enemies** (`Enemy`, `Health`) | `game/enemies/`; `Collider` + kinematic or dynamic `RigidBody`; new layer `Enemy` in `PhysicsLayers` | Physics layers, raycast, `OverlapBox` |
| **Damage** | `Gun` resolves `ShotFiredEvent::target` → `Health::ApplyDamage`; `PlayerState::god` respected | `ActorHandle` (never raw pointers across frames) |
| **HUD** | `GunHUD` / new `ArenaHUD`: timer, wave n/3, health, ammo | `DrawHUD` |
| **Dev commands** | `timer_set`, `timer_pause`, `wave_start <n>`, `wave_clear`, `enemy_spawn <type>`, `kill_all` in `GameCommands.cpp` | §5 |
| **More levels** | Add `.lvl` paths to `LevelList::Default()`; `NextLevelTrigger` or "arena cleared → next" | Kept in B‑D7 |

---

## 10. Risks and notes

| # | Risk | Mitigation |
|---|---|---|
| R1 | **Arena height vs player size** (the first 8×2×8 draft could not fit the 2 m player) | Resolved: the arena is 16×6×16 and the player keeps its size (B‑D9, §6.3). Revisit only if the room gets lower than ~3.5 m |
| R2 | Removing `DrawInspector` loses the rebuild logic for colliders and bodies | Moved to `OnFieldsChanged` in B2, while the inspector still exists, so both paths can be compared before B4 deletes the inspector |
| R3 | Console text input fights other input (WASD moves the player, TAB toggles the editor, ESC pauses) | The console owns the keyboard while open: `PlayerInput` gate off, `ModeMachine` ignores TAB / ESC, editor hotkeys quiet (`Editor::IsTyping` already exists) |
| R4 | Console code leaks into release (a call site without `#if`) | Headers are guarded too, so a stray use fails to compile in Release; the `strings` check in B2 and B5 |
| R5 | Commands that clear the level from inside `ui.Update` | Rule 10: commands make requests only |
| R6 | Inverted cube: the sun light and specular look wrong inside a closed box (no shadows) | Acceptable for a placeholder; ambient carries the ceiling. Real arena art replaces it |
| R7 | Fast player against thin walls: tunnelling through slabs at high speed | Slabs are 0.5 m thick, player CCD is on; the vertical speed clamp stays |
| R8 | Old `.lvl` files with `component Scalable` | The loader warns and skips unknown components; B1 strips them anyway |
| R9 | Settings folder rename loses saved settings | Acceptable (new game); noted in the README |
| R10 | Losing editor features while the tree and inspector go | B‑D4 order (console first, validated in B2); B4 keeps the gizmo, the toolbar and click‑select, and its Done when re‑runs a console round trip |

Open from the previous plan and **still open**: editor Reset cannot bring back a deleted actor (`LevelSnapshot`);
mesh colliders are not saved in level files; audio assets are placeholders; the binary is unsigned.
