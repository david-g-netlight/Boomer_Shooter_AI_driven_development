# Development Progress — Enemy System v1

Tracks the implementation of [`Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN.md`](Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN.md).
Phase names and **Done when** criteria match §11 of that plan. The base game's history (phases B0–B5, decisions
B‑D1–B‑D12) stays in [`Boomer_Shooter_DEV_PROGRESS.md`](Boomer_Shooter_DEV_PROGRESS.md).
History of this plan (decision log, answered questions, risks, session log): [`devlogs/backup.md`](devlogs/backup.md).

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- At the end of every work session, add a line to the **Session log** in [`devlogs/backup.md`](devlogs/backup.md). When the plan changes, add a **Decision** (E‑Dn) there too.
- Open questions for the developer go in an **Open questions** section in this file. Once a question is answered, move it to the history file.
- Branches: one per phase from `integration`, one commit per checkbox (except **Done when**, checked by the developer); a finished branch is renamed `done/<branch>` ([`tasks/TASK_04.md`](tasks/TASK_04.md)).
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| E0 | ECS core | `enemy_ecs_core` | ⬜ | | | `engine/ecs/`, header‑only, std only |
| E1 | Pool and visible enemies | `enemy_pool_and_render` | ⬜ | | | `Primitive::Pyramid`, 3 archetypes, capacity 32 |
| E2 | Idle and wander | `enemy_ai_idle_wander` | ⬜ | | | Movement, walls, avoidance |
| E3 | Shooting enemies | `enemy_damage` | ⬜ | | | `HitscanTargets`, `Life`, fake aggro (`Alert`) |
| E4 | Chase and attack | `enemy_ai_chase_attack` | ⬜ | | | Hysteresis 1.2 ×, player health, lose |
| E5 | Waves | `enemy_waves` | ⬜ | | | 7 / 7 / 6 = 20, 6 spawn points, win |

**Current focus:** none. The plan is approved with comments (iteration 1); waiting for the go to start E0.
**Branch:** `integration`

---

## Phase checklists

### Phase E0 — ECS core ⬜
- [ ] `engine/ecs/Entity.h` (16‑bit index + 16‑bit generation), `ComponentPool.h` (sparse set, swap‑remove), `Registry.h` (create / destroy / free list, `Add` / `Get` / `Remove`, `Each<A, Rest...>`, `Clear`); `eng.h` includes `ecs/Registry.h`
- [ ] Debug self‑check (`#ifndef NDEBUG`) + console `ecs_selftest`
- [ ] **Done when:** Debug and Release build clean; `ecs_selftest` passes (stale id rejected, packed after removes, `Each` visits only full matches, `Clear` keeps capacity)

### Phase E1 — Pool and visible enemies ⬜
- [ ] `Primitive::Pyramid` (`GenMeshCone(0.5, 1, 4)`, `mesh=pyramid`); check the base cap and normals (R8)
- [ ] `EnemyComponents.h`, `EnemyTypes` (Drifter, Stalker, Brute; §8.1), `EnemyPool` (capacity 32, `Spawn`, `FlushReleased`, `Clear`)
- [ ] `EnemyManager` (Init, Reset, FixedUpdate shell, Submit); `ArenaGame` owns it, overrides `FixedUpdate` and `SubmitRenderables`, resets it in `OnLevelLoaded`
- [ ] `RenderSystem` + `AnimationSystem` (spawn scale‑in, turn smoothing); 9 materials
- [ ] Console: `enemy_spawn`, `enemy_spawn_all`, `enemy_list`, `kill_all` (removes for now)
- [ ] **Done when:** `enemy_spawn drifter 5`, `stalker 3`, `brute 2` show three sizes and colours; `enemy_spawn_all` shows 20; a 33rd spawn logs "pool full"; `restart` removes them; no allocation after Init

### Phase E2 — Idle and wander ⬜
- [ ] `Brain` + `EnemyBrain` (`NextState`, `BrainSystem`) with Idle and Wander only
- [ ] `IdleFloatSystem` (5 s bob through velocity) and `WanderSystem` (probe ray, unit‑sphere draw, room check, 8 tries + reflect, snap + smooth turn)
- [ ] `AvoidOthersSystem` and `MovementSystem` (sweep ray, slide, height band, overlap fix); `ArenaBounds` from the `"Arena"` actor
- [ ] `ai_debug`, `ai_freeze` cvars, `EnemyDebugDraw`
- [ ] **Done when:** enemies float 5 s, then roam; none leaves the arena or passes the floor or ceiling in 5 minutes with 20 enemies; they turn before walls; they do not stack; `ai_debug` shows probes and states

### Phase E3 — Shooting enemies ⬜
- [ ] `HitscanTargets` + `Gun` (wall distance first, then ray‑sphere); `ShotFiredEvent` + `enemy`, `damage`, `origin`; crosshair red on enemies
- [ ] `Life`, shot handler (generation check), hit flash, `LifeSystem` (death animation, burst, `EnemyKilledEvent`), `FlushReleased`
- [ ] `Alert` (fake aggro, E‑D15): raised on Idle / Wander enemies; Idle → Wander; `WanderSystem` turns toward the shot origin; `ChaseSystem` clears it
- [ ] `SoundFXBank`: `PlayEnemyHit`, `PlayEnemyDeath`
- [ ] **Done when:** Drifter 3 hits, Stalker 6, Brute 12; hits flash; a wall in front protects an enemy; an enemy behind another is not hit; `kill_all` plays every death; spawn 32 → kill all → spawn 32 works; an enemy shot from outside its range turns toward where the player stood and stays in Wander

### Phase E4 — Chase and attack ⬜
- [ ] `ScanForPlayer` + `ScanSystem` (trigger and lose radius, E‑D6); Brain gets Chase, Attack, Dying and the player‑dead rule; attack exit at 1.2 × range
- [ ] `ChaseSystem` (turn rate), `DealDamageSystem` (1 s cooldown that keeps ticking, E‑D5), lunge animation
- [ ] `PlayerState` health (100), `ApplyDamage`, `god`; `PlayerHitEvent` / `PlayerDiedEvent` → `LoseGame`
- [ ] HUD health bar + hurt flash; `PlayPlayerHurt`; console `player_hp`
- [ ] **Done when:** chase starts inside the trigger radius and stops (idle 5 s) only beyond 1.2 ×; no flicker at either edge; in range it lunges and hits once per second; leaving and re‑entering range does not reset the cooldown; 0 HP shows "You died"; `god 1` takes no damage; the three types are clearly harder in order

### Phase E5 — Waves ⬜
- [ ] `EnemySpawnPoint` component (registered, saved); 6 spawn actors in `arena_01.lvl` at (±6, 3, 0), (±6, 3, −14), (±6, 3, −28)
- [ ] `WaveDirector`: table 7 / 7 / 6 (§6.2), staggered spawns, spawn safety (4 m from the player, 1.5 m from enemies), `WaveStartedEvent`, `WavesClearedEvent` → `WinGame`
- [ ] HUD "Wave n / 3 · k left"; console `wave_start`, `wave_skip`, `waves`, `enemy_tune`
- [ ] **Done when:** wave 1 = 7 Drifters, wave 2 = 5 Drifters + 2 Stalkers, wave 3 = 3 Drifters + 2 Stalkers + 1 Brute, spawned one by one; next wave 2 s after a clear; the last kill shows "Arena cleared"; dying shows "You died" and Retry restarts wave 1 with full health; spawn points moved in the editor and saved are used after a reload
