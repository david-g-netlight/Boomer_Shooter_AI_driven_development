# Devlog — `game_state_and_player`

**Phase:** 10 — Game: state and player ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `e7308ca`
**Date:** 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/game_state_and_player`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `2f98451` | `ActorHandle` | Id-based weak actor reference: `GameActor::GetId`, `Level::FindActor` (process-wide registry, ids never reused). `Get()` is null once the actor is deleted or its level is gone; marked actors still resolve until the flush (`IsAlive()` excludes them). The sandbox's player, crates and pellets are handles. |
| 2 | `0a99c07` | `PlayerState`, `GameState`, `SoundFXBank`, `LevelList` | `src/game/state/`. Walk speed 10, factors x0.25 / x0.5 / x0.75 (clamped), 1 s cooldown, `ScaleFactorChangedEvent`; Idle → Playing → Won / Lost with started / won / lost / level-progressed events; scaling SFX pitch rule; level paths. The sandbox uses them (wheel, SFX, Start / Restart, HUD status). |
| 3 | `98659d3` | `PlayerInput` | Direction, look, jump, fire, wheel → `PlayerState`. Jump and fire buffered 0.1 s so FixedUpdate sees them; `SetControl(false)` and any mode but Play cut everything. |
| 4 | `7d7792a` | `PlayerActor`, `FirstPersonController`, `FirstPersonLook` | `src/game/player/`. Capsule player; gravity 32 and jump 8 from the Unity arc (h 1 m, apex 0.25 s); a buffered jump is taken once grounded. Yaw turns the body, pitch only the camera (±89°), eyes 0.7 m above the capsule centre. The old fly camera / V mode is gone. |
| 5 | `e2ac383` | Spawn from `.lvl` | `spawn x y z yaw` line, `Level::GetSpawn / SetSpawn`, editor "Set Spawn Here" and overlay capsule. The sandbox loads `assets/levels/level_01.lvl` (exported from the code sandbox; falls back to building it), Restart reloads the file. |
| 6 | *(this commit)* | — | This devlog; tracker marks Phase 10 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P10-1 | Handle ids come from one process-wide registry that is never destroyed | Levels owned by the Engine singleton are deleted during static destruction |
| P10-2 | `ActorHandle::Get()` still returns an actor marked for destroy until the flush | Event handlers may look at actors marked this frame; `IsAlive()` is the strict check |
| P10-3 | The player is spawned by the game, never saved in a level file | Its components aren't registered, so the serializer skips it; levels only carry the spawn line. An old file with a player gets it destroyed on load |
| P10-4 | `PlayerInput::Apply` takes plain values | Lets the buffering and the control cut-off be tested without a keyboard |
| P10-5 | The wheel clamps at x0.25 / x0.75 instead of wrapping | Matches the Unity `ChangeFactor` |
| P10-6 | `level_01.lvl` is an export of the code sandbox | Gives the Done-when a level now; Phase 12 authors the real levels |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| `ActorHandle` (headless, ASan): delete, children, clear, level destroyed, ids not reused | ✅ |
| State classes: factor clamp, cooldown, win / lose rules, events, level list | ✅ |
| `PlayerInput`: buffering, expiry, fire sign, wheel, control off, not in Play | ✅ |
| Real sandbox: walked 9.6 m in 1 s, jump rise 0.92 m, yaw 20° / pitch 10°, body rotation kept, no input without control | ✅ |
| Level flow: file load → one player at the spawn, custom spawn, editor Load, Restart | ✅ |
| Walk, jump, look in `level_01`; wheel cycles the factor | ✅ validated by the developer |

## Notes for later phases
- Phase 11: `ScalingGun` reads `PlayerInput::ConsumeFire()` in FixedUpdate and `PlayerState` for factor, cooldown and reach; the sandbox's LMB kick, beam and impact code in `ScaleGame` is the throwaway version of it.
- Phase 11: `Scalable` must be registered in `ScaleGame::Init` so the editor can add it to props.
- Phase 12: `GameplayManager` replaces `LoadCurrentLevel` and uses `LevelList` / `GameState`; `GameUI` calls `PlayerInput::SetControl(false)`.
