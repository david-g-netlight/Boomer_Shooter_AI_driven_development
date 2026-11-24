# Spike Plan — OBJ Gun Rendering (pre‑production)

Builds on [`Custom_rendering_capabilities_workload_assesment.md`](Custom_rendering_capabilities_workload_assesment.md)
(gaps G1–G8, decisions in §5 and §6 there).
Source task: [`tasks/doing/TASK_11.md`](../tasks/doing/TASK_11.md).

**Goal:** see a Blockbench OBJ gun with its PSX texture in the player's hands, in game, and decide if it looks good.
Along the way, close the rendering gaps G1, G2, G3, G4, G6, G7 and G8. G5 (skinning) is out of scope.

**What this is:** a tech spike on its own branch. The code is written to be simple and easy to port or throw away,
not to ship. Enemies keep their sphere + pyramid.

**Status:** draft for the developer's review. Nothing is implemented.

---

## 0. Decisions taken (to review)

| # | Decision | Choice | Why |
|---|---|---|---|
| S‑D1 | Branch | **`spike/obj-gun-rendering`** from `integration`. Phase branches are cut from it and merged back into it (the TASK_04 flow, with the spike branch in place of `integration`). The spike branch is **never merged into `integration`**: what is kept is ported later (§8) | A spike must be free to cut corners (tuning commands, hard‑coded paths) without leaving them in `integration` |
| S‑D2 | Model format and loader | **OBJ + MTL + PNG**, loaded by raylib's built‑in loader (tinyobj) through `ResourceManager::LoadModel` | Assessment §3.3, developer answers Q2 / Q4. No new library |
| S‑D3 | Test asset | Provided by the developer: `assets/models/prototype/gun/gun.obj`, `gun.mtl`, `gun.png` | Developer answer to TASK_11. The plan creates no placeholder model |
| S‑D4 | Lit shader on models (G1) | `ResourceManager::LoadModel` gives every material of a loaded model the **default lit shader** when one is set (`SetDefaultLitShader`), and keeps the material's texture and color | Same rule as `lit=1` in level files: "the lit shader of the game". One line at load time; no per‑draw override needed |
| S‑D5 | Gun pass depth (G2) | The view‑model pass **clears the depth buffer** and draws with depth test **on**. New engine helper `eng::ClearDepthBuffer()` (flushes the rlgl batch, then `glClear(GL_DEPTH_BUFFER_BIT)`) | rlgl has no depth‑only clear. Clearing depth keeps today's property (the gun never clips into walls) and lets a real mesh hide its own back faces |
| S‑D6 | How the gun is drawn | `GunView` owns a small **`eng::RenderQueue`**, submits the model with the gun's pose matrix and draws it with its own camera | Reuses material binding and scene uniforms. No new render path |
| S‑D7 | Gun lighting | The sun direction is **rotated into view space** (by the main camera's right / up / forward) before the gun queue is drawn. `viewPosition` = origin (the gun camera) | The gun camera never moves, so a world‑space sun would light the gun the same way whatever the player looks at |
| S‑D8 | Size and placement | Code **recenters** the model on its bounds and **scales** it so its length along −Z is `GUN_LENGTH` (0.48 m, today's gun). Pivot and Blockbench units do not matter | Blockbench units and pivot choices cannot then break the placement; the gun keeps today's size on screen |
| S‑D9 | Muzzle tip and ready light | Stay **primitives** drawn over the model, at offsets in the model's (recentered) space, tunable live | The flash color and the ready light are animated in code today. Mapping them onto OBJ sub‑meshes is not needed to judge the look |
| S‑D10 | Texture filter (G4) | `LoadTexture(path, filter)`, **`TEXTURE_FILTER_POINT` by default**, mipmaps only for trilinear. The model's own textures get the same filter at load | PSX look (Q1). Point + no mipmaps is the classic look; shimmering at distance is accepted for the gun (always close) |
| S‑D11 | Comparison and fallback | ConVar **`gun_obj 0|1`** switches between the OBJ gun and the primitive gun. If the OBJ fails to load, the primitive gun is drawn and a warning is logged | Task requirement: compare both in the same session; never a gun‑less player |
| S‑D12 | Live tuning | Spike‑only ConVars `gun_ofs x y z`, `gun_rot pitch yaw roll`, `gun_len` and the command `gun_reload` | Placing a view model by recompiling is slow. Final values are copied into constants when the spike ends |
| S‑D13 | Level textures (G6) | `MeshRenderer` saves / loads `texture=<path>` and `uv=<meters>`; edited with the console's `set MeshRenderer texture=...` | **Correction to the assessment:** there is no inspector panel in the current editor (`src/engine/editor` has gizmo, picker, toolbar; fields are edited with `get` / `set`). The `.lvl` keys are all `set` needs |
| S‑D14 | Tiling (G8) | Material parameter **`worldUV`** (meters per texture repeat, 0 = off): `lit.fs` takes UVs from the world position, projected on the dominant normal axis (box projection) | The arena is one unit cube scaled 16 × 6 × 41: a single UV scale cannot fit floor and walls at once. Box projection fits any scaled primitive, needs no mesh change |
| S‑D15 | Transparent texels | Material parameter **`alphaCutoff`** (0 = off): `lit.fs` discards fragments under it. The gun is submitted with an **`eng::Material` override** built from the model (lit shader, the model's albedo texture, `alphaCutoff`), because a model's raw raylib material carries no parameters | Blockbench textures often have transparent pixels (sights, holes). Without a cutout they draw as solid color and write depth |
| S‑D16 | PSX shader effects | **Out of scope** (vertex snapping, affine textures) | Developer answer to TASK_11 |
| S‑D17 | Raw model materials and leftover uniforms | Before a `rawMaterial` draw, `RenderQueue` resets the shader's known material parameters to 0, as `BindMaterial` already does for engine materials | Found while writing the tracker. With G1, raw model materials use the lit shader, and would otherwise inherit the last engine material's `emissive` / `alphaCutoff` / `worldUV` (e.g. an enemy's flash) |

---

## 1. Blockbench → OBJ export rules

| Item | Rule | Why / how it is checked |
|---|---|---|
| Project type | *Generic Model* | Free geometry, no Minecraft limits |
| Axes | **+Y up, barrel pointing north (−Z)**, the side the player sees on the left (−X) and top (+Y) | The view model looks down −Z. A wrong axis is fixed live with `gun_rot`, then written back into the model |
| Origin / pivot | Anything | Code recenters on the bounds (S‑D8) |
| Units / scale | Anything (Blockbench: 16 px = 1 block) | Code scales to `GUN_LENGTH`. The load log prints the raw bounds |
| Faces | Quads are fine | raylib parses with triangulation on |
| Texture | One PNG, **power of two** (32, 64 or 128 px), 16 px per block for a uniform texel size; transparent pixels only where a hole is wanted | PSX look; one texture = one material = one draw |
| Export | *File → Export → Export OBJ Model*, save `gun.obj` + `gun.mtl`; save the texture as `gun.png` in the **same folder** | raylib loads the MTL's texture relative to the OBJ's folder |
| `.mtl` check | Open it: `map_Kd gun.png` must be a **bare file name**, not an absolute path from your disk | The most common OBJ breakage; an absolute path loads on your Mac only |
| Material color | `Kd 1 1 1` | `Kd` multiplies the texture |
| Poly budget | ≤ 1 500 triangles | Low poly, PSX |
| Credits | Add a line to `assets/models/CREDITS.txt` (author, license) | Same rule as audio |

After a re‑export: `cmake --build build --target copy_assets`, then `gun_reload` in the console (no restart).

---

## 2. Placement in the view‑model pass

```
pose  = Translate(ORIGIN + offset + bob + kick) · RotateX(-kick·6°) · RotateY(-3°) · Rotate(gun_rot) · Scale(s) · Translate(-boundsCenter)
s     = GUN_LENGTH / (bounds.max.z - bounds.min.z)
muzzle (model space, recentered) = (0, MUZZLE_Y, -half length) + muzzle offset
```

- `ORIGIN = { 0.19, -0.15, -0.78 }`, bob, kick and the −3° turn are today's values (`GunView.cpp`); only the matrix
  replaces `rlPushMatrix` / `rlTranslatef` / `rlRotatef` / `rlScalef`.
- The camera stays as today: own camera at the origin, FOV 50°, near 0.02, far 10.
- Tip flash and ready light are drawn with the **same pose**, after the model (S‑D9).
- If the model is too long (wide gun) or too thin, change `gun_len`; the length axis is always Z after `gun_rot`.

---

## 3. What changes, by file

| File | Change | Gap | Ported? |
|---|---|---|---|
| `render/RenderUtil.h/.cpp` (new) | `ClearDepthBuffer()`: `rlDrawRenderBatchActive()` + `glClear(GL_DEPTH_BUFFER_BIT)` (`<OpenGL/gl3.h>`, `GL_SILENCE_DEPRECATION`) | G2 | Yes |
| `resources/ResourceManager.h/.cpp` | `LoadTexture(path, filter = POINT)`; `LoadModel`: default lit shader on its materials, filter on its textures, logs bounds; `UnloadAll` + failed load: unload the model's textures (skip `rlGetTextureIdDefault()`, skip duplicates); `ReloadModel(path)` (spike) | G1, G3, G4 | Yes (`ReloadModel`: maybe) |
| `render/RenderQueue.cpp` | Raw‑material draws reset the shader's known material parameters to 0 | S‑D17 | Yes |
| `assets/shaders/lit.fs` | `worldUV` box projection, `alphaCutoff` discard (both 0 = off, current materials unchanged) | G8, S‑D15 | Yes |
| `render/MeshRenderer.cpp` | `texture=` and `uv=` keys in `Serialize` / `Deserialize` (texture loaded through `ResourceManager`) | G6 | Yes |
| `assets/models/`, `assets/textures/` (new) | `README.md`, `CREDITS.txt`, `models/prototype/gun/` (developer's files) | G7 | Yes (not `prototype/`) |
| `game/player/GunView.h/.cpp` | Pose matrix, own `RenderQueue`, view‑space sun, depth‑cleared pass, OBJ path with fallback to primitives, `m_useObj` flag, tuning values | G2 + gun | Partly (§8) |
| `game/dev/GameCommands.h/.cpp` | `GunView*` in `GameCommandContext`; `gun_obj`, `gun_ofs`, `gun_rot`, `gun_len`, `gun_reload` | — | `gun_obj` maybe; rest no |
| `game/ArenaGame.cpp` | Pass `&m_gunView` in the command context | — | No |

Engine changes stay engine‑generic: nothing in `src/engine` knows about guns.

---

## 4. Phases

Ordered so the OBJ gun is on screen by **V2**. Each phase is one or a few commits on the spike branch, and ends
with the developer's in‑game check.

### V0 — Branch, folders, filter (≈ 0.25 day) — G4, G7

1. Branch `spike/obj-gun-rendering` from `integration`.
2. `assets/models/README.md` (export rules of §1), `assets/models/CREDITS.txt`, `assets/textures/`.
3. `LoadTexture(path, filter)` (S‑D10).
4. Developer drops `gun.obj`, `gun.mtl`, `gun.png` in `assets/models/prototype/gun/`.

**Check:** ☐ build passes ☐ the three files are copied next to the executable (`build/.../assets/models/prototype/gun/`)
☐ `map_Kd` in `gun.mtl` is a bare file name.

### V1 — Depth‑cleared, lit gun pass (≈ 0.5 day) — G2

1. `eng::ClearDepthBuffer()`.
2. `GunView::Draw`: clear depth, keep depth test on, build the pose matrix (§2).
3. The primitive gun moves from `DrawCube` / `DrawCylinderEx` to primitive meshes + lit `eng::Material`s submitted
   to the gun's `RenderQueue`, with the view‑space sun (S‑D6, S‑D7).

**Check:** ☐ the primitive gun has the same size and position as on `integration` ☐ it is now shaded and the
shading changes when you turn toward / away from the sun ☐ walk into a wall: the gun never clips into it ☐ kick,
bob, muzzle flash and ready light work as before.

### V2 — OBJ gun on screen (≈ 1 day) — G1, G3 — **main validation**

1. `LoadModel`: lit shader, point filter, bounds log (S‑D4, S‑D10); texture unload (G3).
2. `GunView::Init` loads `assets/models/prototype/gun/gun.obj`. On failure: warning, primitive gun (S‑D11).
3. Recenter + scale from bounds, tip and ready light on top (S‑D8, S‑D9).
4. `gun_obj`, `gun_ofs`, `gun_rot`, `gun_len`, `gun_reload` (S‑D12).
5. `alphaCutoff` in `lit.fs`; the gun drawn with an `eng::Material` override made from the model's texture, `alphaCutoff` 0.5 when the texture has transparent pixels (S‑D15).
6. `RenderQueue`: reset the material parameters before raw‑material draws (S‑D17).

**Check — does it work:**
☐ the log shows the OBJ loaded, its bounds and its texture (`TEXTURE: [ID n] ... loaded`)
☐ rename `gun.obj` → the game starts, logs a warning, shows the primitive gun
☐ `gun_obj 0` / `gun_obj 1` switch between the two guns
☐ quit: the log shows the gun's texture unloaded, no `ResourceManager destroyed with ... still loaded` warning

**Check — does it look right (the spike's question):**
☐ the silhouette reads as a gun at the default FOV, at a glance, without the HUD
☐ the texture is **not flipped** (text or a marked pixel on the texture appears where it was painted), **not
stretched** (square pixels look square) and **not blurry** (sharp pixel edges)
☐ the model is lit by the sun: one side brighter, ambient on the other, the top light reads
☐ no back faces over front faces, no holes, no faces missing (= winding / normals are right)
☐ transparent texels are holes, not black or white blocks
☐ the gun does not clip into walls, the near plane does not cut it
☐ kick, bob and muzzle flash feel attached to the model (the tip sits on the barrel's end)
☐ side by side with the primitive gun (`gun_obj` toggle), the OBJ looks better
☐ 60 FPS is unchanged (`fps 1`)

### V3 — Tune and decide (≈ 0.5 day, mostly the developer)

1. Tune with `gun_ofs`, `gun_rot`, `gun_len`; iterate the art in Blockbench + `gun_reload`.
2. Copy the final values into `GunView.cpp` constants; fix the model's axes in Blockbench if `gun_rot` was needed.
3. Take 3 screenshots (idle, firing, against a bright wall) for the record.

**Decision gate:** **Go** (port to `integration`, §8) · **Iterate art** (pipeline fine, model not yet) · **No‑go**
(the OBJ path has a blocker; write it down in §7 and in the assessment).

### V4 — Textures in level files (≈ 0.5 day) — G6

1. `MeshRenderer` `texture=` / `uv=` keys (S‑D13).
2. One test texture in `assets/textures/`, set on a crate with `select` + `set MeshRenderer texture=assets/textures/crate.png`,
   `save`, reload.

**Check:** ☐ the crate is textured, point filtered ☐ save → reload keeps it (the `.lvl` line has `texture=`) ☐ a
mesh without `texture=` looks exactly as before.

### V5 — Tiling on scaled level geometry (≈ 0.5–1 day) — G8

1. `worldUV` in `lit.fs` (S‑D14), read from `uv=` in `MeshRenderer`.
2. Arena: `set MeshRenderer texture=assets/textures/wall.png uv=2` on the arena cube.

**Check:** ☐ floor, ceiling and the four walls repeat the texture with square texels (no stretch on the 41 m walls)
☐ no visible seam at the corners beyond the texture's own edge ☐ the editor's gizmo and picking still work on
the textured arena ☐ the enemies and the player body (no `uv=`) are unchanged.

**Total: about 3.25–3.75 days**, including the developer's checks.

---

## 5. Out of scope

Enemies on models (see §9), skinning (G5), PSX shader effects, `model=` key in level files (only `texture=` is
needed for the levels; the gun is built in code), normal / emissive maps, a model or texture picker UI.

---

## 6. Order of work at a glance

| Phase | Gaps | Days | Gun visible as OBJ? |
|---|---|---|---|
| V0 | G4, G7 | 0.25 | — |
| V1 | G2 | 0.5 | primitives, lit |
| V2 | G1, G3 | 1 | **yes** |
| V3 | — | 0.5 | tuned + decision |
| V4 | G6 | 0.5 | — |
| V5 | G8 | 0.5–1 | — |

---

## 7. Risks

| # | Risk | Effect | Mitigation |
|---|---|---|---|
| R1 | `map_Kd` holds an absolute path, or the PNG is not next to the MTL | Gun is white (raylib default texture) | §1 `.mtl` check; V2 check reads the texture load line in the log |
| R2 | V coordinate flipped between Blockbench and raylib's OBJ loader | Texture upside down | V2 "not flipped" check with a marked pixel; fix by flipping the UVs once at load (spike) or in the export |
| R3 | Inverted winding or missing normals | Model inside‑out (culled faces) or black | V2 check; temporary `rlDisableBackfaceCulling()` to diagnose; Blockbench exports normals for box faces |
| R4 | Unknown Blockbench units / axes | Gun huge, tiny or sideways | Bounds‑based scale and recenter (S‑D8), `gun_rot`; bounds in the log |
| R5 | Narrow view‑model FOV (50°) or near plane (0.02) distorts / cuts a longer model | Looks wrong only in game | `gun_len`, `gun_ofs`; the FOV can become a ConVar if needed |
| R6 | raylib `5.6-dev` OBJ loader splits meshes per material | Several draws, harmless; a multi‑texture gun needs one material per texture | One texture per gun (§1) |
| R7 | Point filter, no mipmaps | Shimmer on far level textures (V5) | Accepted for the PSX look; trilinear per texture if a surface needs it |
| R8 | `glClear` include is macOS‑specific | Other platforms need another header | Wrapped in `#ifdef __APPLE__`; the engine is Mac‑only today |
| R9 | Spike code leaks into `integration` | Tuning ConVars and hard‑coded prototype path ship | No merge (S‑D1); port list in §8 |

---

## 8. When the spike ends: port or throw away

| Item | Port to `integration` | Throw away |
|---|---|---|
| `ClearDepthBuffer`, depth‑cleared gun pass, pose matrix | ✅ | |
| `LoadTexture` filter, `LoadModel` lit shader + filter, model texture unload | ✅ | |
| `lit.fs` `worldUV`, `alphaCutoff`; `RenderQueue` raw‑material reset | ✅ | |
| `MeshRenderer` `texture=` / `uv=` | ✅ | |
| `assets/models/` README + CREDITS, `assets/textures/` | ✅ | |
| Gun's `RenderQueue` + view‑space sun | ✅ | |
| OBJ gun with fallback to primitives | ✅ if Go, with the final model in `assets/models/gun/` | `prototype/` folder |
| `gun_obj` toggle | Optional (Debug builds) | |
| `gun_ofs`, `gun_rot`, `gun_len`, `gun_reload`, `ReloadModel` | | ✅ (values copied into constants) |

The port is a normal plan + DEV_PROGRESS on `integration`, one phase per row group above.

---

## 9. Answer: what would it take to go from this spike to production for the enemies (R4)?

Everything the enemies need from the engine is ported from this spike (lit models, filter, texture unload). What
is left is game work in `src/game/enemies`, **about 1.5–2 days** plus the art:

| Step | Work |
|---|---|
| Assets | One OBJ + PNG per type (Drifter, Stalker, Brute), body and spike in one model, facing −Z, in `assets/models/enemies/` |
| Archetype table | A model path per type (`EnemyTypes.cpp`); models loaded once in `EnemyManager::Init` |
| Size | Scale from the model's bounds so the model fits inside the hit sphere of `bodyRadius` (the hit test stays the analytic sphere, E‑D2, no change) |
| Materials | Per type: an `eng::Material` with the lit shader and **the model's own texture** (`SetTexture(model.materials[..].maps[ALBEDO].texture)`), plus a flash material with `emissive = 1`. An override `Material` without the texture would lose it |
| `RenderSystem` | `queue.SubmitModel(model, flashing ? flash : body, matrix)` instead of sphere + pyramid. The matrix keeps scale‑in, squash, lunge, slerped rotation and death shrink: all transform animation (Q3), unchanged |
| Draws | 1 per enemy (single‑texture models) instead of 2; 20 enemies = 20 draws, no batching needed |
| Risks | A model larger than its hit sphere looks "hit through"; one texture per enemy model; the spike's PSX texel density rule applies |
| Not needed | Skinning, new components, ECS changes, Bullet bodies |
