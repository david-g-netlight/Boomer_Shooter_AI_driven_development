# Devlog — `scene`

**Phase:** 5 — Scene: GameActor, Component, Level ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `688cb54`
**Date:** 2026-11-06 → 2026-11-07
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/scene`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `9d18610` | `GameActor` | `src/engine/level/GameActor.h/.cpp`. Name, raylib `::Transform` (translation, quaternion, scale), `SetPosition / SetRotation / SetRotationEuler / SetScale`, `GetLocalMatrix` (S·R·T) / `GetWorldMatrix` (local · parent world, computed on demand) / `GetWorldPosition`. Parent/children (non‑owning): `SetParent(parent, keepWorldTransform)` refuses self and descendants (cycles); keep‑world uses `MatrixDecompose`. `IsActive / IsActiveInHierarchy / SetActive`. `MarkForDestroy` is recursive and never deletes. Protected ctor + `friend class Level` (as the reference); non‑copyable; the destructor detaches from parent and children. |
| 2 | `9107f63` | `Component` + `AddComponent / GetComponent` | `src/engine/level/Component.h/.cpp`: `OnCreate, OnEnable, OnDisable, OnDestroy, Update, FixedUpdate, SubmitRenderables`, pure `TypeName`, `GetOwner` (nullptr when standalone), `SetEnabled` (hooks only on change). `GameActor::AddComponent<T>(args...)` (`requires derived_from<T, Component>`) → owner set, `OnCreate`, `OnEnable`; `GetComponent<T>` (`dynamic_cast`, finds derived types). Actors own components (`unique_ptr`); destroyed in reverse order (`OnDisable` if enabled, then `OnDestroy`). Private `Update / FixedUpdate / SubmitRenderables` for the Level, iterating by index over a snapshot of the count, stopping when the actor gets marked; then the actor's virtual `OnUpdate / OnFixedUpdate`. |
| 3 | `1cf537f` | `Level` | `src/engine/level/Level.h/.cpp`. `CreateGameActor(name, parent)` (private `PlainActor`) and `CreateGameActor<T>` (as the reference) → `vector<unique_ptr<GameActor>>`. `SetParent` refuses actors of other levels. `Update / FixedUpdate / SubmitRenderables` skip marked and inactive‑in‑hierarchy actors; by index over a snapshot (actors created during the loop start next frame). `FlushDestroyed()`: unlink every marked actor from parents/children first, then erase (components get their hooks). `FindByName`, `GetActors`, `GetActorCount`, `Clear`. Engine: `level->FixedUpdate` before `app->FixedUpdate`, `level->Update` before `app->Update`, `level->SubmitRenderables` before `app->SubmitRenderables` (re‑queried after Update in case the game switched levels), `FlushDestroyed()` at the end of the frame. |
| 4 | `0737ef5` | `MeshRenderer` | `src/engine/render/MeshRenderer.h/.cpp`: mesh or model (not owned), shared material or `CreateOwnMaterial(shader)` (owned, per actor); lazy default material for a bare mesh; `SetVisible`; `GetLocalBounds` (cached) / `GetWorldBounds` (8 transformed corners); submits with the owner's world matrix (`SubmitModel` for models). Game components `src/game/components/Rotator.h/.cpp` (spin around a local axis) and `SelfDestructTimer.h` (marks its owner from its own Update). `ScaleGame` rewritten around `eng::Level`: Ground; `Ring` (Rotator 15°/s) → 10 cube children (own material, lit/unlit alternating) → Cube 0 (Rotator 90°/s) → `Moon`. **X**: a random cube gets a `SelfDestructTimer(0)`; **R**: ring marked + rebuilt in the same frame; cubes pruned from the game's list once marked; scene panel; debug world‑bounds boxes and labels follow the cubes. |
| 5 | `b5673d1` | `AudioSource` → `Component` | `AudioSource : Component`; `Update(dt)` copies the owner's world position and refreshes gain/pan; `Play()` also syncs first; `OnDisable / OnDestroy` stop the voice. Old `Update()` renamed `RefreshSpatial()` for standalone use. Each cube now has an `AudioSource` component; the game's parallel source list and manual position sync were removed. |
| 6 | *(this commit)* | — | This devlog; tracker marks Phase 5 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P5‑1 | Reuse raylib's `::Transform` for actors | Same fields as the reference's `Transform`; no conversion when handing data to raylib/Bullet |
| P5‑2 | World matrix computed on demand, no cache | Scenes are small; a dirty‑flag cache can be added later without API changes |
| P5‑3 | Protected `GameActor` ctor + `friend class Level`; plain actors via a private `Level::PlainActor` | Keeps the reference's "actors are created by a Level" rule while allowing plain actors with components |
| P5‑4 | Template‑method update: the Level calls private non‑virtual `Update`, which runs components then virtual `OnUpdate` | Derived actors can't forget to update their components (the reference's single virtual `Update` relied on calling the base) |
| P5‑5 | Index loops over a snapshot of the count (actors and components) | Creating actors or components during an update can reallocate the vectors without invalidating the loop; new ones start next frame |
| P5‑6 | Marked actors stop updating immediately (even later components of the same actor), deleted only in `FlushDestroyed` | Destroy is safe from anywhere; nothing touches freed memory during the frame |
| P5‑7 | `FlushDestroyed` unlinks all hierarchy pointers before deleting | Parent and child may be deleted in any order without dangling pointers |
| P5‑8 | Inactive actors skip update/render; components get `OnDisable` only from `SetEnabled` or destruction | Simple first version; hierarchy‑wide enable events can be added if gameplay needs them |
| P5‑9 | `MeshRenderer` can own a per‑actor `Material` | Level files (Phase 9) set a tint per actor without the game managing material lists |
| P5‑10 | `MeshRenderer`'s lazy default material is `mutable` | Created on first draw (needs the window) from a `const` submit method |
| P5‑11 | `AudioSource` kept usable standalone (`RefreshSpatial`) | UI or global sounds don't need an actor |
| P5‑12 | Demo helpers live in `src/game/components/` (`Rotator`, `SelfDestructTimer`) | `Rotator` is reusable for pickups/decoration; `SelfDestructTimer` is demo‑only and can be removed with the demo |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| GameActor headless test (12 checks): translation / yaw +90 / scale 2 / grandchild world positions; self and cycle refused; keep‑world reparent; recursive mark; inactive hierarchy; delete detaches | ✅ |
| Component lifecycle headless test (6 checks): `OnCreate→OnEnable` order, `GetComponent` incl. derived, owner, `SetEnabled` only on change, reverse destruction with disabled skip | ✅ |
| Level headless test **under AddressSanitizer** (11 checks): self‑destruct mid‑update, destroying an actor that hasn't updated yet, parent subtree, child only, spawn during update (next frame), `FindByName`, cross‑level `SetParent` refused, `Clear` | ✅ no ASan errors |
| Whole game built with ASan (scratch build), self‑closing run: Cube 0 (+ Moon) and Cube 5 self‑destruct; ring rebuilt in the same frame | ✅ 13 → 10 → 13 actors, Moon gone with its parent, no ASan errors, clean exit |
| ASan run: cube destroyed while its ping plays | ✅ voice stopped (2 → 1 active), other cube's source position == actor world position |
| Screenshot | ✅ ring #2, `draws 12` (ground + 10 cubes + moon), Moon orbiting Cube 0, labels and bounds follow |
| Children follow a rotating parent; destroying an actor mid‑update is safe | ✅ validated by the developer |

## Notes for later phases
- Phase 6 (Physics): `Collider` / `RigidBody` / `TriggerVolume` / `CharacterMotor` are Components; register with `PhysicsWorld` in `OnEnable`, unregister in `OnDisable` (also runs on destroy), so `FlushDestroyed` removes bodies before deleting them. `ActorMotionState` writes the actor's world pose back; actors with physics bodies should be roots (or the motion state converts world → local).
- Phase 9 (Editor): `Level::GetActors()` feeds the hierarchy panel; `MeshRenderer::GetWorldBounds()` is the picking fallback; components need `DrawInspector()` / serialization hooks then.
- Phase 10: `PlayerActor` can derive from `GameActor` (`CreateGameActor<PlayerActor>`) and use `OnUpdate`.
