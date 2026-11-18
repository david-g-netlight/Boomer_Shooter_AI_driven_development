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
| B0 | Baseline | `bs_baseline` | 🟡 | 2026-11-18 | | Tag, `ENG_DEV_CONSOLE` flag |
| B1 | Remove the scaling mechanic | `bs_remove_scaling` | ⬜ | | | `Scalable` out, `ScalingGun` → `Gun` |
| B2 | Developer console | `bs_dev_console` | ⬜ | | | Before the GUI goes (B‑D4) |
| B3 | Remove the editor GUI | `bs_remove_editor_gui` | ⬜ | | | Capabilities stay, through the console |
| B4 | Arena level | `bs_arena_level` | ⬜ | | | 16×6×16 inverted cube, one level |
| B5 | Rename, docs and release | `bs_rename_release` | ⬜ | | | `ArenaGame`, "Boomer Shooter" |

**Current focus:** B0 Baseline
**Branch:** `bs_baseline`

---

## Phase checklists

### Phase B0 — Baseline 🟡
- [x] Tag `integration` as `build-to-scale-final` (annotated tag on `2a12cbb`)
- [x] Developer decides on the uncommitted `assets/levels/level_01.lvl` playtest edits (commit, or discard; the file is deleted in B4) (+ **discarded** by the developer; `CLAUDE.md` edits left uncommitted on purpose)
- [x] CMake: `ENG_DEV_CONSOLE` (Debug 1 / Release 0) + `ENG_DEV_CONSOLE_IN_RELEASE` option; `eng::IsDevConsoleBuild()`; startup log shows `console ON/OFF` (+ smoke runs: Debug logs `editor ON | console ON`, Release `editor OFF | console OFF`)
- [ ] **Done when:** Debug and Release build; the log shows the console flag; the game plays exactly as before

### Phase B1 — Remove the scaling mechanic ⬜
- [ ] Delete `game/props/Scalable`, its registration and the editor prop decorator; strip `component Scalable` from the `.lvl` files
- [ ] `PlayerState` without scale factors / `ChangeFactor` / `ScaleFactorChangedEvent`; `PlayerInput` without `AltFire`, `Scroll` and the fire sign
- [ ] `ScalingGun` → `Gun` (`ShotFiredEvent`, `GetAimHit`); cooldown and reach become shooter defaults (0.25 s, 100 m)
- [ ] `RayBeamSpawner` → `ShotFXSpawner`, `RayBeam` → `Tracer` (one colour, 80 m/s), `MuzzleFX` one tint
- [ ] `SoundFXBank::PlayShot`; remove `sfx_stretch.wav` / `sfx_shrink.wav`, update the audio generator script and `CREDITS.txt`
- [ ] `GunView` without the factor light; `GunHUD` = crosshair (red on a hit actor in reach) + cooldown ring
- [ ] **Done when:** LMB fires tracer + muzzle + impact + sound; RMB and wheel do nothing; crosshair turns red on a crate; no scaling code left in `src/game`; levels load without warnings

### Phase B2 — Developer console ⬜
- [ ] `DevConsole` (owned by `Engine`), `ConsoleCommands` (register / unregister tokens, completion), `ConsoleArgs`, `ConVar`; `Log` sink into the scroll‑back
- [ ] `ConsoleScreen` on the `UIStack`: grave toggle, input line, scroll‑back, history ↑/↓, TAB autocomplete; cursor free while open
- [ ] `ModeMachine`: TAB / ESC belong to the console while open; `IsSimulating` honours `con_pause`; `PlayerInput` gated
- [ ] Engine commands: `help`, `clear`, `echo`, `quit`, `mode`, `timescale`, `con_pause`, `phys_debug`, `grid`, `fps`, `exec`
- [ ] Editor commands (selection and actors): `editor`, `play`, `ls`, `select`, `pick`, `deselect`, `info`, `rename`, `pos`, `move`, `rot`, `scale`, `active`, `parent`, `add`, `dup`, `del`, `focus`, `spawn_here`, `spawn`, `reset`, `save`, `load`
- [ ] `Component::OnFieldsChanged` (rebuild code shared with the inspector), `Editor::SetComponentField / AddComponent / RemoveComponent`; commands `set`, `get`, `comp_add`, `comp_rm`, `comp_types`
- [ ] Game commands in `game/dev/GameCommands.cpp`: `restart`, `level_list`, `level_load`, `level_next`, `win`, `lose`, `tp`, `tp_spawn`, `god`, `gun_cooldown`, `gun_reach`, `sens` (`noclip` optional); unregistered in `Destroy`
- [ ] Compile‑out check: every console `.cpp`/`.h` guarded; Release `strings` check finds no command names
- [ ] **Done when:** grave opens the console in every mode; `help` lists everything; `pick` → `scale 2` → `set RigidBody mass=5` → `comp_add TriggerVolume` → `save` → `load` round‑trips; `restart`, `win`, `tp 0 0 0` work; ESC closes the console without pausing; Release has no console

