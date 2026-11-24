# Development Progress — OBJ Gun Spike

Tracks the implementation of [`Rendering_OBJ_Gun_Spike_PLAN.md`](../../deliverables/Rendering_OBJ_Gun_Spike_PLAN.md). Phase names and **Done when** criteria match
§4 of that plan. The previous work's history stays in [`Gameplay_Audio_DEV_PROGRESS.md`](../done/Gameplay_Audio_DEV_PROGRESS.md).
History of this plan (decision log, answered questions, risks, session log): [`devlogs/OBJ_Gun_Spike_DEV_LOG.md`](../../devlogs/OBJ_Gun_Spike_DEV_LOG.md).

Gun files: `assets/models/prototype/gun`.

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- At the end of every work session, add a line to the **Session log** in the history file. When the plan changes, add a **Decision** (S‑Dn) there too.
- Open questions for the developer go in an **Open questions** section in this file. Once a question is answered, move it to the history file.
- Branches: **this is a spike** (plan S‑D1). Create `spike/obj-gun-rendering` from `integration` once. Each phase gets its own branch **from `spike/obj-gun-rendering`**, one commit per checkbox (except **Done when**, checked by the developer), merged back into `spike/obj-gun-rendering` and renamed `done/<branch>` ([`tasks/done/TASK_04.md`](../done/TASK_04.md), with the spike branch in place of `integration`). Nothing is merged into `integration`; the port is a later plan (§8).
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| V0 | Branch, folders, filter | `gun_spike_folders_filter` | 🟡 | 2026-11-24 | | G4, G7; developer's `gun.obj` / `.mtl` / `.png` |
| V1 | Depth‑cleared, lit gun pass | `gun_spike_depth_pass` | ⬜ | | | G2; primitive gun, lit, view‑space sun |
| V2 | OBJ gun on screen | `gun_spike_obj_gun` | ⬜ | | | G1, G3; **main validation**; fallback, `gun_obj` toggle, tuning ConVars |
| V3 | Tune and decide | `gun_spike_tune` | ⬜ | | | Final constants, screenshots, Go / Iterate art / No‑go |
| V4 | Textures in level files | `gun_spike_level_textures` | ⬜ | | | G6; `texture=` / `uv=` on `MeshRenderer` |
| V5 | Tiling on scaled level geometry | `gun_spike_world_uv` | ⬜ | | | G8; `worldUV` box projection, textured arena |

**Current focus:** V0, waiting for the developer's **Done when** check.
**Branch:** `gun_spike_folders_filter`

---

## Phase checklists

### Phase V0 — Branch, folders, filter 🟡
- [x] `assets/models/README.md` (the Blockbench → OBJ export rules of §1), `assets/models/CREDITS.txt`, `assets/textures/` (G7, §4 V0) (+ `assets/textures/` has a `.gitkeep` so git tracks the empty folder)
- [x] `ResourceManager::LoadTexture(path, filter = TEXTURE_FILTER_POINT)`; mipmaps only for `TEXTURE_FILTER_TRILINEAR` (G4, S‑D10) (+ no other caller existed; the cache is by path, so the first filter wins)
- [x] The developer's `gun.obj`, `gun.mtl`, `gun.png` committed in `assets/models/prototype/gun/`, with a line in `assets/models/CREDITS.txt` (S‑D3, §1) (+ moved with `git mv` from `tech_spike/`; the tracker start edit is in this commit)
- [ ] **Done when:** the build passes; the three files are copied next to the executable (`build/.../assets/models/prototype/gun/`); `map_Kd` in `gun.mtl` is a bare file name

### Phase V1 — Depth‑cleared, lit gun pass ⬜
- [ ] `render/RenderUtil.h/.cpp` (new): `eng::ClearDepthBuffer()` = `rlDrawRenderBatchActive()` + `glClear(GL_DEPTH_BUFFER_BIT)` (`<OpenGL/gl3.h>` with `GL_SILENCE_DEPRECATION`, inside `#ifdef __APPLE__`) (S‑D5, §3)
- [ ] `GunView::Draw`: clear depth, depth test **on**, one pose matrix (§2) instead of `rlPushMatrix` / `rlTranslatef` / `rlRotatef` / `rlScalef`; same values (`ORIGIN`, bob, kick, −3°, 0.7 scale) (S‑D5, §2)
- [ ] Primitive gun parts as primitive meshes + lit `eng::Material`s, submitted to a `RenderQueue` owned by `GunView` and drawn with the gun camera; sun direction rotated into view space with the main camera's right / up / forward (`Engine::GetMainCamera()`, `Engine::GetWorld()`) (S‑D6, S‑D7)
- [ ] **Done when:** the primitive gun has the same size and position as on `integration`; it is now shaded and the shading changes when you turn toward / away from the sun; walking into a wall, the gun never clips into it; kick, bob, muzzle flash and ready light work as before

