# Development Progress — Boomer‑Shooter base

Tracks the implementation of [`Boomer_Shooter_ARCHITECTURE_PLAN.md`](Boomer_Shooter_ARCHITECTURE_PLAN.md). Phase names
and **Done when** criteria match §8 of that plan. The history of *Build to Scale* (phases 0–13, decisions D1–D14)
stays in [`DEV_PROGRESS.md`](DEV_PROGRESS.md).

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- Add a line to the **Session log** at the end of every work session. Add a **Decision** (B‑Dn) whenever the plan changes.
- Branches: one per phase from `integration`, one commit per checkbox (except **Done when**, checked by the developer); a finished branch is renamed `done/<branch>` ([`tasks/TASK_02.md`](tasks/TASK_02.md)).
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| B0 | Baseline | `bs_baseline` | ✅ | 2026-11-18 | 2026-11-18 | Tag, `ENG_DEV_CONSOLE` flag |
| B1 | Remove the scaling mechanic | `bs_remove_scaling` | ✅ | 2026-11-18 | 2026-11-18 | `Scalable` out, `ScalingGun` → `Gun` |
| B2 | Developer console | `bs_dev_console` | ✅ | 2026-11-18 | 2026-11-18 | Before the GUI goes (B‑D4) |
| B3 | Arena level | `done/bs_arena_level` | ✅ | 2026-11-18 | 2026-11-18 | 16×6×16 inverted cube, one level |
| B4 | Slim the editor | `bs_slim_editor` | 🟡 | 2026-11-18 | | | Tree + inspector out; gizmo, click, toolbar (no Load) stay (B‑D12) |
| B5 | Rename, docs and release | `bs_rename_release` | ⬜ | | | `ArenaGame`, "Boomer Shooter" |

**Current focus:** B4 Slim the editor
**Branch:** `bs_slim_editor`

---

## Phase checklists

### Phase B0 — Baseline ✅
- [x] Tag `integration` as `build-to-scale-final` (annotated tag on `2a12cbb`)
- [x] Developer decides on the uncommitted `assets/levels/level_01.lvl` playtest edits (commit, or discard; the file is deleted in B3, arena) (+ **discarded** by the developer; `CLAUDE.md` edits left uncommitted on purpose)
- [x] CMake: `ENG_DEV_CONSOLE` (Debug 1 / Release 0) + `ENG_DEV_CONSOLE_IN_RELEASE` option; `eng::IsDevConsoleBuild()`; startup log shows `console ON/OFF` (+ smoke runs: Debug logs `editor ON | console ON`, Release `editor OFF | console OFF`)
- [x] **Done when:** Debug and Release build; the log shows the console flag; the game plays exactly as before. 
Confirmed: 
- Ran `cmake -S . -B build/release-dev -DCMAKE_BUILD_TYPE=Release -DENG_EDITOR_IN_RELEASE=ON -DENG_DEV_CONSOLE_IN_RELEASE=ON` `cmake --build build/release-dev -j`, `./build/release-dev/my_app`
- `INFO: ENG: Raylib Engine 0.0.1 | screen 1280 x 720 | render 1280 x 720 | editor ON | console ON | audio ON`

