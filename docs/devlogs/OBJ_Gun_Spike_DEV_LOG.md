# Development Log — OBJ Gun Spike

Tracker: [`tasks/doing/OBJ_Gun_Spike_DEV_PROGRESS.md`](../tasks/doing/OBJ_Gun_Spike_DEV_PROGRESS.md). Plan: [`deliverables/Rendering_OBJ_Gun_Spike_PLAN.md`](../deliverables/Rendering_OBJ_Gun_Spike_PLAN.md).
Assessment it builds on: [`deliverables/Custom_rendering_capabilities_workload_assesment.md`](../deliverables/Custom_rendering_capabilities_workload_assesment.md) (gaps G1–G8).

## Decision log

| # | Date | Decision | Plan section |
|---|---|---|---|
| S‑D1 | 2026-11-24 | Spike branch `spike/obj-gun-rendering` from `integration`; phase branches from it and merged back into it; never merged into `integration` | §0, §8 |
| S‑D2 | 2026-11-24 | OBJ + MTL + PNG through raylib's built‑in loader and `ResourceManager::LoadModel`; no new library | §0 |
| S‑D3 | 2026-11-24 | Test asset from the developer: `assets/models/prototype/gun/gun.obj`, `gun.mtl`, `gun.png` | §0, §1 |
| S‑D4 | 2026-11-24 | `LoadModel` gives the model's materials the default lit shader (G1) | §0 |
| S‑D5 | 2026-11-24 | Gun pass clears depth (`eng::ClearDepthBuffer()`) and draws with depth test on (G2) | §0, §3 |
| S‑D6 | 2026-11-24 | `GunView` owns a `RenderQueue` and draws it with the gun camera | §0 |
| S‑D7 | 2026-11-24 | Sun direction rotated into view space for the gun | §0 |
| S‑D8 | 2026-11-24 | Model recentered on its bounds and scaled to `GUN_LENGTH` 0.48 m along −Z | §0, §2 |
| S‑D9 | 2026-11-24 | Muzzle tip and ready light stay primitives drawn over the model | §0, §2 |
| S‑D10 | 2026-11-24 | `LoadTexture(path, filter)`, point by default, mipmaps only for trilinear; same filter on model textures (G4) | §0 |
| S‑D11 | 2026-11-24 | `gun_obj 0|1` toggle; primitive gun + warning if the OBJ fails to load | §0 |
| S‑D12 | 2026-11-24 | Spike‑only tuning: `gun_ofs`, `gun_rot`, `gun_len`, `gun_reload` | §0 |
| S‑D13 | 2026-11-24 | `MeshRenderer` `texture=` / `uv=` keys, edited with `set`; there is no inspector panel (correction to the assessment's G6) | §0 |
| S‑D14 | 2026-11-24 | `worldUV` material parameter: box‑projected world UVs in `lit.fs` (G8) | §0 |
| S‑D15 | 2026-11-24 | `alphaCutoff` material parameter; the gun drawn with an `eng::Material` override made from the model's texture | §0 |
| S‑D16 | 2026-11-24 | PSX shader effects out of scope | §0 |
| S‑D17 | 2026-11-24 | Added while writing the tracker: `RenderQueue` resets material parameters before raw‑material draws, so models do not inherit the last material's `emissive` / `alphaCutoff` / `worldUV` | §0, §3 |
| — | 2026-11-24 | Plan phases renamed ~~P0–P5~~ **V0–V5**: `P` (core devlog ids) and `G` (the assessment's gaps) were already used | §4 |

## Questions for the developer

| # | Question | Answer |
|---|---|---|
| T11‑1 | ~~Will you provide the gun asset, or should the plan make a placeholder?~~ | **Answered:** the developer provides `.obj`, `.mtl` and `.png` |
| T11‑2 | ~~Include the PSX shader effects (vertex snapping, affine textures)?~~ | **Answered:** no |

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| R1 `map_Kd` absolute path or PNG not next to the MTL | V0, V2 | open | `.mtl` check in V0; texture load line in the V2 log |
| R2 V coordinate flipped between Blockbench and raylib's OBJ loader | V2 | open | Marked‑pixel check; flip once at load or in the export |
| R3 Inverted winding or missing normals | V2 | open | Disable culling temporarily to diagnose |
| R4 Unknown Blockbench units / axes | V2 | open | Bounds‑based scale and recenter, `gun_rot`, bounds in the log |
| R5 Narrow view‑model FOV / near plane distorts or cuts the model | V2, V3 | open | `gun_len`, `gun_ofs`; FOV ConVar if needed |
| R6 raylib OBJ loader splits meshes per material | V2 | accepted | One texture per gun |
| R7 Point filter without mipmaps shimmers on far level surfaces | V4, V5 | accepted | PSX look; trilinear per texture if needed |
| R8 `glClear` include is macOS‑specific | V1 | accepted | `#ifdef __APPLE__`; the engine is Mac‑only today |
| R9 Spike code leaks into `integration` | all | open | No merge into `integration`; port list in plan §8 |

## Session log

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-24 | V3 | Developer validated V3 ("pass, continue with V4"); screenshot [`obj_gun_spike/rendering_spike.png`](obj_gun_spike/rendering_spike.png) (one, not three); **decision: Go** (inferred from the developer moving on to V4; to correct if wrong). Merged into `spike/obj-gun-rendering`, branch renamed `done/gun_spike_tune` | Start V4 | — |
| 2026-11-24 | V3 | Developer's tuned values committed as `GunView::Tuning` defaults (`gun_ofs 0.05 -0.05 0.2`, `gun_rot 0 2 5`, `gun_len 0.6`, `gun_muzzle 0 0.05 -0.1`) and the developer's new gun.obj / .mtl / .png. Found that `gun.obj` was never committed since V0 (`*.obj` in `.gitignore`): fixed. Debug + Release incremental builds pass. **Not run:** the game, clean builds | Developer adds the 3 screenshots (idle, firing, bright wall) and the decision (Go / Iterate art / No-go) | — |
| 2026-11-24 | V2 | Developer validated V2 ("works"); merged into `spike/obj-gun-rendering`, branch renamed `done/gun_spike_obj_gun` | Start V3 | — |
| 2026-11-24 | V2 | 6 checkboxes on `gun_spike_obj_gun`: lit shader + point filter + bounds log in `LoadModel`; model texture unload; `RenderQueue` reset before raw-material draws; `alphaCutoff` in `lit.fs`; `GunView` draws the OBJ (fallback to primitives); `ReloadModel` and the `gun_*` commands. Clean Debug + Release builds, no warnings. Run with the Debug app: log shows the model, bounds `(-0.0056 -0.0100 -0.0249)..(0.0056 0.0100 0.0249)` and the 32x32 texture with transparent texels (alphaCutoff on); with `gun.obj` removed from the build folder the game started, logged the warning and kept running. **Not run:** anything visual (silhouette, texture orientation, lighting, holes, tip placement), the `gun_*` commands, quit-time texture unload log, FPS | Developer runs the V2 **Done when** check | The ready light and the tip flash positions on the OBJ are guesses |
| 2026-11-24 | V1 | Developer validated V1; merged into `spike/obj-gun-rendering`, branch renamed `done/gun_spike_depth_pass` | Start V2 | — |
| 2026-11-24 | V1 | 3 checkboxes on `gun_spike_depth_pass`: `ClearDepthBuffer`; pose matrix + depth clear in `GunView::Draw`; primitive parts as lit materials in the gun's own `RenderQueue` with a view-space sun. Debug + Release (incremental and clean `check-*`) build with no warnings; the Debug app started 6 s with no warnings in the log. **Not run:** anything visual (size / position vs `integration`, shading, wall clipping, kick / bob / flash / light) | Developer runs the V1 **Done when** check | — |
| 2026-11-24 | V0 | Developer validated V0; merged into `spike/obj-gun-rendering`, branch renamed `done/gun_spike_folders_filter` | Start V1 | — |
| 2026-11-24 | V0 | 3 checkboxes on `gun_spike_folders_filter`: README + CREDITS + `assets/textures/`; `LoadTexture(path, filter = POINT)`; gun files moved to `prototype/gun/`. Debug + Release builds (incremental and clean `check-*`) pass with no warnings; the three files are in `build/check-*/assets/models/prototype/gun/`; `map_Kd gun.png` is bare. **Not run:** the app itself (no code path uses the new filter yet). | Developer runs the V0 **Done when** check | — |
| 2026-11-24 | — | Tracker written for `Rendering_OBJ_Gun_Spike_PLAN.md` | Developer reviews, gives the go for V0 | — |
