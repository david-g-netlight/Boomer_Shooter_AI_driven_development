# Custom Rendering Capabilities — Workload Assessment

Source task: [`tasks/done/TASK_10.md`](../tasks/done/TASK_10.md).
Status: **iteration 1** — developer answers in §6. Decision: Option A, then static OBJ models from Blockbench
(Option C1), then textured levels. Options B and C2 are out of scope.
Goal: custom meshes and textures for the **gun** and the **enemies**, OpenGL compatible, a light third‑party library
for models is allowed.

**In one line:** the engine can already sample textures and load glTF/OBJ models. Nothing uses either yet, and
five small gaps (unlit model materials, the gun's no‑depth pass, model texture leaks, filtering, skinning) stand
between "it loads" and "it looks right in game". No new third‑party library is needed.

---

## 1. What exists today

### 1.1 Graphics backend

| Item | State |
|---|---|
| raylib | `5.6-dev`, static `lib/libraylib.a`, `rlgl` on **OpenGL 3.3 core** (shaders are `#version 330`) |
| Engine shader | `assets/shaders/lit.vs/.fs`: one sun (Lambert + Blinn specular) + ambient, `emissive` and `specularStrength` material floats. **It already samples `texture0`**: `albedo = texture(texture0, uv) * colDiffuse * fragColor` |
| Requirement "OpenGL compatible" | Met by every option below: all of them go through raylib / rlgl, nothing else touches the GPU |

### 1.2 Format support compiled into `lib/libraylib.a`

Checked with `nm` / `strings` on the library:

| Kind | Formats | Library inside raylib |
|---|---|---|
| Models | **`.gltf` / `.glb`**, `.obj` (+ `.mtl`), `.iqm`, `.vox`, `.m3d` | cgltf, tinyobj, m3d |
| Animations | glTF, IQM, M3D (`LoadModelAnimations`, `UpdateModelAnimation`) | same |
| Images | `.png`, `.jpg/.jpeg`, `.bmp`, `.gif`, `.qoi`, `.dds` | stb_image, qoi |
| Procedural meshes | `GenMeshCube/Sphere/HemiSphere/Cylinder/Cone/Torus/Knot/Poly/Plane/Heightmap/Cubicmap` | — |

**A third‑party model library is not needed**: raylib already ships cgltf and tinyobj.

### 1.3 Engine render classes (`src/engine/render`, `src/engine/resources`)

| Class | What it gives | Used for textures / models today? |
|---|---|---|
| `ResourceManager::LoadTexture` | path cache, mipmaps, **trilinear** filter | No caller |
| `ResourceManager::LoadModel` | path cache, `IsModelValid` check | No caller |
| `Material::SetTexture` | albedo map (`MATERIAL_MAP_ALBEDO`), `nullptr` = white 1×1 | No caller |
| `MeshRenderer(const Model*, overrideMaterial)` | one draw per model mesh, world bounds | Only primitives are used |
| `RenderQueue::SubmitModel` | one `RenderCommand` per mesh; override material **or** the model's own `rawMaterial` | No caller |
| `ResourceManager::GetPrimitive` | cube, inverted cube, plane, sphere, cylinder, pyramid (unit size, all with UVs) | Yes, everywhere |

### 1.4 How the gun and the enemies are drawn now

| Thing | File | How |
|---|---|---|
| Gun | `src/game/player/GunView.cpp:52` | Immediate mode `DrawCube` / `DrawCylinderEx` with flat colors, in its own pass (`Application::DrawViewModel`) with a separate camera. **Depth test is off**; parts are drawn far to near (painter's order). No `Material`, no lit shader |
| Enemies | `src/game/enemies/RenderSystem.cpp` | Two `RenderQueue::Submit` per enemy: unit sphere (body) + pyramid (spike), 3 lit `eng::Material` per type (`body`, `bodyFlash`, `spike`). Hit flash = swap to a white material + squash |

---

## 2. Gaps found in the review

These are the things that break or look wrong as soon as a texture or a model file is used. Each solution in §3
says which of them it must close.

| # | Gap | Effect | Fix | Size |
|---|---|---|---|---|
| G1 | A loaded `Model` keeps raylib's **default (unlit) shader** in its own materials. `SubmitModel` with no override uses them as is | Model looks flat next to the lit arena; `sunDirection` etc. are never used | On load (or via an option), set `model.materials[i].shader = lit` and keep its textures. Or always draw models with an engine `Material` override | XS |
| G2 | The gun pass draws with **depth test off** and relies on draw order | Any self‑overlapping mesh (every real model) draws its back faces over its front | Clear only the depth buffer at the start of the view‑model pass and keep depth test on. rlgl has no depth‑only clear, so call `glClear(GL_DEPTH_BUFFER_BIT)` (`<OpenGL/gl3.h>`, already linked) or draw the gun into a `RenderTarget` | S |
| G3 | `UnloadModel` frees only each material's `maps` array; raylib leaves the model's **textures** to the caller (same rule the engine follows in `Material`, devlog P3‑5) | GPU texture leak on shutdown / level reload for textured glTF files | In `ResourceManager::UnloadAll`, unload each model material's textures that are not raylib's default texture | XS |
| G4 | `LoadTexture` always uses mipmaps + **trilinear** | Low‑res boomer‑shooter textures look blurry | Add a filter option, **`TEXTURE_FILTER_POINT` by default** (PSX look, Q1), maybe `TEXTURE_WRAP_REPEAT` / `CLAMP` | XS |
| G5 | `lit.vs` has no `vertexBoneIds` / `vertexBoneWeights` / `boneMatrices` | Skeletal animation needs either CPU skinning or a skinned shader variant. **CPU skinning (`UpdateModelAnimation`) writes the shared mesh**, so all enemies of one type would share one pose | Only needed for skeletal animation (§3.3b). Rigid parts animated by code need nothing |
| G6 | `MeshRenderer::Serialize` saves only primitive names, and the inspector has no texture field | A model or texture on a level actor is not saved or editable in the editor | **In scope (Q5).** Add `texture=path` and `model=path` keys to `MeshRenderer`, and a texture / model field to its inspector | S–M |
| G7 | No `assets/textures/` or `assets/models/` folder, no credits file for art | — | Create them, same rules as `assets/audio` (README + CREDITS) | XS |
| G8 | Level geometry is **scaled unit primitives** (the arena is a unit cube scaled 16 × 6 × 41) | A texture stretches across a whole wall instead of repeating | A `tiling` material parameter (UV × scale), or world‑space (triplanar / box‑projected) UVs in a `lit` variant. Needs `TEXTURE_WRAP_REPEAT` (G4) | S–M |

Sizes: XS ≈ under 1 h, S ≈ 1–3 h.

---

## 3. Solutions, sorted by complexity

### 3.1 Option A — Textured primitives (texture sampling on raylib geometries)

**What:** keep the current shapes, add textures to them. Enemies keep sphere + pyramid, the gun is rebuilt from
primitive meshes drawn with lit `eng::Material`s (instead of immediate‑mode colored cubes).

**Work:**
1. G4 (filter option), G7 (folders).
2. Enemy materials: `set.body.SetTexture(resources.LoadTexture("assets/textures/drifter.png"))`. Hit flash: keep
   the white material, or use the existing `emissive` parameter (1 = full bright) so the texture stays visible.
3. Gun: replace `DrawCube`/`DrawCylinderEx` with `DrawMesh(primitive, material, matrix)` for each part, using the
   lit shader and textures. Close G2 at the same time (a depth‑cleared pass removes the far‑to‑near ordering rule).
4. Optionally add more primitives to `Primitive` (`GenMeshTorus`, `GenMeshHemiSphere`, `GenMeshCone` with more
   sides) for more varied silhouettes.

**Limits:** UVs are raylib's: the cube maps the whole texture on each face, the sphere has a seam and stretches at
the poles. Good for noise, metal, trim or "eyes" textures; not for painted characters.

| Complexity | Time | New code | Art workflow |
|---|---|---|---|
| Low | **0.5–1 day** | ~100–150 lines, no new class | Paint PNGs only, no 3D tool |

### 3.2 Option B — Geometry description file (shapes built from primitives) — *dropped, iteration 1*

**What:** a small text file that lists parts (primitive + local transform + material). The engine loads it into a
reusable "shape" that the gun and every enemy type draw. Shapes become data, editable without recompiling, and can
reach much richer forms than sphere + pyramid (fins, eyes, rings, barrels, sights).

<DevComment>
This alternative will requiere to have an scene were I can edit the transform of the geometries in model space, add textures and then save the configuration on a file.
Other wise, I'll be limited to whatever the AI model suggests. This accounts for enemies, gun and the level.
Would this be more work than just enabling OBJ loading and rendering?
</DevComment>

> **Answer (iteration 1):** yes, clearly more work. A shape editor on top of the file format needs a separate
> scene to edit the parts of one shape around its own origin, a **rotate** gizmo (the editor has move and scale
> only), adding / removing parts and choosing their mesh, a texture picker and material fields in the inspector,
> shape save / load and undo. The editor's picker, gizmo, inspector and save / load can be partly reused.
>
> | Path | Estimate |
> |---|---|
> | B: shape file + shape editor | 2–4 days (format) + ~4–7 days (editor) ≈ **6–11 days** |
> | C1: OBJ models made in Blockbench | **2–3 days** |
>
> Blockbench already is that editor: it places and transforms boxes, paints low‑res textures and exports OBJ. You
> build the shapes yourself, so you are not limited to AI‑made layouts. **Option B is dropped** (see §5).

**Format proposal**, same style as `.lvl` (line based, `key=value`, `end`):

```
shape 1
material "hull"  shader=lit  tint=180,60,60,255  texture=assets/textures/hull.png  filter=point  specular=0.3
material "glow"  shader=lit  tint=255,200,80,255 emissive=1
part "body"   mesh=sphere   material=hull  pos=0 0 0        rot=0 0 0   scale=1 1 1
part "spike"  mesh=pyramid  material=hull  pos=0 0 -0.5     rot=-90 0 0 scale=0.6 0.9 0.6
part "eye"    mesh=sphere   material=glow  pos=0 0.15 -0.42 rot=0 0 0   scale=0.2 0.2 0.2
end
```

**Work:**
1. Option A first (textures, filter, G2).
2. `eng::Shape` resource: `std::vector<Part{ const Mesh*, const Material*, Matrix local, std::string name }>`, owning
   its `Material`s. `ResourceManager::LoadShape(path)` with the same cache rules as the other resources.
3. `Shape::Submit(RenderQueue&, const Matrix& world)` and `Shape::Draw(const Matrix& world)` (for the gun pass).
4. Named parts so code can still animate them: `FindPart("barrel")`, a per‑draw local override (gun kick, enemy
   spike lunge, muzzle tip flash color).
5. Enemy `RenderSystem` and `GunView` read their shapes; the archetype table names a shape file per type.
6. Optional: hot reload in editor builds (re‑read on file change), a `shape_reload` console command.
7. Optional: **bake** a shape into one `Mesh` (merge vertex arrays, per‑vertex color instead of per‑part tint) to
   go from ~3–6 draws per enemy to 1. Not needed at 20 enemies.

| Complexity | Time | New code | Art workflow |
|---|---|---|---|
| Medium | **2–4 days** (A included; +1 day with baking and hot reload) | ~400–600 lines: parser, `Shape`, game wiring | Text editor + PNGs; no 3D tool, but layout is done by numbers |

**Trade‑off:** it is a home‑made format, so it needs a parser, error messages and docs, and building a complex
shape by typing numbers is slow. It shines if the developer wants "code art" without Blender.

### 3.3 Option C — Low‑poly 3D model files (glTF / OBJ)

**What:** author the gun and the enemies in Blender (or any tool), export **`.glb`**, load them with raylib's
built‑in loader through `ResourceManager::LoadModel`. Textures come embedded in the `.glb` or as PNGs next to it.

**Format choice (iteration 1):** **OBJ** (`.obj` + `.mtl` + `.png`), exported from **Blockbench** (Q2, Q4).
Animation is transform‑only (Q3), so OBJ's lack of skinning costs nothing. raylib's tinyobj loader reads the
albedo texture from the `.mtl` (`map_Kd`). Blockbench uses its own units, so the size always comes from the model's
bounds in code (step 4). glTF (`.glb`) stays available from Blockbench if skinning is ever needed.
The text below says `.glb` in places; read it as "the model file", OBJ in this plan.

#### 3.3a Static models (rigid, animated by code)

The enemies' current motion (float, slerped rotation, lunge, squash, spawn scale‑in) is all **transform**
animation, so it works unchanged on a rigid model. Same for the gun (kick, bob, tilt).

**Work:**
1. G1 (lit shader on model materials), G2 (gun depth pass), G3 (texture unload), G4, G7.
2. Enemies: `RenderSystem` submits `queue.SubmitModel(*model, overrideOrNull, world)` instead of sphere + pyramid.
   Hit flash: an override `Material` (white, or the same texture with `emissive=1`) for the flash frames.
   Spike as a separate part: either a second small model, or one model with two meshes and the spike mesh index
   kept in the archetype.
3. Gun: `GunView` loads `assets/models/gun.glb` and draws its meshes with `DrawMesh` in the existing pass
   (or through a small `RenderQueue` drawn with the gun camera). The muzzle tip glow can stay a primitive drawn at
   a muzzle offset kept in code.
4. Size and axes: Blender export with **+Y up**, **−Z forward**, meters, "apply transforms". Scale the model from
   its bounds to `bodyRadius` in code, so the archetype table stays the source of the size.
5. Asset rules in `assets/README.md`: poly budget (e.g. enemy ≤ 1–2 k triangles, gun ≤ 3 k), texture size (64–256 px),
   naming, credits.

**Limits:** raylib flattens the glTF node tree into a mesh list and **does not keep node names or extras**, so
sockets like "muzzle" or "eye" must be offsets in code (or a bone, see 3.3b).

| Complexity | Time | New code | Art workflow |
|---|---|---|---|
| Medium | **2–3 days** | ~200–300 lines (mostly game wiring) + G1–G4 | Blender (free) → `.glb`; best visual ceiling |

#### 3.3b Skeletal animation (optional, later) — *out of scope, Q3*

Only if the enemies need real deformation (wings, jaws, tentacles) or the gun needs animated reload parts.

**Work:**
1. `LoadModelAnimations` per model, an animation component (clip, time, loop) in the enemy ECS.
2. Close G5 with **GPU skinning**: add `vertexBoneIds`, `vertexBoneWeights` and `boneMatrices[MAX_BONES]` to a
   `lit_skinned.vs` variant; upload each enemy's bone matrices before its draw. This needs a per‑command bone
   array in `RenderCommand` (today a command is mesh + material + transform only).
3. CPU skinning (`UpdateModelAnimation`) is simpler but writes the shared mesh: one pose per **model**, not per
   enemy. Acceptable only if all enemies of a type may animate in sync, or each enemy gets its own mesh copy.

| Complexity | Time | New code | Art workflow |
|---|---|---|---|
| High | **+3–5 days** on top of 3.3a | shader variant, `RenderCommand` change, ECS animation system | Rigging + animation in Blender |

### 3.4 Third‑party libraries considered

| Library | What for | Verdict |
|---|---|---|
| raylib built‑ins (cgltf, tinyobj, m3d, stb_image) | glTF/OBJ/VOX/IQM, PNG/JPG | **Use these.** Already linked, OpenGL path already done |
| cgltf used directly | Node names, extras, cameras, sockets | Only if sockets become a real need. It is already inside `libraylib.a`: include `cgltf.h` **without** `CGLTF_IMPLEMENTATION`, with the exact header version raylib was built with, or link errors / ABI mismatches follow |
| ufbx | FBX source files | Only if art arrives as FBX. Single file, light, MIT. Converting FBX → glTF in Blender is simpler |
| tinygltf | glTF with a JSON object model | No: heavier than cgltf and duplicates it |
| Assimp | 40+ formats | No: large build, many dependencies, overkill for a few low‑poly models |

---

## 4. Comparison

| | A. Textured primitives | B. Shape file from primitives | C1. Static `.glb` models | C2. Skinned `.glb` models |
|---|---|---|---|---|
| Complexity | Low | Medium | Medium | High |
| Time | 0.5–1 day | 2–4 days (6–11 with a shape editor) | 2–3 days | 5–8 days total |
| Decision (iteration 1) | **Yes, first** | Dropped | **Yes, OBJ** | Out of scope |
| New third‑party code | None | None | None | None |
| Visual ceiling | Low (raylib UVs) | Medium (composed silhouettes) | High | Highest |
| Tool needed | Image editor | Text editor + image editor | Blockbench (or Blender) | Blender (rig + animate) |
| Iteration speed | Recompile | Edit file (hot reload possible) | Re‑export file | Re‑export file |
| Draws per enemy | 2 | 3–6 (1 if baked) | 1–2 | 1–2 + bone upload |
| Gaps to close | G2, G4, G7 | G2, G4, G7 | G1, G2, G3, G4, G7 | all of C1 + G5 |
| Reuses for level props | Yes (tint/texture keys in `.lvl`) | Yes, with a `shape=` key (G6) | Yes, with a `model=` key (G6) | Rarely needed |

---

## 5. Recommendation (iteration 1)

1. **Option A first** (≈ 1 day). It closes G2, G4 and G7, which the model path needs anyway, and gives
   point‑filtered, lit, textured enemies and gun right away.
2. **Then Option C1 with OBJ from Blockbench** (≈ 2–3 days) for the gun and the three enemy types. The current
   code‑driven animation keeps working (Q3).
3. **Then textured levels** (≈ 1.5–2 days): G6 (save / inspector) and G8 (tiling), so walls, floors and props get
   textures in the editor (Q5).
4. **Dropped:** Option B (a shape editor costs 6–11 days and Blockbench already does it, see §3.2).
   **Out of scope:** Option C2 (transform animation is enough, Q3).
5. **Optional, PSX polish** (≈ 0.5 day): vertex snapping and affine (warped) texture mapping in a `lit` variant,
   lower internal resolution through the existing `RenderTarget`.

Total for steps 1–3: **about 4.5–6 days**.

A possible phase list for a later architecture plan:

| Phase | Content | Done when |
|---|---|---|
| R0 | G4 filter option (point by default, wrap mode), G7 folders + README/CREDITS | A point‑filtered PNG loads and shows on a cube in the arena |
| R1 | G2 gun pass with depth (depth clear), gun parts as `DrawMesh` + lit materials | The gun looks the same as today, then lit and textured |
| R2 | Enemy textures (Option A), flash through `emissive` | Each type has its texture; the flash still reads |
| R3 | G1 + G3 in `ResourceManager`, OBJ gun from Blockbench | The gun model replaces the primitives, lit, no leak warning on shutdown |
| R4 | OBJ enemies, size from bounds, spike/eye offsets in the archetype table | The three types use models; hit test and sizes unchanged |
| R5 | G8 tiling, G6 `texture=` / `model=` keys + inspector fields | The arena walls show a repeating texture; a textured prop and a model prop survive save / load |
| R6 *(optional)* | PSX shader effects | Toggle on / off from the console |

---

## 6. Questions for the developer — answered (iteration 1)

| # | Question | Answer | What changed in this assessment |
|---|---|---|---|
| Q1 | Look: pixel‑art textures (point filter, 32–128 px) or smooth (trilinear)? | PSX aesthetic, so low res is fine. | G4: point filter by default; optional PSX shader phase R6 |
| Q2 | Will the art be made in Blender (or another 3D tool), or should it stay "code/text art"? | On Blockbench if I decide to go after the Option C | §3.3 format choice; Option B dropped (§3.2 answer) |
| Q3 | Do the enemies need skeletal animation in the next milestone, or is transform animation enough? | Transform animation is perfect. | C2 out of scope; G5 not needed |
| Q4 | Are there existing assets (store packs, FBX, VOX)? Which license? | OBJ | OBJ is the model format; no new third‑party library |
| Q5 | Should level props (crates, pillars) also use models/textures in the editor? | Yes, I will require to add texture to the level. | G6 in scope, new G8 (tiling), phase R5 |
