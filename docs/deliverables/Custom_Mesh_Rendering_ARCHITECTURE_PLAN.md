# Architecture Plan: Custom Mesh Rendering

Source task: [`TASK_12.md`](TASK_12.md). Open questions: [`Task_012_Q&A.md`](Task_012_Q&A.md).
Built on the tech spike on branch `spike/obj-gun-rendering`. Its assessment is
`docs/deliverables/Rendering_OBJ_Gun_Spike_ASSESSMENT.md` and its history is `docs/devlogs/OBJ_Gun_Spike_DEV_LOG.md`.
Both files exist only on the spike branch: read them with `git show spike/obj-gun-rendering:<path>`.

**Goal:** bring custom meshes into the engine on `integration`. That means OBJ models with their `.mtl` colors or a
PSX texture, lit by the game's sun, a first‑person gun made from a model, textured and tiled level geometry, and
enemies drawn as models instead of a sphere and a pyramid.

**Status:** iteration 2: every question in `Task_012_Q&A.md` (Q1 to Q16) is answered and applied. Nothing is implemented.

---

## 0. Decisions

| # | Decision | Choice | Why |
|---|---|---|---|
| M‑D1 | How the spike reaches `integration` | **Port, never merge.** Each phase branch starts from `integration`. Engine files that the spike finished are taken from it file by file (`git checkout spike/obj-gun-rendering -- <file>`) and then cleaned. Game files are rewritten from the spike without its spike‑only parts (M‑D14) | Spike rule S‑D1. The spike branch also holds `prototype/` assets, test textures and a model reload that must not ship |
| M‑D2 | Model format | **OBJ + MTL (+ one PNG)** through raylib's loader and `ResourceManager::LoadModel` | Proven by the spike (two guns: textured Blockbench and `Kd`-colored Kenney). No new library |
| M‑D3 | Two material cases | A **textured** model is drawn with an `eng::Material` override built from its texture. A **color‑only** model (`Kd`) is drawn with **its own materials** | Spike finding: a white override hides the `.mtl` colors, and a raw raylib material carries no parameters (`alphaCutoff`) |
| M‑D4 | Lighting of models | `LoadModel` gives every material the default lit shader, so the default lit shader must be set first | Spike S‑D4. One rule for level meshes, the gun and the enemies |
| M‑D5 | Texture filter | `LoadTexture(path, filter = TEXTURE_FILTER_POINT)`, with mipmaps only for trilinear. Model textures get point filtering at load | PSX look (spike S‑D10). `LoadTexture` has no callers on `integration` today, so the new default breaks nothing |
| M‑D6 | Model safety | `LoadModel` refuses an OBJ with a face of more than 5 vertices: it logs a warning and returns `nullptr`. Every caller has a primitive fallback | raylib's tinyobj **aborts the game** on such faces. A missing art file must never crash the game or leave the player without a gun |
| M‑D7 | Fitting a model | New engine helper **`eng::FitModel`**: from the model's bounds (computed once at load), a matrix that recenters the model and scales it to a target size. The gun and the enemies both use it | The gun did this inside `GunView`, and the enemies need the same math. Blockbench and Kenney units and pivots then do not matter |
| M‑D8 | Parameter hygiene | `RenderQueue` resets the shader's known material parameters before a raw‑material draw | Spike S‑D17. Without it a color‑only enemy inherits the last flash's `emissive` |
| M‑D9 | Gun pass | Own `RenderQueue`, depth cleared (`eng::ClearDepthBuffer()`), depth test on, one pose matrix, the sun rotated into view space | Spike S‑D5 to S‑D7, validated in V1 and V2 |
| M‑D10 | Level surfaces | `MeshRenderer` keys `texture=<path|none>` and `uv=<meters>`; `lit.fs` **`worldUV`** box projection | Spike S‑D13 and S‑D14, validated in V4 and V5. `set` reaches only keys that `Serialize` writes, so both keys are always written |
| M‑D11 | Enemy hit test | It stays the **analytic sphere** of `bodyRadius`. The model is fitted to it and does not define it | E‑D2 of the enemy plan. No physics or ECS change |
| M‑D12 | Enemy fallback | An enemy type without a loaded model keeps the **sphere and pyramid** | A bad or missing file degrades instead of crashing. Models (Q5, Q15): **Drifter = `target-small`**, **Stalker = `target-large`**, **Brute = `target-detail`** |
| M‑D13 | Gun live tuning | Ported, **Debug only**: `gun_ofs`, `gun_rot`, `gun_len`, `gun_muzzle` (ConVars over `GunView::Tuning`) and `gun_obj`. The defaults are the spike's V3 values | Q3, Q4: the values were tuned on the earlier gun and must be checked on the Kenney blaster |
| M‑D14 | Spike‑only code | Not ported: `gun_reload`, `ResourceManager::ReloadModel`, `assets/models/prototype/`, the `_test` textures and `crate.png` | Q8, Q9 |
| M‑D15 | Model orientation | A per‑archetype **`modelYawDeg`** constant turns a model to face −Z when its file does not. The gun keeps `gun_rot` | The Kenney targets face ±X (thin along X), not −Z. A constant, no live ConVar (Q14) |

