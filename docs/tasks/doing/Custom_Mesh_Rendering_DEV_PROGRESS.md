# Development Progress — Custom Mesh Rendering

Tracks the implementation of [`Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md`](Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md). Phase names and **Done when** criteria match
§4 of that plan. The spike it ports from is tracked in [`OBJ_Gun_Spike_DEV_PROGRESS.md`](../done/OBJ_Gun_Spike_DEV_PROGRESS.md) (branch `spike/obj-gun-rendering`).
History of this plan (decision log, answered questions, risks, session log): [`devlogs/Custom_Mesh_Rendering_DEV_LOG.md`](../../devlogs/Custom_Mesh_Rendering_DEV_LOG.md).

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- At the end of every work session, add a line to the **Session log** in the history file. When the plan changes, add a **Decision** (M‑Dn) there too.
- Open questions for the developer go in an **Open questions** section in this file. Once a question is answered, move it to the history file.
- Branches: one per phase from `integration`, one commit per checkbox (except **Done when**, checked by the developer); a finished branch is renamed `done/<branch>` ([`tasks/done/TASK_04.md`](../done/TASK_04.md)).
- Porting (plan M‑D1): the spike branch is **never merged**. Take a finished file with `git checkout spike/obj-gun-rendering -- <file>`, then remove what M‑D14 lists. Read spike files with `git show spike/obj-gun-rendering:<path>`.
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| M0 | Asset pipeline | `done/mesh_assets_pipeline` | ✅ | 2026-11-24 | 2026-11-24 | `.gitignore`, model README + credits, enemy files committed, `LoadTexture` filter |
| M1 | Models in the engine | `done/mesh_model_loading` | ✅ | 2026-11-24 | 2026-11-24 | `LoadModel` guard / lit / bounds / texture unload, parameter reset, `alphaCutoff`, `FitModel` |
| M2 | Model gun | `done/mesh_gun_view` | ✅ | 2026-11-24 | 2026-11-24 | Depth‑cleared gun pass, Kenney blaster, Debug `gun_*` tuning |
| M3 | Textured levels | `done/mesh_level_textures` | ✅ | 2026-11-24 | 2026-11-24 | `texture=` / `uv=`, `worldUV`, textured arena |
| M4 | Enemy meshes | `mesh_enemy_models` | 🟡 | 2026-11-24 | | Targets for Drifter / Stalker / Brute, sphere + pyramid fallback |

**Current focus:** M4 Enemy meshes.
**Branch:** `mesh_enemy_models`

---

## Phase checklists

### Phase M0 — Asset pipeline ✅
- [x] `.gitignore`: `!assets/models/**/*.obj`; commit the developer's `assets/models/enemies/` (`target-small`, `target-large`, `target-detail` `.obj` + `.mtl`, `Textures/colormap.png`) (§1.2, M‑D2) (+ done in one commit: the rule and the files depend on each other)
- [x] `assets/models/README.md` with the export rules of §2; `assets/models/CREDITS.txt` (Kenney targets, `TODO: check`); `assets/textures/CREDITS.txt` (§1.2, §2) (+ the textures credits list is empty until M3: no textures yet)
- [x] `ResourceManager::LoadTexture(path, filter = TEXTURE_FILTER_POINT)`, mipmaps only for trilinear (§1.1, M‑D5) (+ the cache is keyed by path, so the filter applies on the first load of a path only)
- [x] **Done when:** the Debug and Release builds pass; the three target `.obj` files are tracked by git (`git ls-files assets/models/enemies`); the README matches §2.

### Phase M1 — Models in the engine ✅
- [x] `ResourceManager::LoadModel`: refuse OBJ faces over 5 vertices (warning + `nullptr`); default lit shader and point filter on every material; mesh / material counts and bounds logged; private `UnloadModelAndTextures` used by `UnloadAll` and after a failed load (§3.1, M‑D4, M‑D6) (+ the face check reads `.obj` paths only; `UnloadModelAndTextures` is a private static member)
- [x] `ResourceManager::GetModelBounds(path)`: bounds cached at load (§1.1) (+ returns `nullptr` for a model that is not loaded)
- [x] `RenderQueue`: reset the shader's known material parameters before a raw‑material draw (§1.1, M‑D8) (+ the reset is a `ResetParameters` helper, shared with `BindMaterial`)
- [x] `lit.fs`: `alphaCutoff` discard, 0 = off (§1.2)
- [x] `render/ModelFit.h/.cpp`: `eng::FitModel(bounds, FitAxis, size)` with `FitAxis::Z` and `FitAxis::Largest`; exported in `eng.h` (§1.1, M‑D7) (+ a zero extent keeps scale 1)
- [x] **Done when:** the game looks unchanged; loading the three targets from a temporary call logs their bounds and materials; an OBJ with an 8‑vertex face logs a warning and does not abort; on quit the log has no `still loaded` warning.

