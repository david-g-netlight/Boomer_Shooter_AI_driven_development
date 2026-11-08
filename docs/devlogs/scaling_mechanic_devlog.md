# Devlog — `scaling_mechanic`

**Phase:** 11 — Game: scaling mechanic ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `4e40205`
**Date:** 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/scaling_mechanic`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `d6f80ed` | `ScalingGun` | Component on the player: ticks the cooldown and takes the buffered click each fixed step; the ray starts at the camera, goes along its forward, reach 30 m, mask `Ground \| Prop`. Publishes `ScalingShotEvent` (sign, factor, muzzle, hit point / normal, target handle) and calls `Scalable::TriggerScaling(1 + factor * sign)` on the target. |
| 2 | `5af2b99` | `Scalable` | Smooth change `start * SmoothStep(1, target, t)` at speed 5 (0.2 s); collider rescale, inertia and AABB through `RigidBody::OnScaleChanged`; mass by volume; the lowest collider point stays in place; stretch capped at 1.9× the base scale; destroyed when the largest axis drops below 0.1; pitched SFX when a change starts. Registered for level files and the editor's Add Cube (`Editor::SetPropDecorator`); `RigidBody::Teleport(keepVelocity)`; `SoundFXBank::Instance()`; the crates of `level_01.lvl` carry it. |
| 3 | `7dbea39` | `RayBeamSpawner`, `RayBeam`, `MuzzleFX` | `src/game/fx/`. The spawner listens to the shot event: muzzle burst, a `RayBeam` actor whose tail travels at 20 m/s and removes the beam on arrival, impact burst, beam sound (0.3; placeholder ping). Stretch blue to green, shrink yellow to red. The sandbox's throwaway beam code is gone. |
| 4 | `766e8c3` | Stretch blocked by static geometry | `PhysicsWorld::OverlapBox`; the target box (grown from the same footing, 8 cm inset) is tested against layer Ground and the stretch is refused on overlap. |
| 5 | *(this commit)* | — | This devlog; tracker marks Phase 11 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P11-1 | The stretch target is clamped so the ratio lands exactly on 1.9; the cap is relative to the scale before the first shot | The plan's "block at `scale.z >= 1.9`" lets the last stretch overshoot, and props are not all unit size |
| P11-2 | The base scale is re-captured when the actor's scale differs from what `Scalable` last wrote | An editor edit (gizmo, inspector) must not leave a stale base |
| P11-3 | Minimum-size destroy tests the largest axis, not the vector length | Same result for uniform scale; works for thin planks |
| P11-4 | The overlap test uses the collider's axis-aligned box with an 8 cm inset | Covers Bullet's 4 cm collision margin and a prop resting on the floor; may refuse early for rotated props (safe side) |
| P11-5 | The gun, spawner and props talk through `ScalingShotEvent` and the `Scalable` component, not through each other | New listeners (HUD, audio) need no change in the gun |
| P11-6 | `SoundFXBank::Instance()` instead of a pointer in every prop | Components created by the level loader have no constructor arguments |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Gun (headless on the real sandbox): hit at 13.5 m, second click blocked, ready after 1 s, miss ends at 30 m | ✅ |
| Scalable: 0.9 → 1.575 over 0.2 s, mass 2 → 10.7, clamp to 1.9, blocked at the cap, repeated shrinks delete the prop | ✅ |
| Beam (screenshot): beam, impact burst, muzzle particles; no beams a second later | ✅ |
| Wall check: refused 0.05 m from the thin wall, allowed 0.5 m away and in the open up to 1.9, shrinking beside the wall allowed | ✅ |
| LMB grows (capped), RMB shrinks until gone, colors, cooldown, scaled crates rest and fall | ✅ validated by the developer |

## Notes for later phases
- Phase 12: the level file needs `Scalable` crates; `level_01.lvl` is still the sandbox export, levels 2 and 3 are authored in the editor (Add Cube already adds `Scalable`).
- Phase 12: triggers (win / lose / next) are `TriggerVolume` + a game component registered in `Application::Init`.
- Phase 13: `GunHUD` can read `PlayerState::GetCooldownFraction()` and the factor; the gun view needs its own pass.