---

## 1. What changes, at a glance

### 1.1 Engine (`src/engine`)

| File | Change | Phase |
|---|---|---|
| `resources/ResourceManager.h/.cpp` | `LoadTexture(path, filter)`. In `LoadModel`: the n‑gon guard, the lit shader and point filter on every material, the bounds log, and the bounds cached per model (`GetModelBounds(path)`). A private `UnloadModelAndTextures`, used by `UnloadAll` and after a failed load | M0, M1 |
| `render/RenderQueue.cpp` | Parameter reset before raw‑material draws | M1 |
| `render/ModelFit.h/.cpp` (new) | `Matrix FitModel(const BoundingBox& bounds, FitAxis axis, float size)`: recenter, then scale so that the chosen extent equals `size` (`FitAxis::Z` for length, `FitAxis::Largest` for "fits in a diameter") | M1 |
| `render/RenderUtil.h/.cpp` (new) | `ClearDepthBuffer()`: flushes the rlgl batch, then `glClear(GL_DEPTH_BUFFER_BIT)` (`#ifdef __APPLE__`, a no‑op elsewhere) | M2 |
| `render/MeshRenderer.h/.cpp` | `texture=` and `uv=` keys; an own material when tint, lit or a texture is set | M3 |
| `eng.h` | Export `RenderUtil.h` and `ModelFit.h` | M1, M2 |

### 1.2 Shaders and assets