### Phase M2 — Model gun ✅
- [x] `render/RenderUtil.h/.cpp`: `eng::ClearDepthBuffer()` (`#ifdef __APPLE__`, no‑op elsewhere), exported in `eng.h` (§1.1, M‑D9) (+ the spike's files taken as they are)
- [x] `GunView`: pose matrix, own `RenderQueue`, sun rotated into view space, depth‑cleared pass, primitive parts as lit materials (§3.3, M‑D9) (+ depth test is on now, the parts are no longer drawn far to near)
- [x] Kenney blaster from the spike (`assets/models/prototype/gun/`) moved to `assets/models/gun/` and credited; `GunView` draws it with `FitModel(bounds, Z, length)` and its own `Kd` materials (M‑D3), falls back to the primitives with a warning; `GunView::Tuning` with the V3 defaults (§3.3) (+ the spike's `gun.obj` and `gun.mtl` taken as they are; the spike's reload command is not ported, M‑D14)
- [x] Debug‑only ConVars `gun_obj`, `gun_ofs`, `gun_rot`, `gun_len`, `gun_muzzle`; `GunView*` in `GameCommandContext` (`ArenaGame.cpp`) (§1.3, M‑D13) (+ the ConVars are behind `#ifndef NDEBUG` in `GameCommands.cpp`)
- [x] The developer's re‑tuned values on the Kenney blaster become the `Tuning` defaults (§3.3) (+ the developer kept the V3 defaults: no value change)
- [x] **Done when:** the gun is the Kenney blaster, lit, with its `.mtl` colors; it never clips into walls; kick, bob, tip flash and ready light sit on the model; the `gun_*` ConVars move it live in Debug and do not exist in Release; with the model file renamed, the game starts with the primitive gun and a warning; 60 FPS is unchanged.

### Phase M3 — Textured levels ✅
- [x] `MeshRenderer`: `texture=<path|none>` and `uv=<meters>` saved and loaded; an own material when tint, lit or a texture is set (§1.1, M‑D10) (+ the spike's files taken as they are; the keys are written with the own material, as in the spike)
- [x] `lit.fs`: `worldUV` box projection, 0 = off (§1.2, M‑D10) (+ the `alphaCutoff` comment of M1 is kept)
- [x] `assets/textures/wall.png`, `floor.png`, `ceiling.png` from the spike, credited; `arena_01.lvl` with textured floor, ceiling and walls (`uv=2`) and the `"Arena"` actor's `MeshRenderer` hidden (`visible=0`) (§1.2) (+ the level is the spike's version, which differs from integration only in these lines)
- [x] **Done when:** floor, ceiling and walls tile with square texels; `set MeshRenderer texture=… uv=…`, `save` and a reload keep the values; meshes without the keys look as before; there is no `EnemyManager: no "Arena" actor` warning and the enemies stay inside the arena; editor picking and the gizmo still work.

### Phase M4 — Enemy meshes 🟡
- [x] `EnemyArchetype`: `modelPath` and `modelYawDeg`; Drifter `target-small`, Stalker `target-large`, Brute `target-detail` (`EnemyTypes.h/.cpp`, §3.4, M‑D15)
- [x] `EnemyManager::Init`: load the models, compute the fit (`FitModel(bounds, Largest, 2·bodyRadius)` · yaw), build the per‑type material and flash material (`EnemyMaterials`, §3.2, §3.4) (+ textured models only: a color-only model falls back to the sphere, with a warning; the fit uses the archetype's body radius)
- [x] `RenderSystem`: `SubmitModel` with `fit · scale · rotation · translation`, flash material while flashing, sphere and pyramid when the type has no model (§3.4, M‑D11, M‑D12) (+ a type with a model draws the model in place of the sphere and pyramid)
- [ ] Check the fit with `ai_debug 1` (draws the hit sphere); adjust `modelYawDeg` signs if a target faces away (§3.4)
- [ ] **Done when:** the three types draw their targets with the `colormap.png` colors (not white), the face toward their movement; with a model file renamed, that type draws the sphere and pyramid and a warning is logged; the flash, squash, lunge and death shrink look right on the model; shots that visibly hit the model register (the model sits inside the hit sphere); 20 enemies on screen keep 60 FPS.