### Phase B3 — Remove the editor GUI ⬜
- [ ] Delete `HierarchyPanel`, `InspectorPanel`, `InspectorLayout`, `EditorToolbar`, `EditorUI`, `TransformGizmo`, `Editor::DrawPanels`, their `eng.h` includes
- [ ] Delete `EditorBannerScreen`; `ModeMachine` pushes no screen for Editor
- [ ] Remove `Component::DrawInspector` and the overrides in `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor`
- [ ] `Editor::DrawStatus` (one text line); hotkeys Ctrl+L load and Ctrl+R reset; selection highlight kept
- [ ] README editor section rewritten around the console and hotkeys
- [ ] **Done when:** TAB → editor with no panel / toolbar / banner / gizmo; every row of plan §5.4 works from the console; click‑pick, F, Del, Ctrl+D, Ctrl+S, Ctrl+L, Ctrl+R work; no raygui calls left in `src/engine/editor`

### Phase B4 — Arena level ⬜
- [ ] `Primitive::CubeInverted` (flipped winding and normals), `mesh=cube_inverted` in level files
- [ ] `assets/levels/arena_01.lvl`: render cube 16×6×16 + six 0.5 m static slabs (layer Ground) + spawn; `LevelList` = arena only; delete `level_01–03.lvl`
- [ ] Player size unchanged (2 m capsule, eye 1.7 m, 1 m jump): check it against the 6 m ceiling (plan §6.3)
- [ ] Menu orbit inside the arena (radius 6, height 4)
- [ ] **Done when:** Start spawns the player in a lit 16×6×16 room seen from inside; walk and jump without touching the ceiling; can't leave the room; tracers stop on every face; editor shows six slab wireframes; `level_next` with one level shows the last‑level dialog

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
| B‑D1 | 2026-11-18 | Editor GUI removed (panels, toolbar, banner, gizmo); every editor capability kept and reached through the console + hotkeys | §0, §5.4 |
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
| R1 — arena height vs player size | B4 | resolved | 16×6×16 (B‑D9) |
| R2 — rebuild logic lost with `DrawInspector` | B2 / B3 | open | Moved to `OnFieldsChanged` in B2, before B3 |
| R3 — console input vs game / mode / editor keys | B2 | open | Console owns the keyboard while open |
| R4 — console code in release | B2 / B5 | open | Guarded headers + `strings` check |
| Editor Reset can't restore a deleted actor | carried over | open | From the *Build to Scale* plan |
| Placeholder audio, unsigned binary | carried over | open | |

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-18 | B0 | TASK_04 started: branch `bs_baseline`; tag `build-to-scale-final`; `level_01.lvl` edits discarded; `ENG_DEV_CONSOLE` flag + `IsDevConsoleBuild()` + log line; Debug and Release build and start | Developer runs the B0 **Done when** check | — |
| 2026-11-18 | — | Developer approved the plan and risks; arena changed to 16×6×16, player size kept (B‑D9, Q1) | Wait for the next task | — |
| 2026-11-18 | — | TASK_03: Boomer‑Shooter architecture plan and this tracker written (B‑D1–B‑D11, phases B0–B5) | Developer reviews the plan and answers Q1–Q4 | — |
