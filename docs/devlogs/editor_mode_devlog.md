# Devlog — `editor_mode`

**Phase:** 9 — Editor mode ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `72827ee`
**Date:** 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/editor_mode`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `4756f72` | TAB enter/exit, `LevelSnapshot` | `src/engine/editor/Editor` (owned by the Engine, called by the ModeMachine on Play ⇄ Editor) and `LevelSnapshot`: actor active flag, local transform, rigid body velocities; `Restore` puts them back and removes actors created while editing. |
| 2 | `da00a3f` | `EditorCamera` | RMB look (cursor captured while held), WASD/QE fly, Shift fast, wheel speed, `FocusOn`. The Editor swaps the active camera on enter / exit. |
| 3 | `275f8df` | `Picker` | Mouse ray through `PhysicsWorld::Raycast` first, then render bounds of actors without a collider. Selection with orange bounds, F focuses; the selection is dropped when the actor goes. |
| 4 | `3818dd6` | `TransformGizmo` | W move / R scale handles of constant screen size; drag projects the mouse ray onto the axis. Writes through `RigidBody::Teleport`, `CharacterMotor::Warp` and the collider scale sync; works with parented actors. |
| 5 | `aa3f715` | `HierarchyPanel` | Actor tree, select, rename, Add Cube / Add Trigger / Delete. `EditorUI` layout helpers; hotkeys stay quiet while typing; clicks over panels never pick. |
| 6 | `851ac42` | `InspectorPanel` | `InspectorLayout` widgets, `Component::DrawInspector` for `MeshRenderer`, `Collider`, `RigidBody`, `TriggerVolume`, `CharacterMotor`; `ActorEdit` shared with the gizmo; collider edits re-create the body. |
| 7 | `58b17a6` | `EditorToolbar` | Play, Reset, Move / Scale, Save, Load, Ctrl+S. |
| 8 | `d06ee2d` | `LevelSerializer` + `ComponentRegistry` | `.lvl` text format, `PropertyBag`, `Component::Serialize / Deserialize`, Save / Load / Duplicate in the editor, `Application::GetLevelPath / OnLevelLoaded`, saves mirrored into the source `assets/`. |
| 9 | `aadea4d` | Bullet wireframes in the overlay | Physics debug on while editing (restored on exit), toolbar Physics toggle, grid, translucent trigger boxes from the collider. |
| 10 | *(this commit)* | — | This devlog; tracker marks Phase 9 done (the developer ticked **Done when**). |

Tracker-only commits `9361a53`, `f4dd0db`, `2c1cefd` split the combined panels checkbox and tick its parts.

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P9‑1 | `LevelSnapshot` keeps actor state in memory instead of serialized text | The serializer did not exist yet at task 1. **Known gap:** an actor deleted while editing is not brought back by Reset |
| P9‑2 | D12: one `PropertyBag` for Serialize and Deserialize; actors with an unregistered component are not saved | Simpler than a writer / reader pair; keeps transient actors (beams, timers) out of files |
| P9‑3 | D13: the game's lit shader lives in `ResourceManager::SetDefaultLitShader` | Level loading must work in release builds, so it cannot depend on the editor |
| P9‑4 | Load runs at the start of the next editor frame, then `Application::OnLevelLoaded()` | The game rebinds its actor pointers (player, crates, trigger counter) after the level was replaced |
| P9‑5 | Editor saves are mirrored into the source `assets/` (compile definition `ENG_SOURCE_DIR`, editor builds only) | The build copies `assets/` over the executable's copy and would overwrite a level saved there |
| P9‑6 | Gizmo takes mouse state as arguments, not the `InputManager` | Makes the drag math testable headless |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Snapshot (headless): crate pose, velocity, scale restored; actor added while editing removed | ✅ |
| Camera swap / focus, picker (physics hit, bounds fallback, empty), selection dropped on destroy | ✅ |
| Gizmo drag (move, scale, parented actor; velocity zeroed; collider scale synced) | ✅ |
| Serializer: round trip (second pass gives identical text), file save / load, duplicate, malformed input | ✅ |
| Real sandbox saved, edited, reloaded and played (27 actors, 20 crates, player and trigger rebound) | ✅ |
| Screenshots of the panels, toolbar and wireframe overlay | ✅ |
| Enter the editor mid-game, edit, save, exit with edits live, reload, Reset | ✅ validated by the developer |

## Notes for later phases
- Phase 10: `ScaleGame` still holds raw `GameActor*` (player, crates, trigger counter) and rebinds them in `OnLevelLoaded`; `ActorHandle` replaces that.
- Phase 10: the player is saved in the level as an actor with `CharacterMotor`; the `.lvl` `spawn` line from the plan is still to be added.
- Phase 12: register game components (`Scalable`, triggers…) in `Application::Init` so levels can contain them.
