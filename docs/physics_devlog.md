# Devlog — `physics`

**Phase:** 6 — Physics (Bullet 3.25) ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `9294f5f`
**Date:** 2026-11-07
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/physics`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `3b562db` | `BulletConvert` | `src/engine/physics/BulletConvert.h` (engine‑internal, includes `<cfloat>` first because `btScalar.h` uses `FLT_EPSILON` without it): `ToBt / ToRaylib` for `Vector3`, `Quaternion` (both x,y,z,w), `::Transform` (rotation + translation; scale lives on shapes), `ToRaylibMatrix` (both column‑major in memory). |
| 2 | `ec2ecd7` | `PhysicsWorld` | Pimpl (`PhysicsWorld.h` has no Bullet include). `Impl` (`PhysicsWorldImpl.h`): collision config → dispatcher → `btDbvtBroadphase` → sequential‑impulse solver → `btDiscreteDynamicsWorld`, declared in creation order so destruction is reversed. `Init(gravity)`, `Step(dt)` = `stepSimulation(dt, 1, dt)`, `Shutdown()` detaches leftover objects with a warning, `SetGravity`, `GetObjectCount`. Engine: init after audio, one `Step` per fixed tick after level/app `FixedUpdate`, shutdown after the game's `Destroy()` (level cleared) and before resources. |
| 3 | `8e5fe68` | `PhysicsLayers` | `Layer::{Ground, Prop, Player, Trigger, Beam, All}` bit flags, `DefaultCollisionMask(layer)`, `LayerName`, `static_assert(AreDefaultMasksSymmetric())`. |
| 4 | `69c3f1c` | `Collider` | Component, Bullet shape behind a pimpl, one shape per actor. `SetBox / SetSphere / SetCapsule / SetConvexHull(mesh) / SetStaticMesh(mesh)`; `SetCenter` (wraps in `btCompoundShape` so the offset scales), `SetBottomPivot` (center = half height: grows upward); `SetScale / SyncScaleFromActor` (world matrix column lengths); `SetLayer` (resets mask) / `SetMask`; `GetWorldBounds`; `GetNativeShape` (lazy rebuild). Convex hulls use a 1 cm margin (P6‑5). |
| 5 | `00d2fb9` | `ActorMotionState` | `btMotionState`: `getWorldTransform` = actor world pose without scale; `setWorldTransform` writes back, converting into the parent's local space; never touches scale; `writeRotation=false` keeps the actor's rotation (character). |
| 6 | `1a81aef` | `RigidBody` | Component: `RigidBodySettings{type, mass, friction, restitution, damping, lockRotation, ccd}`; body created in `OnEnable` (needs the Collider first), removed + deleted in `OnDisable`; user pointer = `Collider*`; kinematic = `CF_KINEMATIC_OBJECT` + `DISABLE_DEACTIVATION`; `AddImpulse / AddForce / Set/GetLinear/AngularVelocity / Teleport / OnScaleChanged / IsSleeping / WakeUp`; type change rebuilds. `PhysicsWorld::GetNativeWorld()` is private (friends only). **Fixes in this commit:** `eng::DecomposeTransform` (`core/MathUtils.h`) replaces raymath's `MatrixDecompose` engine‑wide (P6‑1); locked rotation ignores `SetAngularVelocity` (P6‑4). `ScaleGame` rewritten as the physics sandbox (ring demo retired). |
| 7 | `e250457` | `Raycast` | `RaycastHit{actor, collider, point, normal, distance}`; `PhysicsWorld::Raycast(origin, dir, maxDist, mask)` with `ClosestRayResultCallback`, ray group = all bits so only `mask` decides (P6‑6). Sandbox: crosshair (red over a crate), center ray with the gun's `Ground | Prop` mask and 30 m reach, **LMB** kicks the aimed crate at the hit point, **X** deletes it, debug hit point + normal. |
| 8 | `b35a8f3` | `TriggerVolume` | `Component::OnTriggerEnter / OnTriggerExit(GameActor& other)`. `TriggerVolume` component: `btPairCachingGhostObject` (`CF_NO_CONTACT_RESPONSE`) from the actor's Collider; `btGhostPairCallback` installed on the broadphase. `PhysicsWorld::Step`: `SyncTriggers` (ghost poses from actors) → step → `DispatchTriggers` (candidates from the ghost's pair list confirmed with `contactPairTest`, diffed against the previous set; both actors' components notified). `OnColliderRemoved` purges a removed body from every trigger (no Exit). `GameActor::NotifyTriggerEnter/Exit` (friend `PhysicsWorld`). Sandbox: translucent `Trigger Zone` + `TriggerCounter` component (green while occupied). |
| 9 | `4f457b0` | `CharacterMotor` | Component using the actor's capsule Collider (layer Player): dynamic body, mass 80, friction 0, angular factor 0, `DISABLE_DEACTIVATION`, CCD, `BT_DISABLE_WORLD_GRAVITY` + own gravity 32; motion state without rotation write‑back. `FixedUpdate`: sphere sweep (0.95 r) from the bottom sphere down `groundProbe`; grounded if slope ≤ 45° (stores normal + ground body velocity), "steep" otherwise; grounded → walk velocity projected on the ground plane at full speed + ground velocity + glue −0.05, jump latch → vy = v0 + 0.1 s grace; airborne → air control 0.5 per 0.1 s; vertical clamp 15 m/s; **gravity along −normal while grounded** (P6‑7); **steep slope removes input into it** (P6‑8). `SetWalkVelocity`, `Jump`, `SetGravity`, `Warp` (clears velocity, walk input, pending jump), queries. Sandbox: **V** toggles a first‑person capsule (walk 6 m/s, jump 8 m/s, eye 0.7 m above center). |
| 10 | `2b04c05` | `PhysicsDebugDrawer` + F3 | `btIDebugDraw` → `DrawLine3D`; contact points as short yellow normals; Bullet warnings → `Log`. `PhysicsWorld::SetDebugDrawEnabled / IsDebugDrawEnabled / DebugDraw`; Engine draws it in the 3D pass. **F3** in the sandbox. |
| 11 | `364ef6f` | CCD | `CcdMode {Auto, On, Off}` in `RigidBodySettings` (`SetCcdMode`, `IsCcdActive`); Auto = dynamic and smallest scaled dimension < `CCD_AUTO_SIZE` 0.3 m; threshold ½ min size, swept sphere 0.4 × min size; recomputed in `OnScaleChanged`. Sandbox: 5 cm `Thin Wall`, **T** throws 0.12 m pellets at 40 m/s (max 30), tunnelled counter. |
| 12 | *(this commit)* | — | This devlog; tracker marks Phase 6 done; `ActorHandle` scheduled in Phase 10 (D11). |

---

## Decisions and findings

| # | Decision / finding | Why |
|---|---|---|
| P6‑1 | **raymath's `MatrixDecompose` is wrong for our matrices** (reads rows `{m0, m4, m8}`); replaced by `eng::DecomposeTransform` (columns `{m0, m1, m2}`) in `GameActor::SetParent(keepWorld)`, `Collider::GetWorldBounds`, `ActorMotionState` | Found when the sandbox ramps were flat in physics but tilted on screen: for a 6 × 0.4 × 4 box rotated 30°, raymath returned scale (5.20, 0.46, 4.00) and a rotation 27.8° off. Uniform scale hides it (why earlier tests passed); Phase 5 `keepWorld` reparenting had the same latent bug |
| P6‑2 | Physics components register in `OnEnable`, unregister in `OnDisable` (also on destroy) | `Level::FlushDestroyed` removes bodies before deleting them; re‑enabling picks up shape changes |
| P6‑3 | `PhysicsWorld::GetNativeWorld()` is private, used by friend components | Bullet types stay out of the public API; no free helper can reach `Impl` |
| P6‑4 | Locked rotation ignores `SetAngularVelocity` and zeroes spin when locking | Bullet's angular factor only scales impulses/torques; a direct velocity still rotated the "locked" body (failed test) |
| P6‑5 | Convex hull margin 1 cm (default 4 cm) | Hull margins sit outside the points (box margins inside): props collided visibly fat |
| P6‑6 | Rays use an all‑bits group; the caller's mask alone decides | Otherwise a ray could only hit objects whose own mask includes the ray's layer (a `Player` ray couldn't hit the player) |
| P6‑7 | Character gravity points along −ground normal while grounded | With friction 0, full downward gravity made the capsule slide 0.59 m in 2 s standing on 30°; now 0.05 m |
| P6‑8 | On slopes steeper than 45° the input component into the slope is removed | Frictionless contact turns horizontal push into upward speed: the capsule could creep up 50° |
| P6‑9 | `CharacterMotor` uses a Collider component like `RigidBody` | Rays, triggers and the editor see the player like any other collider |
| P6‑10 | Trigger overlaps confirmed with `contactPairTest` | The ghost's pair list is AABB‑only; rotated/odd shapes would report false overlaps |
| P6‑11 | Bodies destroyed inside a trigger are dropped silently (no Exit) | Matches Unity; avoids calling into an actor that is being deleted |
| P6‑12 | F2 / F3 are bound by the game, not hard‑coded in the engine | Same as other debug keys; the engine stays input‑agnostic |
| P6‑13 | Jump apex is 0.91 m instead of the analytic 1.0 m | The step applies one tick of gravity on the jump frame (semi‑implicit Euler); to tune in Phase 10 (e.g. v0 + g·dt/2) |

---

## Verification

All tests are scratch files, not committed. Most were compiled with every engine source under **AddressSanitizer**.

| Test | Result |
|---|---|
| BulletConvert: Vector3 / Quaternion round trips, identical point rotation in both libraries, btTransform → Matrix == R·T | ✅ 5/5 |
| Collider bounds: box, bottom pivot (bottom stays on y=0 when scaled ×2), sphere, capsule (2 m), layer/mask, convex hull (after margin fix), static mesh | ✅ 9/9 |
| ActorMotionState: root read/write, child of a translated + rotated + scaled parent, scale untouched, `writeRotation=false`, **non‑uniform ramp** | ✅ 10/10 |
| RigidBody (ASan): falls, rests at y 0.500, sleeps, Teleport, kinematic follows actor, rotation lock (direct spin + off‑center impulse), delete mid‑sim removes the body, Clear | ✅ 10/10 |
| Raycast: closest hit 9.000 m with up normal, ground‑only mask, max distance, miss, gun mask skips the player, Player mask hits it | ✅ 7/7 |
| TriggerVolume (ASan): fall‑through 1 Enter + 1 Exit on both sides, resting inside, destroyed inside (silent), moving trigger onto a sleeping crate, re‑enable | ✅ 9/9 |
| CharacterMotor (ASan): flat ground y 0.99; climbs 30° (slope reads 30.0°) and stops (0.05 m in 2 s); 50° max +0.12 m in 4 s; jump apex 0.91 m and lands on the 0.8 m crate (crate moved 0.01 m); pushes the 2 kg crate 7.65 m; Warp + trigger detection; rotation untouched | ✅ 10/10 (after the P6‑7 fix and two test‑script fixes) |
| CCD: pellet 0.1 m at 60 m/s vs 5 cm wall: tunnels with CCD off (z −23.4), stopped with Auto (z 0.10); off for 1 m crate, on after shrinking to 0.2 m | ✅ 4/4 |
| Sandbox, ASan game build: 6 s → 20/20 crates asleep, 50 fixed steps/s; delete mid‑fall and aim‑delete (23 → 22 → 21 bodies); 15 pellets → 0 tunnelled; 59 FPS (ASan) with 41 bodies; F3 wireframes | ✅ |
| Done when (sandbox, ramps, jump, push, ray, trigger, F3, delete) | ✅ validated by the developer |

**ASan finding (game side, not engine):** a test that deleted a crate *after* the game had pruned its own `GameActor*` list that frame caused a use‑after‑free in the game's list. The real demo deletes before pruning, so it is safe, but the pattern is fragile → `ActorHandle` scheduled for Phase 10 (D11).

## Notes for later phases
- Phase 7 (FX): beams and impact particles can use `RaycastHit::point/normal`.
- Phase 8: the `ModeMachine` must skip `AdvanceAccumulator()` while paused, so physics freezes with the game.
- Phase 9 (Editor): `RigidBody::Teleport` / `CharacterMotor::Warp` for gizmo moves; `Collider::GetWorldBounds` + `PhysicsWorld::Raycast` for picking; F3 wireframes as the collider overlay.
- Phase 10: `ActorHandle` first; `FirstPersonController` drives `CharacterMotor` (`SetWalkVelocity`, `Jump`); tune jump v0 (P6‑13); Unity walk speed is 10 m/s.
- Phase 11: `Scalable` → actor scale + `RigidBody::OnScaleChanged()` (CCD re‑evaluated automatically); props use `Collider::SetBottomPivot(true)`.
