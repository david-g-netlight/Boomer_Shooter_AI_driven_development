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

---

## Session log

Newest first. One entry per work session.

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-21 | E0 | Developer validated E0 (`ecs_selftest` and both builds); merged into `integration`, branch renamed `done/enemy_ecs_core`. Tracker: E0 ✅, Done when ticked. Next: E1 on `enemy_pool_and_render` | Start E1 | — |
| 2026-11-21 | E0 | Branch `enemy_ecs_core` from `integration`. Commits: ECS headers `Entity.h`, `ComponentPool.h`, `Registry.h` (+ `eng.h` include); Debug self‑check `ecs/RegistrySelfTest.h` run at startup in `ArenaGame::Init` and as console `ecs_selftest` (Debug only). Builds: Debug and Release clean, incremental and from scratch (`build/check-*`), no warnings. Self‑check also compiled alone in the scratchpad with `clang++ -std=c++20` and `c++23`, `-Wall -Wextra -Werror`: 0 failures. Debug app ran 8 s: `ArenaGame initialized`, no ECS failure line. **Not run:** the console `ecs_selftest` command typed in the game; any interactive check. | Developer runs the E0 **Done when** check (`ecs_selftest` in the Debug console) | — |
| 2026-11-21 | — | TASK_06: checked the developer's move of the tracker history into `devlogs/backup.md` (byte‑identical to the removed sections); tracker's "How to use" now points here; skills `write-dev-progress` and `follow-dev-progress` written in `.claude/skills/`; staged | Developer reviews the skills, gives the go for E0 | — |
| 2026-11-21 | — | Developer answered Q5 (20 across all waves) and Q6 (snap): `arena_01.lvl` snapped to an exact 16 × 6 × 41 interior (x −8 … 8, y 0 … 6, z −33 … 8); plan and tracker updated; staged. Not run in the game yet | Developer checks the arena in game, gives the go for E0 | — |
| 2026-11-21 | — | TASK_05_Iteration: plan updated with the review answers (wave table 7 / 7 / 6, hysteresis 1.2 ×, fake aggro `Alert` E‑D15, timer as §13 "separate plan"), spawn points moved for the lengthened arena, R9 / R10 added; this tracker created; changes staged with the developer's `arena_01.lvl` edits | Developer reviews the staged diff, answers Q5 / Q6, gives the go for E0 | — |
| 2026-11-21 | — | TASK_05: enemy system plan written (E‑D1–E‑D14, phases E0–E5) | Developer review | — |