| File | Change | Phase |
|---|---|---|
| `assets/shaders/lit.fs` | `alphaCutoff` discard (M1) and the `worldUV` box projection (M3). Both read 0 = off, so existing materials are unchanged | M1, M3 |
| `.gitignore` | `!assets/models/**/*.obj` (the `*.obj` build rule hid the spike's gun for three phases) | M0 |
| `assets/models/README.md`, `CREDITS.txt` | Export rules (§2) and credits | M0 |
| `assets/textures/CREDITS.txt` | Texture credits | M0 |
| `assets/models/gun/` | The final gun model (not `prototype/`) | M2 |
| `assets/textures/wall.png`, `floor.png`, `ceiling.png` | Level textures, 64 × 64 | M3 |
| `assets/levels/arena_01.lvl` | Floor, ceiling and walls as textured cubes (`uv=2`). The `"Arena"` actor keeps a **hidden** `MeshRenderer` (`visible=0`), because `EnemyManager` reads the arena bounds from it | M3 |
| `assets/models/enemies/` | The developer's Kenney targets: `target-small` (Drifter), `target-large` (Stalker), `target-detail` (Brute), and the shared `Textures/colormap.png` (512 × 512) their `.mtl` files point to. Committed in M0 (Q16) | M0, M4 |

### 1.3 Game (`src/game`)

| File | Change | Phase |
|---|---|---|
| `player/GunView.h/.cpp` | Pose matrix, own `RenderQueue`, view‑space sun, depth‑cleared pass, model gun with primitive fallback, `Tuning` with the V3 values as defaults | M2 |
| `dev/GameCommands.h/.cpp`, `ArenaGame.cpp` | Debug only: `gun_obj`, `gun_ofs`, `gun_rot`, `gun_len`, `gun_muzzle`; `GunView*` in the command context | M2 |
| `enemies/EnemyTypes.h/.cpp` | `modelPath` and `modelYawDeg` per archetype (empty path = fallback) | M4 |
| `enemies/EnemyManager.h/.cpp` | Load the models in `Init`, cache the fit matrix, build the per‑type model materials | M4 |
| `enemies/RenderSystem.h/.cpp` (`EnemyMaterials`) | `SubmitModel` path, with the sphere and pyramid as fallback | M4 |

Nothing in `src/engine` knows about guns or enemies.

---

## 2. Asset rules (`assets/models/README.md`)

| Item | Rule |
|---|---|
| Format | OBJ + MTL, with at most one PNG. `map_Kd` is a path **relative to the `.mtl`** (a bare name, or a subfolder such as Kenney's `Textures/colormap.png`), never an absolute path, and the PNG must be committed with the model |
| Faces | Triangles or quads. **Export triangulated** when unsure: faces over 5 vertices are refused (M‑D6) |
| Axes | +Y up, front toward **−Z**. Pivot and units do not matter (M‑D7) |
| Texture | Power of two (32, 64 or 128 px, or a shared color atlas such as Kenney's 512 px `colormap.png`), one texel size across the model, transparent pixels only for holes |
| Color‑only models | `Kd` per material. Prefer **one material per model**, because each material is one draw per instance (§4.3) |
| Budget | Gun ≤ 1 500 triangles; enemy a few hundred |
| Credits | One line in `CREDITS.txt` per file, with author, source and license |

---

## 3. Architecture

### 3.1 Load path

```
ResourceManager::LoadModel(path)
  ├─ cached? → return it
  ├─ scan the .obj for faces > 5 vertices → warning + nullptr   (M-D6)
  ├─ ::LoadModel, IsModelValid, else unload model + textures → nullptr
  ├─ every material: shader = default lit shader; albedo texture → point filter   (M-D4, M-D5)
  ├─ bounds = GetModelBoundingBox, cached next to the model, logged with mesh / material counts
  └─ return Model*
UnloadAll: UnloadModelAndTextures (raylib's UnloadModel leaks the MTL textures; skip the default texture and duplicates)
```

### 3.2 Draw path

A user of a model (the gun or an enemy type) holds three things, built once:

| Held | Textured model | Color‑only model |
|---|---|---|
| `fit` | `FitModel(bounds, axis, size)` | same |
| `material` | `eng::Material`(lit, the model's albedo texture, `alphaCutoff` 0.5 when it has transparent texels) | `nullptr`: the model's own materials |
| `flash` (enemies) | the same, plus `emissive = 1` | `eng::Material`(lit, white, `emissive = 1`): the flash is a white silhouette |

Per frame: `queue.SubmitModel(model, material, fit · pose)`. `fit` runs first, so the pose works in fitted meters.

### 3.3 Gun pass (unchanged spike design)

```
pose = Scale · RotateY(-3°) · RotateX(-kick·6°) · Translate(ORIGIN + offset + bob + kick)   (raymath: left applied first)
model matrix = FitModel(bounds, Z, GUN_LENGTH) · Rotate(GUN_ROTATION) · pose
```

`GunView::Tuning` holds offset, rotation, length and muzzle. Its defaults are the developer's V3 values: offset
(0.05, −0.05, 0.2), rotation (0, 2, 5), length 0.6 and muzzle (0, 0.05, −0.1). In Debug builds the `gun_*` ConVars
change it live. When the values are re‑checked on the Kenney blaster, the new values become the defaults. The tip
flash and ready light stay primitives drawn with the same pose. When the model is missing, the gun falls back to the
primitive parts (lit materials in the same queue).

### 3.4 Enemies

```
EnemyManager::Init (after SetDefaultLitShader)
  for each type: model = LoadModel(archetype.modelPath)      // may be nullptr → fallback
                 fit   = FitModel(bounds, Largest, 2·bodyRadius) · RotateY(modelYawDeg)
                 build material + flash (§3.2)
RenderSystem (per enemy)
  matrix = fit · Scale(look.scale [· squash when flashing]) · rotation · Translate(center + lunge)
  model ? SubmitModel(model, flashing ? flash : material, matrix) : sphere + pyramid (today's code)
```

| Type | Model | Material case | Notes |
|---|---|---|---|
| Drifter | `assets/models/enemies/target-small.obj` | Textured (`colormap.png`) | 152 triangles, a disc about 0.1 × 0.2 × 0.2, thin along X |
| Stalker | `assets/models/enemies/target-large.obj` | Textured (`colormap.png`) | 168 triangles, about 0.1 × 0.34 × 0.34, thin along X |
| Brute | `assets/models/enemies/target-detail.obj` | Textured (`colormap.png`) | 328 triangles, about 0.13 × 0.36 × 0.36, thin along X |

The three models share one texture, so all are textured and flash with the texture at `emissive = 1`. The
sphere and pyramid stay in the code as the fallback for a model that fails to load.

All animation stays a transform: scale‑in, squash, lunge, slerped rotation and death shrink. There is no skinning,
no new component and no ECS change.

---

## 4. Phases

These are TASK_04 phases, each on its own branch from `integration`. After each phase the game runs and looks at
least as good as before.

### M0: Asset pipeline (`mesh_assets_pipeline`)

1. `.gitignore` negation; `assets/models/README.md` (§2) and `CREDITS.txt`; `assets/textures/CREDITS.txt`.
   The developer's enemy files in `assets/models/enemies/` (three targets and `Textures/colormap.png`) are
   committed with the rule, and credited in `assets/models/CREDITS.txt`.
2. `LoadTexture(path, filter = POINT)`, mipmaps only for trilinear.

**Done when:** the Debug and Release builds pass; the three target `.obj` files are tracked by git; the
README matches §2.

### M1: Models in the engine (`mesh_model_loading`)

1. `LoadModel`: the n‑gon guard, lit shader and point filter, bounds cached and logged, model textures unloaded.
2. `RenderQueue`: parameter reset before raw‑material draws.
3. `lit.fs`: `alphaCutoff`.
4. `eng::FitModel` and `ResourceManager::GetModelBounds`.

**Done when:** the game looks unchanged; loading the three targets (committed in M0) from a temporary call logs their
bounds and materials; an OBJ with an 8‑vertex face logs a warning and does not abort; on quit the log has no
`still loaded` warning.

### M2: Model gun (`mesh_gun_view`)

1. `eng::ClearDepthBuffer()`.
2. `GunView`: pose matrix, own `RenderQueue`, view‑space sun, primitive parts as lit materials.
3. The Kenney blaster moved to `assets/models/gun/`, drawn with `FitModel` and the M‑D3 material rule (its own `Kd`
   materials), with fallback to the primitives; `GunView::Tuning` with the V3 defaults.
4. Debug ConVars `gun_obj`, `gun_ofs`, `gun_rot`, `gun_len`, `gun_muzzle`.
5. The developer re‑tunes on the Kenney blaster; the final values become the `Tuning` defaults.

**Done when:** the gun is the Kenney blaster, lit, with its `.mtl` colors; it never clips into walls; kick, bob, tip
flash and ready light sit on the model; the `gun_*` ConVars move it live in Debug and do not exist in Release; with
the model file renamed, the game starts with the primitive gun and a warning; 60 FPS is unchanged.

### M3: Textured levels (`mesh_level_textures`)

1. `MeshRenderer` `texture=` and `uv=` (save and load).
2. `lit.fs` `worldUV`.
3. Level textures and the reworked `arena_01.lvl`, with a hidden `"Arena"` mesh.

**Done when:** floor, ceiling and walls tile with square texels; `set MeshRenderer texture=… uv=…`, `save` and a
reload keep the values; meshes without the keys look as before; there is no `EnemyManager: no "Arena" actor`
warning and the enemies stay inside the arena; editor picking and the gizmo still work.

### M4: Enemy meshes (`mesh_enemy_models`)

1. `modelPath` and `modelYawDeg` in the archetype table: Drifter `target-small`, Stalker `target-large`,
   Brute `target-detail` (§3.4).
2. `EnemyManager::Init`: load the models, compute the fit, build the material and the flash material.
3. `RenderSystem`: the model path with the sphere and pyramid fallback; the hit flash on both material cases.
4. `ai_debug 1` (existing) draws the hit sphere over the model to check the fit.

**Done when:** the three types draw their targets with the `colormap.png` colors (not white), the face toward
their movement; with a model file renamed, that type draws the sphere and pyramid and a warning is logged; the
flash, squash, lunge and death shrink look right on the model; shots that visibly hit the model register (the model
sits inside the hit sphere); 20 enemies on screen keep 60 FPS.

| Phase | Depends on | Visible result |
|---|---|---|
| M0 | — | none (pipeline) |
| M1 | M0 | none (engine ready) |
| M2 | M1 | the model gun |
| M3 | M0 (M1 for `alphaCutoff` in the same shader) | textured arena |
| M4 | M1 | enemy models |

M3 and M4 are independent of each other, and either can come after M2.

---

## 5. Risks

| # | Risk | Mitigation |
|---|---|---|
| R1 | A model with several `Kd` materials costs one draw per material per enemy (20 × 4 = 80) | Asset rule: one material per enemy model; measure in M4 |
| R2 | A model larger than its hit sphere looks "hit through" (or the reverse) | `FitModel(Largest, diameter)` (Q6) and the `ai_debug` sphere |
| R3 | The gun's tuned values were set on the earlier gun model | Live `gun_*` ConVars in M2 (Q3) |
| R8 | The targets are thin discs (about half as thick as they are wide), so a shot past the disc's edge seen from the side still hits the sphere | Accepted for the first models; a thicker model or a per‑type hit shape is a later change |
| R4 | Point filter without mipmaps shimmers on far walls | Accepted (PSX); a texture can ask for trilinear |
| R5 | `glClear` is macOS‑only | `#ifdef __APPLE__`; the engine is Mac‑only today |
| R6 | Licenses are not checked for the developer's textures and the Kenney pack | Kept as `TODO: check` in `CREDITS.txt`; must be confirmed before a release |
| R7 | Porting by hand drops a spike fix | The §1 tables list every fix; the spike's assessment lists what went wrong |

---

## 6. Out of scope

Skinning or skeletal animation (G5), PSX shader effects (vertex snapping, affine textures), normal or emissive maps,
a `model=` key for level props, a model or texture picker UI, hot reload of models, and platforms other than macOS.
