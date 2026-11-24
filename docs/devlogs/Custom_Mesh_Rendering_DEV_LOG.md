# Development Log — Custom Mesh Rendering

Tracker: [`tasks/doing/Custom_Mesh_Rendering_DEV_PROGRESS.md`](../tasks/doing/Custom_Mesh_Rendering_DEV_PROGRESS.md). Plan: [`tasks/doing/Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md`](../tasks/doing/Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md).
Questions and answers of the plan: [`tasks/doing/Task_012_Q&A.md`](../tasks/doing/Task_012_Q&A.md). Spike it ports from: [`OBJ_Gun_Spike_DEV_LOG.md`](OBJ_Gun_Spike_DEV_LOG.md).

## Decision log

| # | Date | Decision | Plan section |
|---|---|---|---|
| M‑D1 | 2026-11-24 | Port from `spike/obj-gun-rendering`, never merge; finished engine files taken file by file, then cleaned | §0 |
| M‑D2 | 2026-11-24 | OBJ + MTL (+ one PNG) through raylib's loader and `ResourceManager::LoadModel` | §0 |
| M‑D3 | 2026-11-24 | Textured model: `eng::Material` override from its texture; color‑only (`Kd`) model: its own materials | §0, §3.2 |
| M‑D4 | 2026-11-24 | `LoadModel` gives every material the default lit shader | §0 |
| M‑D5 | 2026-11-24 | `LoadTexture(path, filter = POINT)`, mipmaps only for trilinear | §0 |
| M‑D6 | 2026-11-24 | `LoadModel` refuses OBJ faces over 5 vertices (tinyobj aborts on them); every caller has a fallback | §0, §3.1 |
| M‑D7 | 2026-11-24 | New `eng::FitModel`: recenter and scale a model from its bounds, used by the gun and the enemies | §0 |
| M‑D8 | 2026-11-24 | `RenderQueue` resets material parameters before raw‑material draws | §0 |
| M‑D9 | 2026-11-24 | Gun pass: own `RenderQueue`, depth cleared, depth test on, view‑space sun | §0, §3.3 |
| M‑D10 | 2026-11-24 | `MeshRenderer` `texture=` / `uv=`; `worldUV` box projection in `lit.fs` | §0 |
| M‑D11 | 2026-11-24 | Enemy hit test stays the analytic sphere; the model is fitted to it | §0 |
| M‑D12 | 2026-11-24 | Enemy fallback: sphere and pyramid. Models: Drifter `target-small`, Stalker `target-large` (Q15), Brute `target-detail` (Q5) | §0, §3.4 |
| M‑D13 | 2026-11-24 | ~~Gun tuning ConVars not ported~~ Revised by Q3: `gun_ofs`, `gun_rot`, `gun_len`, `gun_muzzle` and `gun_obj` ported, Debug only | §0 |
| M‑D14 | 2026-11-24 | Not ported: `gun_reload`, `ReloadModel`, `prototype/`, `_test` textures, `crate.png` | §0 |
| M‑D15 | 2026-11-24 | Per‑archetype `modelYawDeg` constant (the targets face ±X), no live ConVar (Q14) | §0, §3.4 |
| — | 2026-11-24 | Day estimates removed from the plan (developer's request) | §4 |
| — | 2026-11-24 | Found while writing the tracker: the enemy debug command is `ai_debug`, not `enemy_debug`; the M1 check loads the targets (committed in M0), not the spike's guns (only on the spike branch) | §4 |

## Questions for the developer

All answered in [`Task_012_Q&A.md`](../tasks/doing/Task_012_Q&A.md).

| # | Question | Answer |
|---|---|---|
| Q1 | ~~Was the spike's inferred V3 **Go** right?~~ | **Answered:** yes, but inferring a decision is not the standard |
| Q2 | ~~Which gun ships?~~ | **Answered:** the Kenney blaster, in `assets/models/gun/` |
| Q3 | ~~Keep live tuning to re‑check the gun?~~ | **Answered:** keep it |
| Q4 | ~~Keep `gun_obj` in Debug?~~ | **Answered:** yes, Debug only |
| Q5 | ~~Who makes the enemy models?~~ | **Answered:** the developer, in `assets/models/enemies`; `target-small` = Drifter, `target-detail` = Brute |
| Q6 | ~~How does a model fit its hit sphere?~~ | **Answered:** largest extent = diameter |
| Q7 | ~~White flash on color‑only enemies?~~ | **Answered:** fine |
| Q8 | ~~Which level textures?~~ | **Answered:** `wall.png`, `floor.png`, `ceiling.png` |
| Q9 | ~~Model hot reload?~~ | **Answered:** no, out of scope |
| Q10 | ~~`model=` key for level props?~~ | **Answered:** no, out of scope |
| Q11 | ~~Licenses?~~ | **Answered:** ship with `TODO: check`, confirm before a release |
| Q12 | ~~Phase order?~~ | **Answered:** M0 → M1 → M2 → M3 → M4 |
| Q13 | ~~`Textures/colormap.png` missing?~~ | **Answered:** added |
| Q14 | ~~Turn the targets with `modelYawDeg`?~~ | **Answered:** yes, a constant |
| Q15 | ~~`target-large` for the Stalker?~~ | **Answered:** yes |
| Q16 | ~~Commit the enemy files in M0?~~ | **Answered:** yes |

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| R1 Several `Kd` materials = one draw per material per enemy | M4 | open | The targets have one material each |
| R2 Model larger or smaller than its hit sphere | M4 | open | `FitModel(Largest, diameter)`, `ai_debug` sphere |
| R3 Gun values tuned on the earlier model | M2 | open | Live `gun_*` ConVars |
| R4 Point filter shimmers on far walls | M3 | accepted | PSX look |
| R5 `glClear` is macOS‑only | M2 | accepted | `#ifdef __APPLE__` |
| R6 Licenses `TODO: check` (Kenney, developer's textures) | M0, M3 | open | Confirm before a release |
| R7 Hand port drops a spike fix | all | open | Plan §1 tables, spike assessment |
| R8 Thin targets: shots past the disc edge still hit the sphere | M4 | accepted | Later: thicker models or per‑type hit shape |

## Session log

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-24 | M3 | Developer validated M3 (test pass, save and load working); merged into `integration`, branch renamed `done/mesh_level_textures`. Developer's level edits committed with it: four "Wall N Copy" wall actors (`texture=assets/textures/wall.png uv=2`) added in the editor. Next: M4 on `mesh_enemy_models` | Start M4 | — |
| 2026-11-24 | M3 | M3 on `mesh_level_textures`, three checkboxes committed: `MeshRenderer` texture and uv keys, the spike's files as they are (5ec64af); `lit.fs` `worldUV` (4b6a506); level textures, credits and the reworked `arena_01.lvl` (69ffbef). Clean Debug and Release builds pass. Verified in a short Debug run: the three textures load, and the level loads with 0 warnings. Not run: the `no "Arena" actor` warning check (enemies start in gameplay, which the short run does not reach), tiling and texel size, `set`/`save`/reload, enemies inside the arena, editor picking and gizmo (need a person) | Developer runs the M3 **Done when** check | — |
| 2026-11-24 | M2 | Developer validated M2 (test pass, V3 defaults kept); merged into `integration`, branch renamed `done/mesh_gun_view`. Next: M3 on `mesh_level_textures` | Start M3 | — |
| 2026-11-24 | M2 | M2 on `mesh_gun_view`, four checkboxes committed: `ClearDepthBuffer` (8083f83); `GunView` on its own queue with primitive parts (e86a0c2); Kenney blaster moved to `assets/models/gun/` and credited (d84e8af); `GunView` draws it fitted, with its `Kd` materials and the primitive fallback (856a797); Debug-only `gun_*` ConVars (36a5fdb). Clean Debug and Release builds pass. Verified: the Debug app loads the blaster (4 meshes, 4 materials), no warning; with the model renamed in the build folder, the app logs the warning and starts with the primitive gun (restored afterwards); `gun_*` strings are in Debug and absent from Release. Not run: the look, clipping, kick, bob, flash, the live ConVars, the 60 FPS check (they need a person). The developer's re-tune (checkbox 5) is open | Developer re-tunes with `gun_*` and runs the M2 **Done when** check | — |
| 2026-11-24 | M1 | Developer validated M1; merged into `integration`, branch renamed `done/mesh_model_loading`. Developer ticked the **Done when** line on disk. Next: M2 on `mesh_gun_view` | Start M2 | — |
| 2026-11-24 | M1 | M1 on `mesh_model_loading`, five checkboxes committed: LoadModel guard, lit shader, point filter, bounds log and texture unload (a6cc861); `GetModelBounds` (fc316cc); RenderQueue parameter reset (551bf4d); `alphaCutoff` in `lit.fs` (7b78ae5); `eng::FitModel` (9c0597b). Clean Debug and Release builds pass. Verified with a temporary probe (reverted, not committed): the three targets load and log bounds and materials, and an 8-vertex OBJ logs a warning without aborting. Not run: the game's look ("game looks unchanged" needs a person), the on-quit `still loaded` check (the probe run was stopped by timeout, so shutdown did not run) | Developer runs the M1 **Done when** check | — |
| 2026-11-24 | M0 | Developer validated M0; merged into `integration`, branch renamed `done/mesh_assets_pipeline`. Developer ticked the **Done when** line on disk. Next: M1 on `mesh_model_loading` | Start M1 | — |
| 2026-11-24 | M0 | Developer's pending task-file moves committed on `integration` as a clean-up (164c590). M0 on `mesh_assets_pipeline`: `.gitignore` negation and the enemy `.obj` files committed (504f5c2), model README and credits (4f1e1d8), `LoadTexture` filter default point, mipmaps only for trilinear (8b1b5a1). Clean Debug and Release builds pass with no warnings. Not run: the app itself (M0 changes no game-visible behavior); the README was checked by hand against §2, not by diff; `LoadTexture` has no callers yet, so the new default is untested at runtime | Developer runs the M0 **Done when** check | — |
| 2026-11-24 | — | Tracker written for `Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md` | Developer reviews, gives the go for M0 | — |
