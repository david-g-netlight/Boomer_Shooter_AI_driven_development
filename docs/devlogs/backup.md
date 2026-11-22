## Decision log

See the plan's §0 for the reasons.

| # | Date | Decision | Plan section |
|---|---|---|---|
| E‑D1 | 2026-11-21 | Enemies are entities in a small ECS store, not `GameActor`s | §0, §5.1 |
| E‑D2 | 2026-11-21 | No Bullet bodies for enemies: analytic spheres, raycasts for walls | §0, §5.1 |
| E‑D3 | 2026-11-21 | Enemies fly inside a height band | §0, §7.3 |
| E‑D4 | 2026-11-21 | ~~6 / 7 / 7~~ Revised by the review (Q1): waves 7 / 7 / 6 = 20 in total; pool capacity stays 32 | §0, §6.2 |
| E‑D5 | 2026-11-21 | Attack cooldown keeps ticking outside Attack | §0, §7.1 |
| E‑D6 | 2026-11-21 | ~~No hysteresis~~ Revised by the review (Q2): Chase ends at 1.2 × trigger radius; Attack ends at 1.2 × attack range | §0, §7.1 |
| E‑D7 | 2026-11-21 | Gun → enemy through `HitscanTargets`, damage applied at dispatch, generation‑checked | §0, §9.1 |
| E‑D8 | 2026-11-21 | Archetype values copied into components at spawn | §0, §8.1 |
| E‑D9 | 2026-11-21 | Spawn points in the level file, wave table in code | §0, §6.2 |
| E‑D10 | 2026-11-21 | Player health in `PlayerState`; 0 HP → `LoseGame` | §0, §9.2 |
| E‑D11 | 2026-11-21 | Win = last wave cleared; the 5‑minute timer is a separate plan (Q4) | §0, §13 |
| E‑D12 | 2026-11-21 | ECS storage engine‑level; AI, components and systems game‑level | §0, devlog |
| E‑D13 | 2026-11-21 | AI and movement in the fixed step; animation and render per frame | §0, §3.3 |
| E‑D14 | 2026-11-21 | `Primitive::Pyramid` from `GenMeshCone(0.5, 1, 4)` | §0, §8.2 |
| E‑D15 | 2026-11-21 | Review (Q3): fake aggro. A shot enemy outside its range stays in Wander and turns toward the shot origin (`Alert` component) | §0, §7.3, §9.1 |
| E‑D16 | 2026-11-21 | Wander: a new direction after a wall must lead away from it (dot with the wall normal ≥ 0.5, in addition to the room check). Found in test: a tangent direction passed the room check, so the enemy slid along the wall and the probe never fired again | §7.3 step 3–4 |
| E‑D17 | 2026-11-22 | `ArenaBounds` margin 1.0 m → 0.5 m: the clamp must sit inside the probe reach (1 m), else it pins enemies just outside the probe (R11) | Developer report: label stays "wander" at the wall |

---

## Questions for the developer

| # | Question | Default if no answer |
|---|---|---|
| Q1 | ~~20 in total or 20 alive at once?~~ **Answered:** new table 7 / 7 / 6 | — |
| Q2 | ~~Hysteresis on Chase → Idle?~~ **Answered:** yes, 1.2 × (also applied to Attack → Chase) | — |
| Q3 | ~~Aggro on hit?~~ **Answered:** fake it, turn toward the shooter and stay in Wander | — |
| Q4 | ~~Timer in this plan?~~ **Answered:** separate small plan; state and needs in plan §13 | — |
| Q5 | ~~Is 20 the total over all waves?~~ **Answered:** yes, 20 across all waves (7 in the first) | — |
| Q6 | ~~Snap the lengthened arena's values?~~ **Answered:** yes, snapped (R9) | — |