### Phase V2 — OBJ gun on screen ⬜
- [ ] `ResourceManager::LoadModel`: the default lit shader on every material of the model (when set), the point filter on the model's textures, the raw bounds in the load log (G1, S‑D4, S‑D10)
- [ ] `ResourceManager`: unload a model's own textures in `UnloadAll` and on a failed load (skip `rlGetTextureIdDefault()` and duplicates) (G3, §3)
- [ ] `RenderQueue::Draw`: before a `rawMaterial` draw, reset the shader's known material parameters to 0, as `BindMaterial` does (S‑D17)
- [ ] `alphaCutoff` in `assets/shaders/lit.fs` (0 = off, discard under it) (S‑D15)
- [ ] `GunView::Init` loads `assets/models/prototype/gun/gun.obj`; on failure a warning and the primitive gun (S‑D11). Recenter on the bounds, scale to `GUN_LENGTH` = 0.48 along −Z (S‑D8, §2). Drawn with an `eng::Material` override (lit, the model's albedo texture, `alphaCutoff` 0.5 when the texture has transparent pixels) (S‑D15). Tip flash and ready light stay primitives drawn after the model with the same pose (S‑D9)
- [ ] `ResourceManager::ReloadModel(path)` (spike); `GunView*` in `GameCommandContext` (`ArenaGame` passes `&m_gunView`); ConVars `gun_obj 0|1`, `gun_ofs x y z`, `gun_rot pitch yaw roll`, `gun_len`, command `gun_reload` (S‑D11, S‑D12)
- [ ] **Done when:**
  - Works: the log shows the OBJ loaded, its bounds and its texture (`TEXTURE: [ID n] ... loaded`); renaming `gun.obj` → the game starts, logs a warning, shows the primitive gun; `gun_obj 0` / `gun_obj 1` switch between the two guns; on quit the log shows the gun's texture unloaded and no `ResourceManager destroyed with ... still loaded` warning
  - Looks right: the silhouette reads as a gun at the default FOV, at a glance, without the HUD; the texture is not flipped (a marked pixel appears where it was painted), not stretched (square pixels look square), not blurry (sharp pixel edges); the model is lit by the sun (one side brighter, ambient on the other, the top light reads); no back faces over front faces, no holes, no missing faces; transparent texels are holes, not black or white blocks; the gun does not clip into walls and the near plane does not cut it; kick, bob and muzzle flash feel attached to the model (the tip sits on the barrel's end); side by side (`gun_obj`), the OBJ looks better than the primitive gun; 60 FPS unchanged (`fps 1`)

### Phase V3 — Tune and decide ⬜
- [ ] Final `gun_ofs` / `gun_rot` / `gun_len` values copied into `GunView.cpp` constants (the model's axes fixed in Blockbench if `gun_rot` was needed, re‑exported files committed) (§4 V3)
- [ ] Three screenshots (idle, firing, against a bright wall) committed for the record (§4 V3; folder: open question Q1)
- [ ] **Done when:** the developer records the decision in the history file: **Go** (port to `integration`, §8) · **Iterate art** · **No‑go** (blocker written in plan §7 and in the assessment)

### Phase V4 — Textures in level files ⬜
- [ ] `MeshRenderer::Serialize` / `Deserialize`: `texture=<path>` (loaded through `ResourceManager::LoadTexture`) and `uv=<meters>` (the material's `worldUV` parameter); edited with `set MeshRenderer texture=...` (G6, S‑D13)
- [ ] One test texture `assets/textures/crate.png` on a crate in `arena_01.lvl`, set with `select` + `set MeshRenderer texture=assets/textures/crate.png`, then `save` (§4 V4; source of the texture: open question Q2)
- [ ] **Done when:** the crate is textured and point filtered; save → reload keeps it (the `.lvl` line has `texture=`); a mesh without `texture=` looks exactly as before

### Phase V5 — Tiling on scaled level geometry ⬜
- [ ] `worldUV` in `assets/shaders/lit.fs`: UVs from the world position, projected on the dominant normal axis, divided by `worldUV` (0 = off, mesh UVs) (G8, S‑D14)
- [ ] Arena: `set MeshRenderer texture=assets/textures/wall.png uv=2` on the `"Arena"` actor, saved in `arena_01.lvl` (§4 V5)
- [ ] **Done when:** floor, ceiling and the four walls repeat the texture with square texels (no stretch on the 41 m walls); no visible seam at the corners beyond the texture's own edge; the editor's gizmo and picking still work on the textured arena; the enemies and the player body (no `uv=`) are unchanged

---

## Open questions

| # | Question | Default if no answer |
|---|---|---|
| Q1 | Where do the V3 screenshots go? | `docs/devlogs/obj_gun_spike/` (PNG), linked from the history file |
| Q2 | Who provides `crate.png` (V4) and `wall.png` (V5)? | The agent makes 32 × 32 placeholders (a checker with one marked corner pixel, to check flips), marked as generated in `assets/textures/CREDITS.txt`; the developer can replace them at any time |
| Q3 | `gun_obj` and the tuning ConVars exist only with the dev console (Debug builds). In Release the OBJ gun is always used when it loads. OK? | Yes, as written; the Release check of V2 only covers loading, fallback and the look |
