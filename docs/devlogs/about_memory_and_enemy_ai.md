## 10. Answers to the developer's questions

### 10.1 Should the AI system be an engine‑level system?

**Not now. Keep the AI in `game/enemies/ai/`, and build it so the parts that will become engine‑level are already
separate.**

- **What is in v1 is game rules, not engine.** "Idle for 5 s", "chase inside the trigger radius", and "attack means
  bite" are this game's design, just like `GameState`'s win and lose rules (which the base plan kept out of the
  engine's `ModeMachine` for the same reason). A generic engine FSM (state classes, enter/exit, transition tables)
  would be more code than the 5‑state switch it replaces, and it would still need game code for every state.
- **What should become engine‑level is the world queries, not the decisions.** Pathfinding is a question about
  level geometry ("how do I get from A to B?"), the same kind of question as `PhysicsWorld::Raycast`. When it comes,
  it goes in the engine (`engine/nav/`: a nav grid baked from the level's `Ground` colliders, A*, path smoothing),
  with a query API like `nav.FindPath(from, to, agentRadius, outPath)`. It must not know about enemies, ECS or
  states, so an NPC built from `GameActor`s can use it too.
- **The seam is already in v1:** `EnemyBrain` decides the state, and `ChaseSystem` turns the state into a direction.
  With pathfinding, only `ChaseSystem` changes: "toward the player" becomes "toward the next waypoint of a path to
  the player" (with a `PathFollow` component holding the path index). The state machine and the other systems stay.
- **When to promote more:** if NPCs need the same idle / wander / react pattern, move only the reusable pieces into
  the engine:
  - a ~40‑line `eng::ai::StateTimer<Enum>` helper (state, stateTime, change with enter callback);
  - steering helpers (`Seek`, `Separate`, `TurnToward`) in `MathUtils` or `engine/ai/Steering.h`, as pure functions.

  Behaviours and thresholds stay in the game.

Proposed later layout: `engine/nav/` (pathfinding), `engine/ai/` (steering and the state timer, pure functions, no
ECS dependency), `game/enemies/ai/` (the brains).

### 10.2 Should the small ECS be an engine‑level system?

**Yes, but only the storage core (`engine/ecs/`: `Entity`, `ComponentPool`, `Registry`). Components and systems
stay with the feature that owns them.**

- **Why engine‑level:** it has no game knowledge at all (ids and packed arrays), you plan to reuse it for NPCs and
  effects, and the engine's rule is "the game may include the engine, never the other way round". If it starts in
  `game/`, the first engine user (for example an effects system) would force a move anyway.
- **Why only storage:** an engine ECS with built‑in systems (a render system, a transform hierarchy, a scheduler)
  would compete with the `GameActor` model the editor, level files and Bullet are built on. v1 keeps a clear split:
  - **`GameActor`** = authored, few, heterogeneous: level geometry, triggers, spawn points, the player. It is saved,
    edited and physical (Bullet).
  - **`ecs::Registry`** = runtime, many, homogeneous: enemies now, then projectiles, NPC crowds, gibs and decals,
    pickups. It is simulated by data‑oriented systems and never saved.
- **Rules that keep it small:** std only (rule 1); no virtuals on the iteration path; capacity fixed per registry
  (each feature owns its own `Registry`, so enemies never share a capacity budget with effects); no system code in
  `engine/ecs/`.
- **What could move into the engine later, once two features need it:** an `ecs::Transform` component and a
  `MeshRenderSystem` that submits to the `RenderQueue` (with instancing). Enemies and effects would then share one
  render path. A bridge for entities to be found by `PhysicsWorld` queries (Bullet ghost objects with an `Entity`
  user pointer) if enemies ever need real physics (ragdoll‑free knockback, explosions pushing them).
- **What it is not for:** replacing `GameActor` or adding entities to the editor or level files. That would be a
  large change (serialization, picking, gizmos for entities) and should be its own plan if it is ever wanted.