### Phase B1 — Remove the scaling mechanic ✅
- [x] Delete `game/props/Scalable`, its registration and the editor prop decorator; strip `component Scalable` from the `.lvl` files (+ the gun's crosshair check now means 'any actor on layer Prop' until the `Gun` task; the puzzle levels can no longer be won, they are replaced in B3, arena)
- [x] `PlayerState` without scale factors / `ChangeFactor` / `ScaleFactorChangedEvent`; `PlayerInput` without `AltFire`, `Scroll` and the fire sign (+ `PlayerInput` no longer needs a `PlayerState`; `ConsumeFire()` returns a bool; the factor text / pips in `GunHUD` and the factor shade of the gun light had to go in the same commit to compile)
- [x] `ScalingGun` → `Gun` (`ShotFiredEvent`, `GetAimHit`); cooldown and reach become shooter defaults (0.25 s, 100 m) (+ reach and cooldown are `PlayerState` values with setters, ready for the console's `gun_reach` / `gun_cooldown`; `GetAimHit()` returns a `GunAim` (`isTarget` = layer Prop until there are enemies))
- [x] `RayBeamSpawner` → `ShotFXSpawner`, `RayBeam` → `Tracer` (one colour, 80 m/s), `MuzzleFX` one tint (+ warm orange colour shared through `MuzzleFX::START_COLOR / END_COLOR`; tracer faint at the tail, bright at the head)
- [x] `SoundFXBank::PlayShot`; remove `sfx_stretch.wav` / `sfx_shrink.wav`, update the audio generator script and `CREDITS.txt` (+ `Sfx` has only `Shot` (the ping, ±5 % random pitch); the generator's `chirp` is gone, music output unchanged)
- [x] `GunView` without the factor light; `GunHUD` = crosshair (red on a hit actor in reach) + cooldown ring (+ muzzle tip uses the shot colour; README controls: LMB fires, RMB / wheel gone (the intro text is rewritten in B5))
- [x] **Done when:** LMB fires tracer + muzzle + impact + sound; RMB and wheel do nothing; crosshair turns red on a crate; no scaling code left in `src/game`; levels load without warnings

### Phase B2 — Developer console ✅
- [x] `DevConsole` (owned by `Engine`), `ConsoleCommands` (register / unregister tokens, completion), `ConsoleArgs`, `ConVar`; `Log` sink into the scroll‑back (+ `Log::SetSink` is the hook for the scroll-back; cvars register as commands (`RegisterVar`); the sink skips Debug lines)
- [x] `ConsoleScreen` on the `UIStack`: grave toggle, input line, scroll‑back, history ↑/↓, TAB autocomplete; cursor free while open (+ the screen re-pushes itself on top when a mode change pushes its own screen (`KeepOnTop`); the typed line and the scroll survive that)
- [x] `ModeMachine`: TAB / ESC belong to the console while open; `IsSimulating` honours `con_pause`; `PlayerInput` gated (+ `ModeMachine::AcceptsGameInput()` = Play and no console: `PlayerInput` reads it; `IsSimulating()` honours `con_pause`; editor hotkeys are quiet while the console is open)
- [x] Engine commands: `help`, `clear`, `echo`, `quit`, `mode`, `timescale`, `con_pause`, `phys_debug`, `grid`, `fps`, `exec` (+ `phys_debug`, `grid`, `fps` toggle with no argument; `timescale` is a command that clamps to 0–4; `con_pause` is a real cvar; `exec` searches `name`, `name.cfg`, `assets/dev/name[.cfg]`, nesting ≤ 8)
- [x] Editor commands (selection and actors): `editor`, `play`, `ls`, `select`, `pick`, `deselect`, `info`, `rename`, `pos`, `move`, `rot`, `scale`, `active`, `parent`, `add`, `dup`, `del`, `focus`, `spawn_here`, `spawn`, `reset`, `save`, `load` (+ `ls` / `select` / `pick` / `info` work in any mode, the rest need the editor; `pick` ignores the player layer; `save [path]` / `load [path]`; `Reset` no longer redraws the panels)
- [x] `Component::OnFieldsChanged` (rebuild code shared with the inspector), `Editor::SetComponentField / AddComponent / RemoveComponent`; commands `set`, `get`, `comp_add`, `comp_rm`, `comp_types` (+ `set` takes several `key=value` pairs and checks them all before applying; only keys the component serializes can be set, so `RigidBody lockRotation` and `MeshRenderer visible` are now always written; the API is `SetComponentFields`; `GameActor::RemoveComponent` is new; `comp_rm Collider` is refused while a body / trigger / motor needs it; `comp_add` refuses a second component of a type)
- [x] Game commands in `game/dev/GameCommands.cpp`: `restart`, `level_list`, `level_load`, `level_next`, `win`, `lose`, `tp`, `tp_spawn`, `god`, `gun_cooldown`, `gun_reach`, `sens` (`noclip` optional); unregistered in `Destroy` (+ `GameCommands` class registered in `ScaleGame::Init`; `noclip` left out (optional); `level_next` on the last level only prints a message: the last-level dialog is B3's (arena) check; `god` is `PlayerState::GetGodFlag()`)
- [x] Compile‑out check: every console `.cpp`/`.h` guarded; Release `strings` check finds no command names (+ `tools/check_release_console.sh`: Release has no command names, release-dev has them; clean Release build from scratch, no warnings)
- [x] **Done when:** grave opens the console in every mode; `help` lists everything; `pick` → `scale 2` → `set RigidBody mass=5` → `comp_add TriggerVolume` → `save` → `load` round‑trips; `restart`, `win`, `tp 0 0 0` work; ESC closes the console without pausing; Release has no console

### Phase B3 — Arena level ✅
- [x] `Primitive::CubeInverted` (flipped winding and normals), `mesh=cube_inverted` in level files (+ `GenMeshCube` uploads the outward faces first, so the flipped CPU data is written into those GPU buffers with `rlUpdateVertexBuffer` / `rlUpdateVertexBufferElements`; `MeshRenderer` name `cube_inverted`)
- [x] `assets/levels/arena_01.lvl`: render cube 16×6×16 + six 0.5 m static slabs (layer Ground) + spawn; `LevelList` = arena only; delete `level_01–03.lvl` (+ spawn at `yaw 0` (looks along −Z, to the centre) instead of the plan's `yaw 180`, which faced the back wall; the uncommitted `level_01.lvl` edits went with the file)
- [x] Player size unchanged (2 m capsule, eye 1.7 m, 1 m jump): check it against the 6 m ceiling (plan §6.3) (+ checked by reading the constants: head about 2.5 m at the jump apex, no constant changed; no commit)
- [x] Menu orbit inside the arena (radius 6, height 4) (+ `ORBIT_RADIUS` 6, `ORBIT_HEIGHT` 4, looks at (0, 1, 0))
- [x] **Done when:** Start spawns the player in a lit 16×6×16 room seen from inside; walk and jump without touching the ceiling; can't leave the room; tracers stop on every face; editor shows six slab wireframes; `level_next` with one level shows the last‑level dialog

### Phase B4 — Slim the editor 🟡
- [x] Delete `HierarchyPanel`, `InspectorPanel`, `InspectorLayout`, the `Editor` members that own them, their `eng.h` includes (+ done in one commit with the next line: the component overrides need `InspectorLayout`, and the panels are its only caller; `Editor::IsTyping()` went with the panels' text boxes, so the hotkeys no longer check it; the hierarchy's Add / Delete buttons are gone, the console has `add` / `dup` / `del`)
- [x] Remove `Component::DrawInspector` and the overrides in `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor` (+ done with the line above; the now-unused `<cstdio>` includes went too)
- [x] `EditorToolbar` without the Load button; `EditorUI.h` trimmed to the toolbar / status layout (`IsOverToolbar`) (+ `ToolbarRect` spans the window under the status line; `IsOverToolbar` also covers the status line; Load is console `load [path]`)
- [x] Delete `EditorBannerScreen` (`ModeMachine` pushes no screen for Editor); `Editor::DrawStatus` one line with the selection name / id and the hotkeys (+ `Editor::DrawStatus` draws `EDITOR  -  "name" #id  -  W move  R scale  F focus  Del  Ctrl+D  Ctrl+S  -  ` console`, ASCII separators because raylib's default font has no `·`)
- [x] README editor section rewritten around the light editor (gizmo, click, toolbar) + console (+ README editor section rewritten, Ctrl+S names `arena_01.lvl`)
- [ ] **Done when:** TAB → editor with only the toolbar (no Load), the status line and the gizmo, no hierarchy / inspector; click‑select, W / R gizmo, F, Del, Ctrl+D, Ctrl+S work and the status line names the selection; console `select` → `info` → `set RigidBody mass=5` → `comp_add TriggerVolume` → `save` → `load` round‑trips; `grep -rn "HierarchyPanel\|InspectorPanel\|InspectorLayout\|DrawInspector" src` finds nothing; Debug and Release build clean

### Phase B5 — Rename, docs and release ⬜
- [ ] `ScaleGame` → `ArenaGame`; title "Boomer Shooter"; `Settings` folder `BoomerShooter`
- [ ] `GameUI` placeholder texts ("Arena cleared" / "You died"); Next Level only when `LevelList::HasNext`
- [ ] `tools/package_release.sh` → `dist/BoomerShooter/`; README and `assets/README.md`; `ARCHITECTURE_PLAN.md` header points to the new plan
- [ ] Release package checked: no console, no editor, starts from another folder; Debug has both
- [ ] **Done when:** the release package plays the arena with no console or editor; a Debug build has both

---

## Decision log

Decisions taken while writing the plan (see plan §0 for the reasons).

| # | Date | Decision | Plan section |
|---|---|---|---|
| B‑D1 | 2026-11-18 | ~~Editor GUI removed (panels, toolbar, banner, gizmo)~~ Revised by B‑D12: hierarchy, inspector and banner removed; gizmo, click‑select and toolbar (no Load) kept; every editor capability kept, the rest through the console | §0, §5.4 |
| B‑D2 | 2026-11-18 | New `ENG_DEV_CONSOLE` definition (Debug 1, Release 0, `ENG_DEV_CONSOLE_IN_RELEASE` forces 1) | §5.6 |
| B‑D3 | 2026-11-18 | Console and editor flags independent; editor commands only with `ENG_EDITOR` | §0 |
| B‑D4 | 2026-11-18 | Console built before the editor GUI is removed | §8 |
| B‑D5 | 2026-11-18 | `ScalingGun` → hitscan `Gun` that reports hits; damage comes with enemies | §7.2, §9 |
| B‑D6 | 2026-11-18 | Beam kept as a one‑colour tracer with muzzle / impact bursts | §7.2 |
| B‑D7 | 2026-11-18 | One level in `LevelList`; level flow (manager, triggers, events) kept | §6.2 |
| B‑D8 | 2026-11-18 | Arena = inverted‑normals render cube + six static box slabs | §6 |
| B‑D9 | 2026-11-18 | Arena is 16×6×16 (developer decision, replaces the 8×2×8 draft); the player keeps its 2 m size | §6.3, R1 |
| B‑D10 | 2026-11-18 | `ScaleGame` → `ArenaGame`; placeholder name "Boomer Shooter" | §0 |
| B‑D11 | 2026-11-18 | Win / Lose / NextLevel triggers stay registered, unused by the arena | §0 |
| B‑D12 | 2026-11-18 | Developer review: the editor phase slims the editor instead of removing its GUI, and runs after the arena (B3 arena, B4 slim editor, B5 release). Load console only; no Ctrl+L / Ctrl+R; status line replaces the banner | §0, §3.3, §8 |

---

## Questions for the developer

| # | Question | Default if no answer |
|---|---|---|
| Q1 | ~~Is 8×2×8 the intended arena size?~~ **Answered:** 16×6×16, player unchanged | — |
| Q2 | Should the console pause the game while open? | Yes (`con_pause 1`), switchable |
| Q3 | ~~Keep the uncommitted `level_01.lvl` playtest edits?~~ **Answered:** discard | — |
| Q4 | Final game name for the title, settings folder and package | "Boomer Shooter" / `BoomerShooter` |

---

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| R1 — arena height vs player size | B3 | resolved | 16×6×16 (B‑D9) |
| R2 — rebuild logic lost with `DrawInspector` | B2 / B4 | resolved in B2 | `Collider::OnFieldsChanged`; the other setters apply in `Deserialize`; `DrawInspector` goes in B4 |
| R3 — console input vs game / mode / editor keys | B2 | resolved | Console owns the keyboard while open; validated by the developer |
| R4 — console code in release | B2 / B5 | B2 checked | Guarded headers + `tools/check_release_console.sh`; the package check is B5 |
| Editor Reset can't restore a deleted actor | carried over | open | From the *Build to Scale* plan |
| Placeholder audio, unsigned binary | carried over | open | |

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-18 | B4 | All 5 B4 tasks committed on `bs_slim_editor`: hierarchy and inspector deleted with `DrawInspector` (one commit, they depend on each other), toolbar without Load, `EditorBannerScreen` gone with a one-line status (`Editor::DrawStatus`), README editor section. Clean from-scratch Debug and Release builds of HEAD: no warnings; the grep finds nothing; Debug smoke run loads `arena_01.lvl` with 0 warnings. The editor round trip (`select` → `info` → `set` → `comp_add` → `save` → `load`) and the TAB / gizmo checks are not run by me | Developer runs the B4 **Done when** check | — |
| 2026-11-18 | B3 | Developer validated B3 (Done when pass); `Log.h` typo fixed by the developer; merged into `integration`, branch renamed `done/bs_arena_level` | Start B4 on `bs_slim_editor` | — |
| 2026-11-18 | B3 | Four B3 checkboxes done on `bs_arena_level`: three commits (`CubeInverted`, `arena_01.lvl` + `LevelList` + `level_01–03.lvl` deleted, menu orbit in the arena) and the player-size check (no change needed). A clean copy of HEAD with these changes builds Debug with no errors or warnings; the app starts with no warnings. The level load and the room are not exercised yet (they need Start in the game). The working tree has a typo in `src/engine/core/Log.h` (`{1.` in `enum class LogLevel`) that stops the build; left untouched | Developer fixes `Log.h`, runs the B3 **Done when** check (Start, walk, jump, tracers on the faces, `level_next` shows the last-level dialog) | `Log.h` typo blocks building the working tree |
| 2026-11-18 | — | Developer review of the editor phase: phases reordered (B3 arena, B4 slim editor, B5 release) and the editor phase rescoped to keep the gizmo, click‑select and toolbar (no Load) (B‑D12); plan §0–§10 and this tracker updated. Phase numbers in older log rows refer to the old order | Start B3 on `bs_arena_level` when the developer says so | — |
| 2026-11-18 | B2 | Developer validated B2 (pass); merged into `integration`, branch renamed `done/bs_dev_console` | ~~Start B3 on `bs_remove_editor_gui`~~ (superseded by B‑D12) | — |
| 2026-11-18 | B2 | All 8 B2 tasks committed on `bs_dev_console`; Debug, Release (from scratch) and release-dev build without warnings; the Release binary has no console strings; smoke run loads `level_01` with 0 warnings. Interactive checks (typing, the `pick` → `save` → `load` round trip) not done by me | Developer runs the B2 **Done when** check | — |
| 2026-11-18 | B1 | Developer validated B1 (pass); merged into `integration`, branch renamed `done/bs_remove_scaling` | Start B2 on `bs_dev_console` when the developer says so | — |
| 2026-11-18 | B1 | All 6 B1 tasks committed on `bs_remove_scaling` (Scalable out, PlayerState / PlayerInput trimmed, `Gun` + `ShotFiredEvent`, `ShotFXSpawner` / `Tracer`, `PlayShot`, HUD and gun view); clean Debug and Release builds without warnings; smoke run loads `level_01` with 0 warnings. The puzzle levels can't be won any more (no scaling); replaced in B4 | Developer runs the B1 **Done when** check | — |
| 2026-11-18 | B0 | Developer validated B0 (release-dev build: `editor ON | console ON`); README build-flags section added; merged into `integration`, branch renamed `done/bs_baseline` | Start B1 on `bs_remove_scaling` | — |
| 2026-11-18 | B0 | TASK_04 started: branch `bs_baseline`; tag `build-to-scale-final`; `level_01.lvl` edits discarded; `ENG_DEV_CONSOLE` flag + `IsDevConsoleBuild()` + log line; Debug and Release build and start | Developer runs the B0 **Done when** check | — |
| 2026-11-18 | — | Developer approved the plan and risks; arena changed to 16×6×16, player size kept (B‑D9, Q1) | Wait for the next task | — |
| 2026-11-18 | — | TASK_03: Boomer‑Shooter architecture plan and this tracker written (B‑D1–B‑D11, phases B0–B5) | Developer reviews the plan and answers Q1–Q4 | — |
