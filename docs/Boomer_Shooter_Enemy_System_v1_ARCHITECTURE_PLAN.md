# Architecture Plan — Enemy System v1 (Boomer Shooter)

This plan adds enemies to the base built in [`Boomer_Shooter_ARCHITECTURE_PLAN.md`](Boomer_Shooter_ARCHITECTURE_PLAN.md)
(phases B0–B5, all done). It builds the **Enemies**, **Damage** and **Waves** rows of that plan's hand‑off table (§9).
It adds a small ECS‑style store, an AI state machine (idle, wander, chase, attack), an enemy pool, a wave spawner and
three enemy types. Everything is written in plain C++20. No new third‑party library.

Source tasks: [`tasks/TASK_05.md`](tasks/TASK_05.md), review answers in
[`tasks/TASK_05_Iteration.md`](tasks/TASK_05_Iteration.md). Progress: [`Enemy_System_v1_DEV_PROGRESS.md`](Enemy_System_v1_DEV_PROGRESS.md).
Status: **approved with comments, iteration 1** (nothing is built yet).

**In one line:** 20 floating sphere‑and‑pyramid enemies come in 3 waves. They drift, see the player, chase and bite.
The gun kills them, and clearing the last wave wins the arena.

### Iteration 1 (developer review, 2026-11-21)

| Q | Answer | What changed in this plan |
|---|---|---|
| Q1 | New wave table: 7 Drifters · 5 Drifters + 2 Stalkers · 3 Drifters + 2 Stalkers + 1 Brute | E‑D4, §6.2. The table adds up to **20 in total** (7 + 7 + 6), **confirmed by the developer** (tracker Q5): 20 across all waves, 7 in the first |
| Q2 | Yes, avoid flickering | E‑D6: Chase is left at **1.2 × the trigger radius**. Attack is left at 1.2 × the attack range for the same reason. §5.2, §7.1 |
| Q3 | Fake aggro: an enemy shot outside its chase range stays in Wander and turns toward where the player was when they shot | E‑D15 (new), `Alert` component, §7.1, §7.3, §9.1 |
| Q4 | The 5‑minute timer gets its own small plan | §13 (new): what exists today and what the timer needs |
| — | The developer lengthened the arena in `arena_01.lvl`; values then snapped (Q6): interior **16 × 6 × 41 m**, x −8 … 8, y 0 … 6, z −33 … 8 | §6.2 spawn points, §12 R9 (resolved) |

---

## 0. Reading the task: decisions taken

