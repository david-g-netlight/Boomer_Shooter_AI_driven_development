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
| E0 | ECS core | `done/enemy_ecs_core` | ✅ | 2026-11-21 | 2026-11-21 | `engine/ecs/`, header‑only, std only |
| E1 | Pool and visible enemies | `done/enemy_pool_and_render` | ✅ | 2026-11-21 | 2026-11-21 | `Primitive::Pyramid`, 3 archetypes, capacity 32 |
| E2 | Idle and wander | `done/enemy_ai_idle_wander` | ✅ | 2026-11-21 | 2026-11-22 | Movement, walls, avoidance |
| E3 | Shooting enemies | `enemy_damage` | 🟡 | 2026-11-22 | | `HitscanTargets`, `Life`, fake aggro (`Alert`) |
| E4 | Chase and attack | `enemy_ai_chase_attack` | ⬜ | | | Hysteresis 1.2 ×, player health, lose |
| E5 | Waves | `enemy_waves` | ⬜ | | | 7 / 7 / 6 = 20, 6 spawn points, win |

**Current focus:** E3 — Shooting enemies (`enemy_damage`).
**Branch:** `enemy_damage`

---

## Phase checklists

### Phase E0 — ECS core ✅
- [x] `engine/ecs/Entity.h` (16‑bit index + 16‑bit generation), `ComponentPool.h` (sparse set, swap‑remove), `Registry.h` (create / destroy / free list, `Add` / `Get` / `Remove`, `Each<A, Rest...>`, `Clear`); `eng.h` includes `ecs/Registry.h` (+ `IComponentPool` lives in `ComponentPool.h`; `Registry` also keeps an `m_alive` flag per slot, so a free slot's current generation is not treated as alive)
- [x] Debug self‑check (`#ifndef NDEBUG`) + console `ecs_selftest` (+ the checks live in `ecs/RegistrySelfTest.h`, a new header; the startup check runs in `ArenaGame::Init`; the command is in `GameCommands`)
- [x] **Done when:** Debug and Release build clean; `ecs_selftest` passes (stale id rejected, packed after removes, `Each` visits only full matches, `Clear` keeps capacity)

### Phase E1 — Pool and visible enemies ✅
- [x] `Primitive::Pyramid` (`GenMeshCone(0.5, 1, 4)`, `mesh=pyramid`); check the base cap and normals (R8) (+ the cap and normals are **not checked in code**: raylib's source is not in the repo. Look at the spike from below in the test)
- [x] `EnemyComponents.h`, `EnemyTypes` (Drifter, Stalker, Brute; §8.1), `EnemyPool` (capacity 32, `Spawn`, `FlushReleased`, `Clear`) (+ components are in namespace `enemies` (raylib's `::Transform` clash); `EnemyLook` has no material pointers, the manager's table gives them; the spawn particle burst of §6.1 is **not** done: no checkbox names it)
- [x] `EnemyManager` (Init, Reset, FixedUpdate shell, Submit); `ArenaGame` owns it, overrides `FixedUpdate` and `SubmitRenderables`, resets it in `OnLevelLoaded` (+ `FixedUpdate` runs `FlushReleased`; `Animate` is called from `ArenaGame::Update` while simulating, as §3.3)
- [x] `RenderSystem` + `AnimationSystem` (spawn scale‑in, turn smoothing); 9 materials (+ `MathUtils::LookRotation` added, §7.3 names it but it did not exist; the flash material and squash are drawn, but nothing sets `hitFlash` until E3; the lunge comes in E4)
- [x] Console: `enemy_spawn`, `enemy_spawn_all`, `enemy_list`, `kill_all` (removes for now) (+ `enemy_spawn` puts the group in a grid: the first row 6 m in front, 1.5 m apart, so they do not stack; `enemy_spawn_all` is 15 Drifters, 4 Stalkers, 1 Brute)
- [x] **Done when:** `enemy_spawn drifter 5`, `stalker 3`, `brute 2` show three sizes and colours; `enemy_spawn_all` shows 20; a 33rd spawn logs "pool full"; `restart` removes them; no allocation after Init

### Phase E2 — Idle and wander ✅
- [x] `Brain` + `EnemyBrain` (`NextState`, `BrainSystem`) with Idle and Wander only (+ `NextState` takes only the `Brain` in E2 and handles Idle and Wander; `Brain` got `baseY` (the bob centre), set at spawn as well as on entering Idle; the rest of §7.1 comes in E4)
- [x] `IdleFloatSystem` (5 s bob through velocity) and `WanderSystem` (probe ray, unit‑sphere draw, room check, 8 tries + reflect, snap + smooth turn) (+ the bob is a velocity, plus a pull onto the bob centre so it does not drift; the Wander spin of §8.2 is not done: it is an animation, not in this checkbox)
- [x] `AvoidOthersSystem` and `MovementSystem` (sweep ray, slide, height band, overlap fix); `ArenaBounds` from the `"Arena"` actor (+ `ArenaBounds` is shrunk 1 m from the "Arena" box and set on each level load; without an Arena actor there are no limits and a warning; the sweep slides once, a second wall within the same step is not swept; the push speed 3 m/s is a tuning value)
- [x] `ai_debug`, `ai_freeze` cvars, `EnemyDebugDraw` (+ `ai_debug` also switches the engine's DebugDraw on, since it drops everything while off; trigger and attack spheres come in E4)
- [x] **Done when:** enemies float 5 s, then roam; none leaves the arena or passes the floor or ceiling in 5 minutes with 20 enemies; they turn before walls; they do not stack; `ai_debug` shows probes and states

### Phase E3 — Shooting enemies 🟡
- [x] `HitscanTargets` + `Gun` (wall distance first, then ray‑sphere); `ShotFiredEvent` + `enemy`, `damage`, `origin`; crosshair red on enemies (+ `GunAim` also takes the enemy, so the crosshair turns red on one; `PlayerState::GUN_DAMAGE` = 10; the targets reach the Gun through `PlayerContext`; the tracker's start edit (status 🟡, date, branch) was left out of this commit by mistake and is in the tracker commit)
- [x] `Life`, shot handler (generation check), hit flash, `LifeSystem` (death animation, burst, `EnemyKilledEvent`), `FlushReleased` (+ `kill_all` now sets hp to 0, so every enemy plays its death; `LifeSystem` takes `SystemContext` with the event bus and particles; the shrink and the flash timer are in `AnimationSystem`; `EnemyManager::Shutdown` unsubscribes the shot handler)
- [x] `Alert` (fake aggro, E‑D15): raised on Idle / Wander enemies; Idle → Wander; `WanderSystem` turns toward the shot origin; `ChaseSystem` clears it (+ `NextState` now also takes the `Alert`; the clearing by `ChaseSystem` comes with E4, which has no Chase yet)
- [x] `SoundFXBank`: `PlayEnemyHit`, `PlayEnemyDeath` (+ there is only one sound file: both sounds are the ping at another pitch; the hit sound is not placed at the hit point, the bank has no 3D play)
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
