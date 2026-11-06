# Devlog — `fx`

**Phase:** 7 — FX: particles and lines ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `3cfcf54`
**Date:** 2026-11-07
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/fx`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `d57624f` | `ParticleEmitter` + pooled `ParticleSystem` | `src/engine/fx/ParticleSystem.h/.cpp`, owned by the Engine (`GetParticles()`): fixed pool of `MAX_PARTICLES` 1024, alive particles packed at the front (swap‑remove on expiry), full pool drops new particles and counts them. `ParticleEmitterSettings`: burst count, lifetime ± variation, speed ± variation, spread cone (uniform over the spherical cap), start/end size, gravity, exponential drag, start/end color (alpha included), additive flag. `Emit(settings, position, direction)`, `Update(dt)` (game time), `Draw(camera)`: billboards with raylib's 1×1 white texture, alpha pass then additive pass, depth test without depth write; fixed RNG seed. `src/engine/fx/ParticleEmitter.h/.cpp`: component that bursts at its actor along local −Z. Engine: update after the game, draw after the opaque queue. Sandbox: LMB / RMB impact bursts at the aimed hit point along the surface normal (stretch blue → green, shrink yellow → red, additive); particle count in the HUD. |
| 2 | `18a07a9` | `LineRenderer` | `LineCommand` + `RenderQueue::SubmitLine` + transparent line pass at the end of `Draw()` (alpha blend, no depth write, culling off): open tapered tube with 8 sides, colors interpolated per vertex; radius 0 = 1 px line. `src/engine/render/LineRenderer.h/.cpp` component: world‑space points, start/end color, start/end width, `FadeOut(seconds)` (alpha → 0, then hidden), `GetOpacity`. Sandbox: LMB / RMB fire a beam from the Unity `RayBeamProvider` muzzle offset (camera + right·0.3 − up·0.25 + forward·0.6) to the hit point, fading in 0.35 s, removed by a `SelfDestructTimer`; lines count in the HUD. |
| 3 | *(this commit)* | — | This devlog; tracker marks Phase 7 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P7‑1 | One engine‑owned fixed pool for all particles | No allocation per burst; bounded cost; bursts outlive the actors that emitted them |
| P7‑2 | Full pool drops new particles (counted) instead of overwriting old ones | Simple and predictable; 1024 is far above what the game needs |
| P7‑3 | Two‑key color gradient (start → end, alpha included) | Matches what the Unity gradients used (one color, alpha 1 → 0); more keys can be added if needed |
| P7‑4 | Transparent passes (particles, lines) test depth but don't write it | Overlapping transparent quads/tubes don't cut holes into each other |
| P7‑5 | Particles follow game time | Effects freeze with the game while paused (Phase 8) |
| P7‑6 | Lines go through the RenderQueue (`SubmitLine`) instead of drawing in components | Components submit before `BeginMode3D`; queued lines are also drawn by the minimap's second camera |
| P7‑7 | **Beams are open tubes with per‑vertex colors**, not raylib `DrawCylinderEx` segments | The first screenshot showed stacked hexagon caps through the transparency; the tube is one piece with a smooth gradient |
| P7‑8 | `ParticleEmitter` members are private with accessors | Same `m_` member style as the rest of the engine |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Particle pool (headless): burst of 50, expiry after 1 s, full pool 1024 alive + 226 dropped of 1250, `Clear`, emitter forward for a yaw +90 actor = −X | ✅ (the spread cone was checked visually, not numerically) |
| ASan screenshot run #1 | Bursts and beams visible; found the hexagon cap artifact (P7‑7) |
| ASan screenshot run #2 | ✅ smooth gradient beams, 80 particles + 2 beams on fire, no lines left 0.5 s later, no ASan errors |
| Click spawns a colored burst and a fading beam | ✅ validated by the developer |

## Notes for later phases
- Phase 8: `Update(dt)` already uses game time; when the ModeMachine skips the simulation, call `m_particles.Update` only in Play.
- Phase 11: `RayBeam` = `LineRenderer` whose start point moves toward the hit (Unity tail displacement 20 m/s) + `ParticleEmitter` at the muzzle (`MuzzleFX`) and at the impact. Particles are flat squares today; a soft round texture can be added with the gun polish.