---

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| R1 — fixed‑step judder | E1 / E2 | open | Interpolate positions with `GetFixedAlpha` if visible |
| R2 — wall tunnelling | E2 | open | Sweep ray + 0.5 m slabs |
| R3 — wander stuck in a corner | E2 | open | 8 tries, then reflect |
| R4 — stale ids in shot events | E3 | open | Generation check |
| R5 — enemies invisible to Bullet | — | accepted | E‑D2 |
| R6 — editor reset does not know entities | — | accepted | `restart` resets enemies |
| R7 — O(n²) avoidance | — | accepted | Grid beyond ~200 |
| R8 — pyramid mesh from `GenMeshCone` | E1 | open | Hand‑built fallback |
| R9 — arena render cube vs slabs out of line | — | resolved | Snapped to a 16 × 6 × 41 interior (Q6) |
| R10 — fake aggro may lose the player | — | accepted | By design (Q3) |
| R11 — enemies slide along walls and do not turn away | E2 | closed | Reported after the E2 test; the E‑D16 fix (new direction must lead away from the wall) did **not** fix it in the developer's test. Suspects: (a) the X/Z clamp in `MovementSystem` sits 0.75 m inside the physical wall faces (clamp at ±7 m, faces at ±7.75 m), so an enemy is pinned at the clamp instead of turning; (b) the probe reach is only `radius + max(1 m, speed × 0.4 s)`, so an enemy heading almost parallel to a wall never sees it; (c) AvoidOthers changes the velocity direction after Wander picks `forward`, so the sweep can hit a wall the probe did not see. Not reproduced in code: needs a run with `ai_debug 1` | Cause found 2026-11-22: suspect (a). The level's wall faces are at ±8 m; the clamp (margin 1 m) pinned an enemy at 1 m + radius from the wall, exactly the probe reach (radius + 1 m), so the ray missed and the label stayed "wander". Fix: margin 0.5 m (E‑D17)

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-22 | E2 | Developer validated E2 (Done when passed); merged into `integration`, branch renamed `done/enemy_ai_idle_wander`. R11 closed. Next: E3 on `enemy_damage` | Start E3 | — |
| 2026-11-22 | E2 | Developer reported the label stays "wander" at the wall. Found the cause (R11, suspect a): clamp pinned enemies exactly at probe reach. Commits: `ai_freeze` guard behind `ENG_DEV_CONSOLE` (developer edit), docs move to `docs/tasks/doing|done`, margin 1.0 → 0.5 m. Builds: clean Debug and Release (`build/check-*`), no warnings. **Not run:** the game (no enemy seen turning at a wall); the 5 min / 20 enemies check | Developer runs the E2 **Done when** check | R11 |
| 2026-11-21 | E2 | Developer re-tested the wall fix (E‑D16): enemies still slide along walls and do not turn. Logged as R11, open. E2 stays 🟡 (its **Done when** is not passed). Next step for the next session: reproduce with `ai_debug 1` and check suspects a–c in R11. Also added `CMakePresets.json` (Debug and Release) so CLion can load the project | Developer decides next step on R11 | R11 |
| 2026-11-21 | E2 | Developer tested E2 and reported that enemies slide along walls without turning. Change: `WanderSystem` only accepts a new direction with dot(normal) ≥ 0.5 (decision E‑D16). Not verified in game yet. Built Debug and Release clean | Developer re-tests the wall behaviour | — |
| 2026-11-21 | E2 | Branch `enemy_ai_idle_wander` (from `integration`, which has E0 and E1). Commits: `Brain` + `NextState` + `BrainSystem` (Idle, Wander); `IdleFloatSystem`, `WanderSystem`; `AvoidOthersSystem`, `MovementSystem`, `ArenaBounds` from the "Arena" actor; `ai_debug`, `ai_freeze`, `EnemyDebugDraw`. Builds: Debug and Release clean, incremental and from scratch (`build/check-*`), no warnings. Debug app ran 8 s: `ArenaGame initialized`, no errors. **Not run:** any enemy in the game (no interactive run); the 5 min / 20 enemies bounds check; the wall-turn and no-stacking checks; `ai_debug` and `ai_freeze` in game. Known limit: the sweep slides once per tick. | Developer runs the E2 **Done when** check | — |
| 2026-11-21 | E1 | Developer validated E1 (spawns, `enemy_spawn_all`, pool full, `restart`, material warning fixed); merged into `integration`, branch renamed `done/enemy_pool_and_render`. Tracker: E1 ✅, Done when ticked. Next: E2 on `enemy_ai_idle_wander` | Start E2 | — |
| 2026-11-21 | E1 | Developer reported `restart` does not remove the enemies. Cause: `restart` goes through `GameplayManager` → `ArenaGame::LoadLevelFile` (the game's loader). `OnLevelLoaded` is only called by the editor's `LoadNow`, so `EnemyManager::Reset` never ran in play. Fix: reset in `LoadLevelFile`; `OnLevelLoaded` keeps its reset for the editor. Debug and Release clean. Not run: `restart` in game | Developer runs `enemy_spawn` then `restart` | — |
| 2026-11-21 | E1 | Developer reported the console warning `UNSUPPORTED ... GLD_TEXTURE_INDEX_2D is unloadable`. Cause: the 9 enemy materials were built in `EnemyMaterials`' constructor, which runs in `ArenaGame`'s constructor before the window opens, so their albedo texture is raylib's default (id 0). Fix: `EnemyManager::Init` builds them after the window. Debug and Release builds clean. Not run: the warning check (needs an enemy on screen) | Developer checks the warning is gone with `enemy_spawn` | — |
| 2026-11-21 | E1 | Branch `enemy_pool_and_render`. Commits: `Primitive::Pyramid`; `EnemyComponents`, `EnemyTypes`, `EnemyPool`; `EnemyManager` shell wired into `ArenaGame`; `RenderSystem`, `AnimationSystem`, 9 materials, `MathUtils::LookRotation` (checked numerically in the scratchpad: worst error 2e-5); console `enemy_spawn`, `enemy_spawn_all`, `enemy_list`, `kill_all`. Builds: Debug and Release clean, incremental and from scratch (`build/check-*`), no warnings. Debug app ran 8 s: `ArenaGame initialized`, no errors. **Not run:** any spawn in the game (no interactive run); the pyramid's base cap and normals (not verifiable in code: raylib source not in repo); the 'no allocation after Init' check; the spawn particle burst (§6.1) not built. | Developer runs the E1 **Done when** check | — |
| 2026-11-21 | E0 | Developer validated E0 (`ecs_selftest` and both builds); merged into `integration`, branch renamed `done/enemy_ecs_core`. Tracker: E0 ✅, Done when ticked. Next: E1 on `enemy_pool_and_render` | Start E1 | — |
| 2026-11-21 | E0 | Branch `enemy_ecs_core` from `integration`. Commits: ECS headers `Entity.h`, `ComponentPool.h`, `Registry.h` (+ `eng.h` include); Debug self‑check `ecs/RegistrySelfTest.h` run at startup in `ArenaGame::Init` and as console `ecs_selftest` (Debug only). Builds: Debug and Release clean, incremental and from scratch (`build/check-*`), no warnings. Self‑check also compiled alone in the scratchpad with `clang++ -std=c++20` and `c++23`, `-Wall -Wextra -Werror`: 0 failures. Debug app ran 8 s: `ArenaGame initialized`, no ECS failure line. **Not run:** the console `ecs_selftest` command typed in the game; any interactive check. | Developer runs the E0 **Done when** check (`ecs_selftest` in the Debug console) | — |
| 2026-11-21 | — | TASK_06: checked the developer's move of the tracker history into `devlogs/backup.md` (byte‑identical to the removed sections); tracker's "How to use" now points here; skills `write-dev-progress` and `follow-dev-progress` written in `.claude/skills/`; staged | Developer reviews the skills, gives the go for E0 | — |
| 2026-11-21 | — | Developer answered Q5 (20 across all waves) and Q6 (snap): `arena_01.lvl` snapped to an exact 16 × 6 × 41 interior (x −8 … 8, y 0 … 6, z −33 … 8); plan and tracker updated; staged. Not run in the game yet | Developer checks the arena in game, gives the go for E0 | — |
| 2026-11-21 | — | TASK_05_Iteration: plan updated with the review answers (wave table 7 / 7 / 6, hysteresis 1.2 ×, fake aggro `Alert` E‑D15, timer as §13 "separate plan"), spawn points moved for the lengthened arena, R9 / R10 added; this tracker created; changes staged with the developer's `arena_01.lvl` edits | Developer reviews the staged diff, answers Q5 / Q6, gives the go for E0 | — |
| 2026-11-21 | — | TASK_05: enemy system plan written (E‑D1–E‑D14, phases E0–E5) | Developer review | — |