| # | Decision | Choice | Why |
|---|---|---|---|
| E‑D1 | Enemies: `GameActor`s or entities? | **Entities in a small ECS‑style store** (`eng::ecs::Registry`), not `GameActor`s | A `GameActor` is a heap object with a `std::string` name, a vector of `unique_ptr<Component>` (one heap block each), virtual `Update` per component and `dynamic_cast` lookups. That fits authored level objects (files, editor, Bullet). It is the wrong shape for many small things that are all updated the same way (§5) |
| E‑D2 | Enemy collision | **No Bullet bodies for enemies.** Each enemy is an analytic sphere. Walls are found with `PhysicsWorld::Raycast` against `Ground \| Prop`. Enemy against enemy is an O(n²) sphere test. The gunshot is a ray‑sphere test | 20 spheres need no physics engine. This keeps the ECS free of Bullet and of `GameActor`. The `Enemy` layer from the base plan §9 is **not** needed in v1 |
| E‑D3 | Do enemies fly? | **Yes, in a height band** (y 1.0 … 5.0 m in the 6 m arena) | The task's idle "float up and down" and wander "random direction inside a unit sphere" describe flying enemies. The vertical part of a wander direction is damped (×0.35) so they mostly roam sideways |
| E‑D4 | "It must spawn 20 enemies" | **20 enemies in total over 3 waves: 7 / 7 / 6** (developer's table, Q1). At most 7 are alive at once in normal play. Pool capacity stays **32**, so all 20 can also be alive at once (`enemy_spawn_all`, stress test) | Developer review (Q1). The spare capacity costs about 3 KB and keeps the stress test possible |
| E‑D5 | Attack cooldown | The 1 s cooldown is a timer on the `DealDamage` component. It **keeps running outside the Attack state** | If stepping out of range reset the timer, the player could dance in and out of range and never be hit |
| E‑D6 | Chase exit (hysteresis) | Chase starts when `distSq < triggerRadiusSq` and ends only when `distSq ≥ loseRadiusSq`, with **lose radius = 1.2 × trigger radius** (so `loseRadiusSq = 1.44 × triggerRadiusSq`, computed at spawn). Then the enemy is **Idle for 5 s**, then Wander. The same rule is used for Attack: it starts inside the attack range and ends outside 1.2 × the attack range | Developer review (Q2): an enemy at the edge of its range must not flicker between states. Both radii are compared squared, so there is still no square root |
| E‑D7 | Gun → enemy damage | The `Gun` asks a `HitscanTargets` interface (made by the enemy side) for the closest enemy sphere in front of the wall hit. `ShotFiredEvent` gets `enemy` (entity id) and `damage`. The enemy side applies the damage when the event is dispatched (end of frame) and checks the id's **generation** first | Same rule as `ActorHandle`: never keep a raw pointer across frames. An enemy killed by an earlier event, or a recycled slot, is ignored safely |
| E‑D8 | Tuning per type | Three entries in a const **archetype table**. Their values are **copied into the components at spawn** | Each system reads only its own components and never the table. It also lets the console (and later wave scaling) tune one enemy without touching the others |
| E‑D9 | Wave data | **Spawn points come from the level file** (an `EnemySpawnPoint` component on empty actors, so they can be moved in the editor). **The wave table is in code** in v1 | Spawn points are level layout. A table of 3 waves does not need a file format yet. The base plan §9 suggested level data, which can come later (§12) |
| E‑D10 | Player health | `PlayerState` gets health (100) and `ApplyDamage`, and respects `god`. 0 HP → `GameState::LoseGame()` | `PlayerState` already holds the player's numbers, and `GameState` already owns win and lose |
| E‑D11 | Win rule | **Last wave cleared → `GameState::WinGame()`.** The 5‑minute `ArenaTimer` is not in this plan | The task asks for enemies. The timer is its own item in the base plan §9 |
| E‑D12 | Where things live | **ECS storage** (`Entity`, `ComponentPool`, `Registry`) is **engine‑level**, header‑only, std only, no systems. **AI, components and systems** are **game‑level** (`game/enemies/`) | §10 answers both questions |
| E‑D13 | Tick rates | AI and movement run in the **fixed step (50 Hz)** via `Application::FixedUpdate`. Animation and rendering run **every frame** | Same split as the player: simulation is deterministic and frozen when paused, visuals are smooth |
| E‑D14 | New primitive | `Primitive::Pyramid` = `GenMeshCone(0.5, 1, 4)` (4 slices make a square pyramid), base on y = 0 like the cylinder | The task asks for a sphere and a pyramid. It is a one‑line generator next to the existing ones |
| E‑D15 | Shot outside the chase range ("fake aggro") | The shot handler stores where the player was (the shot origin) in an **`Alert`** component. On the next tick an Idle enemy switches to Wander, and a wandering enemy **stays in Wander** with its forward turned toward that point. Chase, Attack and Dying ignore it | Developer review (Q3). No aggro timer and no new state: the enemy flies toward the shooter, and the normal trigger check starts the chase if it gets close enough. Through `Alert`, the shot handler never writes `Brain` or `Motion` (rule 3) |

---

## 1. What changes, at a glance

### 1.1 Add

| What | Where | Notes |
|---|---|---|
| `Entity`, `ComponentPool<T>`, `Registry` | `engine/ecs/` (new, header‑only) | §4 |
| `Primitive::Pyramid` (`mesh=pyramid`) | `engine/resources/ResourceManager` | E‑D14 |
| Enemy components (plain structs) | `game/enemies/EnemyComponents.h` | §5.2 |
| `EnemyType`, archetype table (Drifter, Stalker, Brute) | `game/enemies/EnemyTypes.h/.cpp` | §8 |
| `EnemyPool` | `game/enemies/EnemyPool.h/.cpp` | Fixed capacity, acquire and release, spawn from an archetype (§6) |
| `EnemyManager` | `game/enemies/EnemyManager.h/.cpp` | Owns the registry and the pool, runs the systems in order, implements `HitscanTargets`, handles shots (§3.2) |
| `EnemyBrain` (the state machine) | `game/enemies/ai/EnemyBrain.h/.cpp` | §7 |
| Systems (free functions) | `game/enemies/systems/` | `SenseSystems`, `MoveSystems`, `CombatSystems`, `PresentSystems` (§5.3) |
| `EnemyDebugDraw` | `game/enemies/` | `ai_debug 1`: trigger spheres, state labels, forward rays |
| `WaveDirector` | `game/flow/WaveDirector.h/.cpp` | 3 waves, staggered spawns, next wave when the room is clear (§6.2) |
| `EnemySpawnPoint` component | `game/flow/EnemySpawnPoint.h/.cpp` | Registered in the `ComponentRegistry`, saved in level files |
| `HitscanTargets` interface | `game/player/HitscanTargets.h` | What the gun asks for (E‑D7) |
| Events | game | `PlayerHitEvent`, `PlayerDiedEvent`, `EnemyKilledEvent`, `WaveStartedEvent`, `WavesClearedEvent` |
| Enemy console commands | `game/dev/GameCommands.cpp` | §9.3 |

### 1.2 Change

| What | Change |
|---|---|
| `Gun` | `Bind(..., const HitscanTargets*)`. A shot tests enemies up to the wall distance. `ShotFiredEvent` + `enemy`, `damage`, `origin` (E‑D15). `GunAim::isTarget` = "on an enemy" (was "on a Prop") |
| `PlayerState` | + `health`, `MAX_HEALTH = 100`, `GUN_DAMAGE = 10`, `ApplyDamage(int) -> bool died`, `IsAlive`. `Reset()` refills the health |
| `ArenaGame` | Owns `EnemyManager` and `WaveDirector`. Overrides `FixedUpdate` and `SubmitRenderables`. Resets enemies and waves in `OnLevelLoaded` and wires player death to `LoseGame` |
| `GunHUD` | Health bar, "Wave n / 3 · k left", short red flash on `PlayerHitEvent` |
| `SoundFXBank` | `PlayEnemyHit`, `PlayEnemyDeath`, `PlayPlayerHurt` (placeholders: the ping at different pitches) |
| `arena_01.lvl` | + 6 actors `"Spawn W1"` … `"Spawn E3"` with `component EnemySpawnPoint` (§6.2) |

Nothing is removed.

---

## 2. Repository layout (target)

`+` added, `~` changed.

```
├── assets/levels/arena_01.lvl          ~ + 6 EnemySpawnPoint actors
└── src/
    ├── engine/
    │   ├── eng.h                       ~ + ecs/Registry.h
    │   ├── ecs/                        + Entity.h, ComponentPool.h, Registry.h   (header-only, std only)
    │   └── resources/ResourceManager   ~ + Primitive::Pyramid
    └── game/
        ├── ArenaGame.h/.cpp            ~ FixedUpdate, SubmitRenderables, owns EnemyManager + WaveDirector
        ├── enemies/                    +
        │   ├── EnemyComponents.h         plain structs (§5.2), incl. Alert (E-D15)
        │   ├── EnemyTypes.h/.cpp         EnemyType enum + archetype table (§8)
        │   ├── EnemyPool.h/.cpp          capacity 32, acquire / release (§6.1)
        │   ├── EnemyManager.h/.cpp       registry, system order, HitscanTargets, shot handling
        │   ├── EnemyDebugDraw.h/.cpp     ai_debug
        │   ├── ai/EnemyBrain.h/.cpp      state machine: transitions only (§7)
        │   └── systems/
        │       ├── SenseSystems.cpp      ScanForPlayer
        │       ├── MoveSystems.cpp       IdleFloat, Wander, Chase, AvoidOthers, Movement
        │       ├── CombatSystems.cpp     DealDamage, Life (death), Despawn
        │       ├── PresentSystems.cpp    Animation, Render
        │       └── Systems.h             the declarations + SystemContext
        ├── flow/                       ~ + WaveDirector, EnemySpawnPoint
        ├── player/                     ~ Gun (HitscanTargets, damage), + HitscanTargets.h
        ├── state/                      ~ PlayerState (health), SoundFXBank (3 sounds)
        ├── ui/GunHUD                   ~ health, wave counter, hurt flash
        └── dev/GameCommands.cpp        ~ enemy / wave / ai commands
```

---

## 3. Architecture

### 3.1 Layers

```
+=====================================================================================+
|                 APPLICATION LAYER   (src/game)                                       |
|                                                                                     |
|  ArenaGame ── owns ──> EnemyManager ─────────────┐        WaveDirector (flow/)      |
|                         │ eng::ecs::Registry     │ <── Spawn(type, pos) ── │         |
|                         │ EnemyPool (cap 32)     │                         │ reads   |
|                         │ systems (fixed order)  │ ── EnemyKilledEvent ──> │ spawn   |
|                         │ EnemyBrain (ai/)       │                           points  |
|                         └─ implements HitscanTargets <── Gun (player/)              |
|                                                                                     |
|  PlayerState (health) <── PlayerHitEvent ── DealDamage system                      |
|  GameState  <── PlayerDiedEvent / WavesClearedEvent                                  |
+==========================================^==========================================+
                                           |  #include <eng.h>
+==========================================|==========================================+
|                        ENGINE   (src/engine, namespace eng)                          |
|   ecs/  Entity · ComponentPool<T> · Registry      (storage only: no systems)        |
|   PhysicsWorld::Raycast (walls)  · RenderQueue (draws)  · ParticleSystem (bursts)   |
|   EventBus · DebugDraw (ai_debug) · ResourceManager (+ Pyramid) · ComponentRegistry |
+=====================================================================================+
```

Rules that do not change: the engine never includes `src/game`, and the game never includes Bullet. New rules:

1. **`engine/ecs/` includes only the standard library.** No raylib, no other engine header. Nothing else in the
   engine uses it in v1. It is a library the game (and later NPCs and effects) builds on.
2. **ECS entities are runtime only.** They are not saved in level files and the editor does not show them. Authored
   content stays `GameActor`s (spawn points are actors, enemies are entities).
3. **Only `MovementSystem` writes `Transform.position`** (spawning in the pool aside). Behaviour systems write the
   velocity. `EnemyBrain` writes only `Brain`. Each field has one writer, so system order bugs are easy to find.
   The one shared component is `Alert` (E‑D15): the shot handler raises it, and `WanderSystem` / `ChaseSystem`
   clear it.
4. **No system keeps a pointer or reference into a pool after it returns.** A swap‑remove moves elements. Keep
   `Entity` ids, which `Registry::IsAlive` checks by generation.
5. **Destroying is deferred.** Systems mark entities (`Life::isDead`). `EnemyPool::FlushReleased()` frees them at
   the end of `EnemyManager::FixedUpdate`, like `Level::FlushDestroyed`.
6. **No allocation after `Init`.** Every pool reserves its capacity once. Spawning, dying and waves only move data
   inside reserved memory.

### 3.2 Ownership

```
ArenaGame
  ├─ ... (unchanged)
  ├─ EnemyManager m_enemies
  │    ├─ eng::ecs::Registry   (capacity 32; one ComponentPool per component type, made in Init)
  │    ├─ EnemyPool            (free list, counts per type; NON-owning ref to the registry)
  │    ├─ const Mesh* sphere, pyramid · Material body[3], bodyFlash[3], spike[3]   (resources, shared)
  │    └─ EventBus token: ShotFiredEvent → damage
  └─ WaveDirector m_waves
       ├─ wave table (const, code) · spawn points (Vector3 list, copied from the level on load)
       └─ EventBus tokens: EnemyKilledEvent
```

`EnemyManager::Init` registers the component pools. `Destroy` unsubscribes. `Reset` (level load, restart) clears
the registry. Pools keep their memory, so nothing is freed or allocated again.

### 3.3 Frame and fixed‑step flow

```
Engine::Run()
│  ├─ fixed loop (only while simulating: Play, console closed or con_pause 0)
│  │    level.FixedUpdate      → FirstPersonController, Gun (shot: Bullet ray, then HitscanTargets::Raycast)
│  │    app.FixedUpdate        → ArenaGame::FixedUpdate
│  │         if GameState is Playing:
│  │           m_waves.FixedUpdate(dt, m_enemies)      staggered spawns, wave clear check
│  │           m_enemies.FixedUpdate(dt, playerInfo)   ── the system order in §5.3 ──
│  │    physics.Step
│  ├─ level.Update / particles.Update
│  ├─ app.Update               → if IsSimulating: m_enemies.Animate(dt)    (presentation only)
│  ├─ render                   → app.SubmitRenderables → m_enemies.Submit(queue)   (2 commands per enemy)
│  │                             DrawHUD: health, wave counter
│  └─ events.Dispatch()        → ShotFiredEvent: EnemyManager applies damage (generation-checked)
│                                PlayerHitEvent: PlayerState::ApplyDamage → PlayerDiedEvent → LoseGame
│                                EnemyKilledEvent: WaveDirector counts · WavesClearedEvent: WinGame
```

`playerInfo` = `{ position of the player's chest (feet + 1.2 m), isAlive }`. `ArenaGame` reads it from `m_player`
(an `ActorHandle`) once per tick. The enemy code never sees a `GameActor`.

---

## 4. The tiny ECS (`engine/ecs/`)

### 4.1 What it is (and is not)

It is a **store**: ids, one packed array per component type, add / remove / get, and iteration over the entities
that have a set of components. It has **no** systems, no scheduler, no events, no reflection, no serialization and
no archetype graph. Those stay in the code that uses it. About 250 lines in three headers.

### 4.2 Entity

```cpp
namespace eng::ecs
{
	/// 4 bytes: a slot index and how many times that slot was reused.
	/// A stale id (the slot was released and taken again) has an old generation and is rejected.
	struct Entity
	{
		uint16_t	index		= INVALID_INDEX;
		uint16_t	generation	= 0;
		static constexpr uint16_t INVALID_INDEX = 0xFFFF;
		bool operator == (const Entity&) const = default;
	};
}
```

`uint16_t` caps one registry at 65 535 slots. That is plenty for enemies, NPCs and effects, and it halves the
sparse arrays (below) compared with 32‑bit indices.

Using `0xFFFF` as a sentinel value for an invalid or null entity ID is a pattern in Entity Component System (ECS) architectures. Since `0xFFFF` is the maximum value of a 16-bit unsigned integer (`uint16_t` / `u16`), reserving it gives you 65,535 valid entities (indices 0 through 65534). It keeps component arrays and sparse sets compact, so more data fits into CPU cache lines. Checking `entity == INVALID_ENTITY` compiles down to a single, fast CPU instruction. And because valid IDs start at 0, you can use an entity ID directly as an index into a dense array or sparse set without subtracting 1.


### 4.3 ComponentPool<T> (a sparse set)

```cpp
template<typename T>						// T: a plain struct, trivially copyable (static_assert)
class ComponentPool final : public IComponentPool
{
public:
	explicit ComponentPool(uint16_t capacity);	// reserves dense arrays; sparse array sized to capacity, filled with NONE
	T&			Add(Entity e, const T& value);	// e must not already have T
	void		Remove(Entity e) override;		// swap the last element into the hole, pop: stays packed
	bool		Has(Entity e) const;
	T*			Get(Entity e);					// nullptr if missing
	std::span<T>				Data();			// packed, in dense order
	std::span<const Entity>		Entities() const;
	size_t		Size() const;
private:
	std::vector<T>			m_dense;			// the components, packed, no holes
	std::vector<Entity>		m_denseEntities;	// m_dense[i] belongs to m_denseEntities[i]
	std::vector<uint16_t>	m_sparse;			// entity index → dense index (NONE if missing)
};
```

### 4.4 Registry

```cpp
class Registry
{
public:
	explicit Registry(uint16_t capacity);

	template<typename T> void	RegisterComponent();	// makes the pool; call at Init, never in the hot path
	Entity		Create();								// from the free list; invalid Entity if full
	void		Destroy(Entity e);						// removes it from every pool, bumps the generation
	bool		IsAlive(Entity e) const;
	void		Clear();								// destroys all, keeps memory

	template<typename T> T&		Add(Entity e, const T& value = {});
	template<typename T> void	Remove(Entity e);
	template<typename T> T*		Get(Entity e);
	template<typename T> ComponentPool<T>& Pool();

	/// f(Entity, A&, B&, ...) for every entity that has all of them. It walks the pool of A
	/// (put the rarest component first) and looks up the others through their sparse arrays.
	template<typename A, typename... Rest, typename F>
	void		Each(F&& f);

	size_t		GetAliveCount() const;
	uint16_t	GetCapacity() const;
private:
	std::vector<uint16_t>							m_generations;	// per slot
	std::vector<uint16_t>							m_freeList;		// LIFO: reuses warm slots first
	std::vector<std::unique_ptr<IComponentPool>>	m_pools;		// indexed by ComponentTypeIndex<T>()
};
```

The type index uses the same idiom as `EventBus::TypeId`: one static per type, but a sequential `uint8_t` counter
instead of an address, so `m_pools` is a small array instead of a hash map. Only `Destroy` and `Clear` use the
`IComponentPool` virtual (once per pool for each destroy). Iteration never goes through a virtual call: `Each` is a
template, so the lambda is inlined.

---

## 5. Enemies as entities

### 5.1 Why (cache and memory)

Be honest about the numbers: **at 20 enemies, any layout runs fast.** 20 × 11 systems at 50 Hz is about 11 000 tiny
function bodies per second. What costs time is the draws (40 per frame) and the wall raycasts (about 1 000 per second).
The layout choices below are about **steady frame times, safety, and the later uses** (NPC crowds, projectiles,
effects with hundreds or thousands of entities), not about 20 enemies.

| Choice | Motivation |
|---|---|
| One packed array per component (struct of arrays at the component level) | A system walks only the arrays it uses. For example, `AvoidOthers` reads positions and radii: 20 × (12 + 4) B = 320 B, which is 5 cache lines, prefetched in order. With `GameActor`s the same loop goes actor → component vector → heap component → `dynamic_cast`, which is about 4 dependent pointer loads per enemy, each likely a cache miss |
| Plain structs, no virtuals in components | No vtable pointer (8 B) in each element and no indirect call per element. The compiler can inline and vectorize the loops |
| Fixed capacity, reserved at `Init` (32 enemies) | No allocation during play: no allocator time spikes, no memory spread across the heap, and no pointer invalidation in the middle of a tick (rule 6). The engine's `ParticleSystem` already does this ("one fixed pool, no allocation per burst") |
| Hot / cold split | Simulation data (`Transform`, `Motion`, `Brain`, …) is kept apart from render‑only data (`EnemyLook`: material pointers, animation phases). Simulation never pulls render data into cache, and the other way round |
| Tuning copied into components at spawn (E‑D8) | A system reads one contiguous component instead of jumping to a table per entity. It costs a few bytes per enemy. The table (3 × ~80 B) stays as the source for spawning |
| `uint16_t` ids and sparse arrays | Sparse arrays are 2 B per slot: 32 slots × 13 pools = 832 B. Ids are 4 B, so they fit in events and in `ShotFiredEvent` by value |
| LIFO free list | A released slot is taken first on the next spawn, so it is likely still in cache |
| A 4‑byte xorshift RNG per enemy (in `Wander`) | `std::mt19937` is 2.5 KB of state. One per enemy would be larger than all the other enemy data together. A shared one makes the result depend on system order |
| Analytic spheres instead of Bullet bodies (E‑D2) | A Bullet body is several heap objects (shape, motion state, body, broadphase proxy) and adds work to every physics step. A sphere is 4 bytes (`radius`) |

**Memory budget** (capacity 32, sizes rounded to 4‑byte alignment):

| Component | Fields | Bytes |
|---|---|---|
| `Transform` | position (12), rotation quaternion (16) | 28 |
| `Motion` | velocity (12), forward (12), turnRateRad (4) | 28 |
| `Brain` | state (1, padded), stateTime (4), idleDuration (4) | 12 |
| `ScanForPlayer` | triggerRadiusSq, loseRadiusSq (E‑D6), distSq (out), toPlayerDir (out, 12) | 24 |
| `Wander` | speed, probeDistance, rngState | 12 |
| `ChasePlayer` | speed | 4 |
| `AvoidOthers` | strength | 4 |
| `DealDamage` | damage, rangeSq, exitRangeSq (E‑D6), cooldown, cooldownLeft | 20 |
| `Life` | hp, maxHp, hitFlash, dyingTime, isDead (padded) | 20 |
| `Alert` | shotFrom (12), pending (1, padded) (E‑D15) | 16 |
| `CollisionSphere` | radius | 4 |
| `EnemyKind` | type (1, padded to 4) | 4 |
| `EnemyLook` (cold) | 2 material pointers (16), scale, bobPhase, lunge, renderRotation (16) | 44 |
| **Per enemy** | components 220 + dense entity ids 13 × 4 = 52 | **≈ 272** |
| **All 32 slots** | 32 × 272 + sparse 832 + generations / free list 128 | **≈ 9.4 KB** |

9.4 KB fits in L1 (an M2 performance core has a 128 KB L1 data cache), so all enemy data stays in L1 for the whole
tick.

### 5.2 Components

These follow the task's list. Each is a plain struct in `EnemyComponents.h`:

| Task's component | v1 component | Read by | Written by |
|---|---|---|---|
| Transform | `Transform` | most systems | `MovementSystem` (position), `WanderSystem` / `ChaseSystem` (rotation) |
| — | `Motion` (velocity, forward, turn rate) | Movement, Wander, Chase, Animation | Idle, Wander, Chase, AvoidOthers |
| (state machine) | `Brain` | every behaviour system | `EnemyBrain` only |
| Scan for nearby player | `ScanForPlayer` (trigger and lose radius) | Brain, Chase, DealDamage | `ScanSystem` |
| Wander | `Wander` | Wander | Wander (RNG) |
| Chase player | `ChasePlayer` | Chase | — |
| Avoid others | `AvoidOthers` | AvoidOthers | — |
| Deal damage to player | `DealDamage` | Brain (range), DealDamage | DealDamage (cooldown) |
| Life | `Life` | Brain, Life, Animation | shot handler (hp, hitFlash), Life (dying) |
| (fake aggro, E‑D15) | `Alert` | Brain, Wander | shot handler (raises), Wander / Chase (clear) |
| Collision detection | `CollisionSphere` | Movement, AvoidOthers, `HitscanTargets` | — |
| Mesh | `EnemyLook` (+ `EnemyKind`) | Animation, Render | Animation |

### 5.3 Systems and their order

Each system is a free function: `void WanderSystem(eng::ecs::Registry&, const SystemContext&, float dt)`.
`SystemContext` = `{ PhysicsWorld& physics, Vector3 playerTarget, bool playerAlive, EventBus& events,
ParticleSystem& particles, const ArenaBounds& bounds }`. `EnemyManager::FixedUpdate` calls them in this order.
There is no scheduler: the order is the code.

| # | System | Iterates | Does |
|---|---|---|---|
| 1 | `ScanSystem` | `Transform, ScanForPlayer` | `distSq = |player − pos|²`, `toPlayerDir`. No square root unless needed (the direction is normalized only when `distSq < loseRadiusSq`) |
| 2 | `BrainSystem` (`EnemyBrain`) | `Brain, ScanForPlayer, DealDamage, Life, Alert` | State transitions and state timers (§7). Writes only `Brain` |
| 3 | `IdleFloatSystem` | `Brain, Motion` | Idle only: `velocity = (0, A·ω·cos(ω·t), 0)`. Moves through velocity, so `MovementSystem` stays the only writer of position (rule 3) |
| 4 | `WanderSystem` | `Brain, Transform, Motion, Wander, CollisionSphere, Alert` | Wander only: a raised `Alert` turns `forward` toward the shooter first (E‑D15); then one forward probe ray, and a new direction when there is a wall (§7.3) |
| 5 | `ChaseSystem` | `Brain, Transform, Motion, ChasePlayer, ScanForPlayer, Alert` | Chase and Attack: clears a raised `Alert` (the enemy already knows where the player is); turn `forward` toward the player, limited by `turnRateRad · dt`, `velocity = forward · speed` (slows to 0 when in attack range) |
| 6 | `AvoidOthersSystem` | `Transform, Motion, CollisionSphere, AvoidOthers` | Every pair closer than `rᵢ + rⱼ + 0.5 m`: push the velocities apart, scaled by the overlap. O(n²): 20 enemies make 190 pairs |
| 7 | `MovementSystem` | `Transform, Motion, CollisionSphere` | `pos += velocity · dt`. Sweep ray for walls: stop at `hit − radius`, slide (`v −= (v·n)n`). Clamp y to the height band. Solve sphere overlaps left after step 6 (push each enemy half the overlap) |
| 8 | `DealDamageSystem` | `Brain, DealDamage, ScanForPlayer` | Ticks `cooldownLeft`. In Attack with `cooldownLeft ≤ 0`: `Publish(PlayerHitEvent{damage, entity})`, `cooldownLeft = cooldown`, starts the lunge animation |
| 9 | `LifeSystem` | `Life, Brain, Transform` | `hp ≤ 0` → Dying (0.25 s), then `isDead` + particle burst + `EnemyKilledEvent{type}` |
| 10 | `EnemyPool::FlushReleased` | dead entities | `Registry::Destroy`, slot back on the free list (rule 5) |

Per frame (`Animate`, when simulating) and in render (`Submit`):

| System | Does |
|---|---|
| `AnimationSystem` | Kinematic animation only (§8.2): smooths the render rotation toward `Transform.rotation`, bob, lunge, hit flash timer, death shrink |
| `RenderSystem` | Two `RenderCommand`s per enemy: the sphere (body) and the pyramid (spike pointing forward). The flash material is used while `hitFlash > 0` |

---

## 6. Pool and wave spawner

### 6.1 `EnemyPool`

```cpp
class EnemyPool
{
public:
	static constexpr uint16_t CAPACITY = 32;

	void			Init(eng::ecs::Registry& registry);		// registers every enemy component, reserves CAPACITY
	eng::ecs::Entity Spawn(EnemyType type, const Vector3& position, const Vector3& forward, uint32_t seed);
	void			FlushReleased();						// destroys entities whose Life::isDead is set
	void			Clear();
	int				GetAliveCount() const;
	int				GetAliveCount(EnemyType type) const;
	bool			IsFull() const;
};
```

`Spawn` takes a slot, adds every component with values from the archetype (§8), starts in **Idle** with a 0.3 s
scale‑in, and emits a small particle burst. When the pool is full it returns an invalid `Entity` and logs a warning
(the console prints it). The pool is the only place that creates or destroys enemy entities.

The slots are reused through the registry's free list, and the pool memory is reserved for the registry's lifetime.
So the "pool" is **pre‑reserved storage with recycled ids**, not a list of pre‑built objects that get switched on.
This means no dormant enemies to skip in every loop, and no "is active" flag to forget.

### 6.2 `WaveDirector`

```cpp
struct WaveEntry	{ EnemyType type; int count; };
struct Wave			{ std::array<WaveEntry, 3> entries; float spawnInterval; };	// seconds between two spawns

// v1 table, 20 enemies in total (E-D4, developer's table from the review)
constexpr Wave WAVES[] =
{
	{ { { { Drifter, 7 }, { Stalker, 0 }, { Brute, 0 } } }, 0.6f },	// 7
	{ { { { Drifter, 5 }, { Stalker, 2 }, { Brute, 0 } } }, 0.5f },	// 7
	{ { { { Drifter, 3 }, { Stalker, 2 }, { Brute, 1 } } }, 0.5f },	// 6
};
```

```
  Waiting(2 s) ──> Spawning ──(all of the wave spawned)──> Fighting ──(alive == 0)──┐
     ^                 │ one enemy every spawnInterval:                          │
     │                 │ next spawn point (round robin), skipped if the player   │
     │                 │ is closer than 4 m or an enemy is closer than 1.5 m     │
     └──── next wave ──┴─────────────────────────────────────────────────────────┤
                                                    last wave ──> WavesClearedEvent → GameState::WinGame
```

- Spawn points: on `OnLevelLoaded`, `ArenaGame` collects the world position of every actor with an
  `EnemySpawnPoint` (fields: `enabled`). With none, the director logs a warning and uses the 4 upper corners of the
  arena bounds.
- Arena bounds (`ArenaBounds` in `SystemContext`: the height band and the fallback corners) come from the world box
  of the actor named `"Arena"` (the render cube), shrunk by 1 m. Walls themselves are still found by raycasts.
- It counts deaths with `EnemyKilledEvent` and asks the pool for the alive count (the pool is the source of truth).
- Publishes `WaveStartedEvent{index, count}` (HUD, sound) and `WavesClearedEvent`.
- Ticks only while `GameState` is Playing. `Reset()` on level load and restart.

The developer lengthened the arena (interior x −8 … 8, y 0 … 6, z −33 … 8, snapped: render cube 16 × 6 × 41 centred at z −12.5, slabs 0.5 m thick outside it). `arena_01.lvl` gets **six**
spawn actors, two rows along the side walls: (±6, 3, 0), (±6, 3, −14), (±6, 3, −28), named `"Spawn W1"` …
`"Spawn E3"`. The player spawns at (0, 0, 6), so the nearest spawn point is 9 m away when the first wave starts, and
the far rows make enemies cross the room toward the player.

---

## 7. AI state machine

### 7.1 States and transitions

```
                         spawn (0.3 s scale-in)
                                 │
                                 v
        ┌──────────────────> [ IDLE ] ── stateTime ≥ 5 s ──> [ WANDER ]
        │   float up / down     │                               │
        │                        │ distSq < triggerRadiusSq      │ distSq < triggerRadiusSq
        │                        v                               v
        │   distSq ≥ loseRadiusSq     │                               │ shot from outside the range (Alert):
        │   (1.2 × trigger, E-D6)     v                               │ stays in WANDER, forward → shooter
        └──────────────────────── [ CHASE ] <───────────────────────┘    (an IDLE enemy that is shot → WANDER)
                                │    ^
                                │    │ distSq > exitRangeSq (1.2 × attack range)
           distSq ≤ attackRangeSq    │
                                v    │
                            [ ATTACK ]   cooldownLeft ≤ 0 → hit player (DealDamageSystem), cooldownLeft = 1 s

   any state ── Life.hp ≤ 0 ──> [ DYING ] (0.25 s shrink + burst) ──> released to the pool
```

| From | Condition (checked in this order) | To |
|---|---|---|
| any | `hp ≤ 0` | Dying (final: no transition out) |
| Chase, Attack | player dead | Idle (enemies stop attacking a dead player) |
| Idle | `distSq < triggerRadiusSq` | Chase |
| Idle | `Alert.pending` (shot from outside the range) | Wander (`WanderSystem` turns it toward the shooter, E‑D15) |
| Idle | `stateTime ≥ 5 s` | Wander |
| Wander | `distSq < triggerRadiusSq` | Chase |
| Wander | `Alert.pending` | *stays* Wander (`WanderSystem` turns it toward the shooter, E‑D15) |
| Chase | `distSq ≥ loseRadiusSq` (1.2 × trigger) | Idle (5 s, E‑D6) |
| Chase | `distSq ≤ attackRangeSq` | Attack |
| Attack | `distSq ≥ loseRadiusSq` | Idle |
| Attack | `distSq > exitRangeSq` (1.2 × attack range) | Chase |

Between the two radii nothing changes: an enemy that is not chasing does not start, and one that is chasing does
not stop. That band is what stops the flicker (E‑D6).

### 7.2 Implementation

```cpp
enum class AiState : uint8_t { Idle, Wander, Chase, Attack, Dying };

struct Brain
{
	AiState	state			= AiState::Idle;
	float	stateTime		= 0.0f;		// seconds in the current state
	float	idleDuration	= 5.0f;
};

// ai/EnemyBrain.cpp — the whole state machine is one function with a switch
AiState NextState(const Brain& brain, const ScanForPlayer& scan, const DealDamage& attack,
				  const Life& life, const Alert& alert, bool playerAlive);

void BrainSystem(eng::ecs::Registry& registry, const SystemContext& ctx, float dt);	// stateTime += dt; on change: stateTime = 0, OnEnter
```

A switch on an enum, not state objects with virtual `Enter / Update / Exit`, for these reasons:

- 5 states. The rules fit in one screen and read top to bottom (the table above *is* the code).
- `NextState` is a **pure function** (data in, state out), so it is easy to test and to print (`ai_debug`).
- The **decision** (which state) is kept apart from the **behaviour** (what each state does to the velocity, in its
  own system). This is the seam that later AI work plugs into (§10.1): pathfinding changes how `ChaseSystem` picks
  its direction, not the state machine.

`OnEnter` is a small switch too. For example, entering Wander picks a first direction. Entering Idle stores the
current height as the bob centre.

### 7.3 Behaviours in detail

**Idle.** `y = baseY + A · sin(ω · stateTime)`, with A = 0.25 m and a 2.5 s period (ω = 2π / 2.5), written as a
velocity (`A·ω·cos`). Horizontal velocity eases to 0. After 5 s → Wander.

**Wander** (as in the task):

1. Move along `forward` at `Wander.speed`.
2. Every tick, cast one ray along `forward`, length `radius + probeDistance` (probe = speed × 0.4 s, at least
   1 m), mask `Ground | Prop`. No hit → done.
3. Hit (a wall is near): draw a random point **inside the unit sphere** with rejection sampling (xorshift: x, y, z
   in [−1, 1], retry while |p|² > 1 or |p|² < 0.01). Damp y (×0.35) and normalize.
4. **Check for room:** cast a ray along the candidate, length `radius + minRoom` (2 m), and reject it if the ray
   hits or the move would leave the height band. Up to **8 tries**. If all fail, use the reflection of `forward`
   off the hit normal (that always has room in a convex room).
5. Set `forward` = candidate and set `Transform.rotation` from it (yaw = `atan2(−f.x, −f.z)`, pitch = `asin(f.y)`,
   no roll, in the `MathUtils` helper `LookRotation(forward)`). The simulation **snaps**. `AnimationSystem`
   slerps the render rotation toward it over about 0.2 s, so the turn looks smooth.

Cost: 1 ray per wandering enemy per tick, plus up to 8 rays on the tick it meets a wall. Worst case 20 × 50 =
1 000 rays per second, cheap for Bullet against 7 static boxes.

**Shot from outside the chase range** (fake aggro, E‑D15). When `Alert.pending` is set, `WanderSystem` does this
before step 1, once: `forward = normalize(Alert.shotFrom − position)`, rotation from forward (same snap + smooth turn
as step 5), then `pending = false`. No room check is needed: the shot reached the enemy, so the line toward the
shooter is open. The enemy then wanders on along that line. If it gets within its trigger radius, the normal rule
starts the chase. If the player has moved away, it reaches the wall behind the shot position and wanders as usual.
`shotFrom` is the shot's origin (the camera), so the enemy also turns up or down toward the player's eye height.
The height band still clamps it.

**Chase.** `desired = toPlayerDir`. Rotate `forward` toward it by at most `turnRateRad · dt` (slerp on the
direction), `velocity = forward · ChasePlayer.speed`. The target is the player's chest, so enemies come to face
height. A limited turn rate is what makes the types differ beyond speed: a Drifter can be side‑stepped, a Brute
cannot.

**Attack.** The enemy stays and faces the player (Chase steering with speed 0). `DealDamageSystem` hits when the
cooldown is ready. A lunge animation (§8.2) sells the bite. The hit has no projectile and no miss chance: in range
means hit.

**Avoid others.** Separation, as an additive velocity: for each close pair, `push = (overlap / minDist) · strength`
along the line between the two centres, half to each. A hard overlap fix in `MovementSystem` keeps spheres from
sitting inside each other in a crowd at the player.

---

## 8. The three enemy types and how they look

### 8.1 Archetype table (`EnemyTypes.cpp`)

| | **Drifter** (easy) | **Stalker** (medium) | **Brute** (hard) |
|---|---|---|---|
| Body radius (sphere) | 0.45 m | 0.55 m | 0.8 m |
| Colour (body / spike) | green / pale green | orange / yellow | red / dark red |
| HP (shots at 10 dmg) | 30 (3) | 60 (6) | 120 (12) |
| Wander speed | 3.0 m/s | 2.5 m/s | 2.0 m/s |
| Chase speed | 6.0 m/s | 4.5 m/s | 3.5 m/s |
| Turn rate | 90 °/s | 180 °/s | 270 °/s |
| Trigger radius | 7 m | 10 m | 14 m |
| Lose radius (1.2 ×, E‑D6) | 8.4 m | 12 m | 16.8 m |
| Attack range (centre to chest) / exit (1.2 ×) | 1.6 / 1.92 m | 1.8 / 2.16 m | 2.3 / 2.76 m |
| Damage per hit (1 s cooldown) | 5 | 10 | 20 |
| Avoid strength | 1.0 | 1.5 | 0.5 (pushes through) |

The Drifter is the fastest, to balance its weakness (E‑D18). Context: the player walks at 10 m/s, has 100 HP, and the gun fires 4 shots per second at 10 damage (40 DPS).
A Brute takes 3 s of steady fire and kills the player in 5 hits. Numbers are starting points for playtests, all
tunable from the console (`enemy_tune`). The 1 s cooldown and the 5 s idle are the same for all types, as the task
says.

### 8.2 Geometry and kinematic animation

No skeleton: the two parts are separate draws with transforms computed in code.

```
        side view (forward = -Z, to the left)

             ___
      ◄────<|   |      pyramid ("spike"): Primitive::Pyramid, rotated -90° about X so its tip points
             ‾‾‾        along forward, base at the sphere's surface, scale (0.6r, 0.9r, 0.6r)
           (  ●  )     sphere ("body"):   Primitive::Sphere (r = 0.5) scaled to 2r
```

| Animation | Where | How |
|---|---|---|
| Idle bob | simulation (§7.3) | Real movement, so the hit sphere bobs too |
| Spawn | `AnimationSystem` | Scale 0 → 1 over 0.3 s (ease‑out) |
| Turn smoothing | `AnimationSystem` | Render rotation slerps toward `Transform.rotation` (rate 12/s) |
| Wander | `AnimationSystem` | Slow spin of the spike around forward (30 °/s) |
| Chase | `AnimationSystem` | Body leans forward 10°, the spike pulses (scale 1 ± 0.1 at 4 Hz) |
| Attack lunge | `AnimationSystem` | On each hit: body offset along forward 0 → 0.35 m → 0 over 0.18 s (fast out, slow back), spike scale 1.3 |
| Hit flash | `AnimationSystem` / `RenderSystem` | 0.1 s: the body uses the type's white flash material and squashes (scale 1.15 xz, 0.85 y) |
| Death | `LifeSystem` / `AnimationSystem` | 0.25 s shrink to 0 + spin, then a particle burst in the type's colour |

Materials: 3 types × (body, body flash, spike) = **9 `Material`s**, made once in `EnemyManager::Init` with the lit
shader. A per‑enemy tint would need a material per enemy, because `RenderQueue` takes material pointers.
Draw cost: 2 commands per enemy, 40 for 20 enemies (`RenderQueue` reserves 256). raylib's `DrawMeshInstanced` is
the next step if counts reach the hundreds.

Later, a mesh replaces the two primitives: only `EnemyLook` and `RenderSystem` change.

---

## 9. Damage, player health and tools

### 9.1 Gunshot → enemy

```cpp
// game/player/HitscanTargets.h
struct HitscanHit { eng::ecs::Entity entity; Vector3 point; Vector3 normal; float distance; };
class HitscanTargets
{
public:
	virtual ~HitscanTargets() = default;
	virtual std::optional<HitscanHit> Raycast(const Vector3& origin, const Vector3& dir, float maxDistance) const = 0;
};
```

`Gun::Fire`: (1) the Bullet ray against `Ground | Prop` as today sets `maxDistance` to the wall distance; (2)
`targets->Raycast(origin, forward, maxDistance)` makes a ray‑sphere test for every living enemy (20 × one quadratic)
and keeps the nearest; (3) `ShotFiredEvent{ ..., enemy, damage = PlayerState::GUN_DAMAGE }`. The tracer and the
impact burst use the enemy hit point when there is one. `GunAim` uses the same query every frame, so the crosshair
turns red **on an enemy** (until now it meant "on a Prop").

`EnemyManager` subscribes to `ShotFiredEvent`. If `registry.IsAlive(enemy)` and the enemy is not Dying:
`hp −= damage`, `hitFlash = 0.1`, `SoundFXBank::PlayEnemyHit` at the hit point. If the enemy is Idle or Wander
(the player is outside its chase range), it also raises `Alert{ shotFrom = shot origin, pending = true }` (E‑D15).
Death is then handled by `LifeSystem` on the next tick, and the alert by `BrainSystem` / `WanderSystem`.

`ShotFiredEvent` carries the shot origin for this (`origin`, the camera position; `muzzle` stays the tracer start).

The `Gun` depends only on the `HitscanTargets` interface, not on the ECS or the enemies. A later shootable (a
barrel, an NPC) only has to implement it.

### 9.2 Enemy → player

`DealDamageSystem` publishes `PlayerHitEvent{damage, from}`. `ArenaGame` handles it: if not `god`,
`PlayerState::ApplyDamage(damage)` and `PlayPlayerHurt`. When the player reaches 0 → `PlayerDiedEvent` →
`GameState::LoseGame()` → the existing "You died" dialog. `GunHUD` shows a health bar and a 0.2 s red edge flash.

### 9.3 Console commands (`GameCommands.cpp`, `ENG_DEV_CONSOLE` only)

| Command | What it does |
|---|---|
| `enemy_spawn <drifter\|stalker\|brute> [n]` | Spawns n enemies 6 m in front of the player |
| `enemy_spawn_all` | All 20 of the wave table at once (pool capacity check, E‑D4) |
| `enemy_list` | id, type, state, hp, distance |
| `kill_all` | Every enemy hp = 0 (deaths, wave counting and the win path run as normal) |
| `wave_start <n>` · `wave_skip` · `waves 0\|1` | Start wave n · clear the current one · director on / off (sandbox) |
| `ai_debug 0\|1` (cvar) | `EnemyDebugDraw`: trigger and attack spheres, forward probe rays, state label above each enemy |
| `ai_freeze 0\|1` (cvar) | Systems 2–8 skipped: enemies hang in the air (aim and damage tests) |
| `enemy_tune <type> <field> <value>` | Changes an archetype value (only for later spawns) |
| `player_hp [n]` | Prints or sets health |

---

## 10. Answers to the developer's questions

Read `docs/devlogs/about_memory_and_enemy_ai.md` only if needed.

---

## 11. Implementation plan (in order)

Same workflow as [`tasks/TASK_04.md`](tasks/TASK_04.md): one branch per phase from `integration`, one commit per
checkbox, the developer runs **Done when** and merges, the finished branch is renamed `done/<name>`. Phases are
numbered **E0–E5**. Progress is tracked in [`Enemy_System_v1_DEV_PROGRESS.md`](Enemy_System_v1_DEV_PROGRESS.md).

### Phase E0 — ECS core (`enemy_ecs_core`)
- `engine/ecs/Entity.h`, `ComponentPool.h`, `Registry.h` (§4); `eng.h` includes `ecs/Registry.h`.
- Debug self‑check at startup (`#ifndef NDEBUG`): create, add, remove, destroy and check generations. Console `ecs_selftest`.

**Done when:** Debug and Release build clean; `ecs_selftest` passes (stale id rejected, packed after removes, `Each` visits only full matches, `Clear` keeps capacity).

### Phase E1 — Pool and visible enemies (`enemy_pool_and_render`)
- `Primitive::Pyramid` (`mesh=pyramid`).
- `EnemyComponents.h`, `EnemyTypes` (3 archetypes), `EnemyPool` (capacity 32), `EnemyManager` (Init, Reset, FixedUpdate shell, Submit).
- `ArenaGame`: owns `EnemyManager`; overrides `FixedUpdate` and `SubmitRenderables`; `Reset` on level load.
- `RenderSystem` + `AnimationSystem` (spawn scale‑in, turn smoothing); 9 materials.
- Console: `enemy_spawn`, `enemy_spawn_all`, `enemy_list`, `kill_all` (removes for now).

**Done when:** `enemy_spawn drifter 5`, `stalker 3` and `brute 2` show three sizes and colours of sphere‑and‑spike enemies in front of the player; `enemy_spawn_all` shows 20; a 33rd spawn logs "pool full"; `restart` removes them; no allocation after Init (checked once with a counting allocator in Debug, or Instruments).

### Phase E2 — Idle and wander (`enemy_ai_idle_wander`)
- `Brain` + `EnemyBrain` with Idle and Wander only.
- `IdleFloatSystem`, `WanderSystem` (probe, unit‑sphere draw, room check, snap + smooth turn), `AvoidOthersSystem`, `MovementSystem` (walls, height band, overlap fix).
- `ai_debug`, `ai_freeze`.

**Done when:** spawned enemies float for 5 s, then roam; none leaves the arena or goes through the floor or ceiling in 5 minutes with 20 enemies; they turn away before walls; they do not stack inside each other; `ai_debug` shows the probes and states.

### Phase E3 — Shooting enemies (`enemy_damage`)
- `HitscanTargets` + `Gun` changes (§9.1), `ShotFiredEvent` + `enemy` / `damage`, crosshair red on enemies.
- `Life`, shot handler (generation check), hit flash, `LifeSystem` (death animation + burst + `EnemyKilledEvent`), `FlushReleased`.
- `Alert` (fake aggro, E‑D15): raised by the shot handler on Idle / Wander enemies; Idle → Wander; `WanderSystem` turns toward the shot origin.
- `SoundFXBank`: `PlayEnemyHit`, `PlayEnemyDeath`.

**Done when:** a Drifter dies in 3 hits, a Stalker in 6, a Brute in 12; each hit flashes the enemy; shots stop at a wall in front of an enemy; an enemy behind another takes no damage; `kill_all` plays every death; spawning 32 → killing all → spawning 32 again works (slot reuse); an Idle or wandering enemy shot from far away turns and flies toward where the player stood, still in Wander (`ai_debug` label).

### Phase E4 — Chase and attack (`enemy_ai_chase_attack`)
- `ScanForPlayer` + `ScanSystem`; Brain gets Chase, Attack, Dying and the player‑dead rule; hysteresis (lose radius and attack exit at 1.2 ×, E‑D6); `ai_debug` draws the trigger and lose spheres.
- `ChaseSystem` (turn rate), `DealDamageSystem` (1 s cooldown, keeps ticking, E‑D5), lunge animation.
- `PlayerState` health, `PlayerHitEvent` → `ApplyDamage`, `PlayerDiedEvent` → `LoseGame`; `god` respected; HUD health + hurt flash; `PlayPlayerHurt`; `player_hp`.

**Done when:** an enemy chases when the player enters its trigger radius and goes idle (5 s) only once the player is beyond 1.2 × that radius; standing on the edge of either radius does not make it flicker between states; in range it lunges and hits once per second for its damage; walking out of range and back does not reset the cooldown; at 0 HP the "You died" dialog shows; `god 1` takes no damage; the three types feel clearly harder in order.

### Phase E5 — Waves (`enemy_waves`)
- `EnemySpawnPoint` component (registered), 6 spawn actors in `arena_01.lvl` (§6.2).
- `WaveDirector` (table §6.2, staggered spawns, spawn safety rules, `WaveStartedEvent`, `WavesClearedEvent` → `WinGame`).
- HUD "Wave n / 3 · k left"; console `wave_start`, `wave_skip`, `waves`; `enemy_tune`.

**Done when:** Start → wave 1 (7 Drifters) spawns one by one at the spawn points; wave 2 is 5 Drifters + 2 Stalkers, wave 3 is 3 Drifters + 2 Stalkers + 1 Brute; clearing it starts wave 2 after 2 s, then wave 3; 20 enemies in total; killing the last one shows "Arena cleared"; dying in any wave shows "You died", and Retry starts again from wave 1 with full health; spawn points moved in the editor and saved are used after a reload.

---

## 12. Risks, open questions and later work

| # | Risk | Mitigation |
|---|---|---|
| R1 | Fixed‑step movement (50 Hz) looks choppy at 120 Hz | `AnimationSystem` smooths rotation. If position judder shows, store `prevPosition` in `Transform` (+12 B) and render `lerp(prev, pos, Time::GetFixedAlpha())` |
| R2 | Enemies clip through walls at high speed or in corners | Sweep ray in `MovementSystem` + height band + the arena's 0.5 m slabs. Speeds ≤ 7 m/s are 0.14 m per tick, well under any radius |
| R3 | Wander gets stuck in a corner (every candidate rejected) | 8 tries, then reflect off the hit normal. `ai_debug` shows the probes |
| R4 | A stale `Entity` in a `ShotFiredEvent` hits a recycled slot | Generation check (E‑D7). Slots are released only in `FlushReleased`, after the systems run |
| R5 | Enemies are invisible to Bullet: physics props pass through them, and `Picker` and `OverlapBox` ignore them | Accepted for v1 (E‑D2). §10.2 names the bridge (ghost objects) if needed |
| R6 | Editor and enemies: the editor freezes the world and its `Reset` (`LevelSnapshot`) does not know about entities | Enemies are runtime only (rule 2). `restart` is the reset. Entering the editor keeps them frozen in place |
| R7 | O(n²) avoidance at higher counts | Acceptable for now. Fine up to ~200 (20 000 pairs). Beyond that, a uniform grid over the arena (cell = 2 m) inside `AvoidOthersSystem` |
| R8 | `GenMeshCone` with 4 slices may lack a base cap or have odd normals | Check in E1. Fall back to a hand‑built 5‑vertex, 6‑triangle mesh in `ResourceManager` |
| R9 | ~~The lengthened arena's render cube and slabs did not line up (up to ~0.2 m at the north wall)~~ | **Resolved** (Q6): values snapped in `arena_01.lvl`. Render cube `0 3 −12.5` scale `16 6 41`; floor / ceiling at y −0.25 / 6.25 scale `17 0.5 42`; walls N / S at z −33.25 / 8.25, E / W at x ±8.25 scale `0.5 7 42`, all centred at z −12.5 |
| R10 | "Aggro on hit" is faked (E‑D15): a shot enemy flies toward where the player *was*. If the player moves away it may never find them | Intended by the review. A real aggro (chase for N seconds) is a later change in `EnemyBrain` only |

---

## 13. The 5‑minute `ArenaTimer` (separate small plan)

Developer review (Q4): the timer is **not** built in E0–E5. It gets its own small plan. This section records where
it stands today and what it needs, so that plan can start from here.

### 13.1 Current state (2026-11-21)

- **No timer code exists.** `src/` has no `ArenaTimer`, no `TimerExpiredEvent`, no countdown on the HUD and no
  `timer_*` commands.
- It exists only on paper: the base plan's target game ("a 5‑minute timer starts … win by killing every wave or by
  surviving the whole 5 minutes") and its hand‑off table (base plan §9: `ArenaTimer` in `game/flow/`, ticked in
  `ArenaGame::FixedUpdate` while Playing, `TimerExpiredEvent`, `timer_set` / `timer_pause`).
- Win and lose are already single‑shot: `GameState::WinGame` / `LoseGame` only work from Playing and return false
  otherwise. Whichever comes first (last wave cleared, timer expired, player died) decides the run, with no extra
  code.

### 13.2 What this plan leaves ready for it

| Piece | Where (this plan) | Used by the timer for |
|---|---|---|
| `ArenaGame::FixedUpdate` with a "GameState is Playing" block | §3.3, E1 | Ticking it: frozen in menus, pause, the editor and an open console, because the engine only runs fixed steps while simulating |
| `WavesClearedEvent` → `WinGame` | §6.2, E5 | The other way to win; the timer adds a second path |
| `PlayerDiedEvent` → `LoseGame` | §9.2, E4 | Dying before the end still loses |
| `OnLevelLoaded` reset of enemies and waves | §3.2 | The same place resets the timer (restart, Retry) |
| HUD block (health, wave counter) | `GunHUD`, E4 / E5 | The `mm:ss` readout goes next to the wave counter |

### 13.3 What the timer needs (scope of its own plan)

| Item | Notes |
|---|---|
| `ArenaTimer` (`game/flow/`) | `Reset(duration)`, `FixedTick(dt)`, `GetRemaining()`, `IsExpired()`, `SetPaused(bool)`. Plain data, no actor. Counts fixed steps (`dt` = 0.02 s), so it is frame‑rate independent |
| `TimerExpiredEvent` | Published once when it reaches 0. `ArenaGame`: `GameState::WinGame()` if the player is alive (the "survive" win). `WaveDirector` stops spawning and enemies can be left as they are, because the game is no longer Playing |
| Duration | 300 s constant in v1. Later per level: an `ArenaRules` component on a level actor (`duration=300`) |
| HUD | `mm:ss`; turns red in the last 30 s; optional tick sound in the last 10 s (`SoundFXBank`) |
| Win dialog text | `GameUI` tells "Arena cleared" (waves) and "Survived" (timer) apart. That needs a reason in `GameWonEvent` or a separate event |
| Console | `timer_set <s>`, `timer_pause 0\|1`, `timer_expire` (tests the survive path at once) |
| Design question for that plan | Does the timer keep running between waves (in the 2 s `Waiting` state)? Default: yes, it is a run clock |

Estimate: one phase, about 4 commits (timer + event, HUD, dialog text, console).

