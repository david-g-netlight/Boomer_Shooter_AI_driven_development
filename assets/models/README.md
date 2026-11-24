# Models

3D meshes loaded through `ResourceManager::LoadModel` (raylib's OBJ + MTL loader). Credits are in `CREDITS.txt`.

| Folder | Content |
|---|---|
| `enemies/` | Enemy targets: `target-small` (Drifter), `target-large` (Stalker), `target-detail` (Brute). Their shared `Textures/colormap.png` |
| `gun/` | The player's gun: Kenney's Blaster Kit blaster (`gun.obj`, `gun.mtl`, colors only) |

## Export rules

| Item | Rule |
|---|---|
| Format | OBJ + MTL, with at most one PNG. `map_Kd` is a path **relative to the `.mtl`** (a bare name, or a subfolder such as Kenney's `Textures/colormap.png`), never an absolute path, and the PNG must be committed with the model |
| Faces | Triangles or quads. **Export triangulated** when unsure: faces over 5 vertices are refused |
| Axes | +Y up, front toward **−Z**. Pivot and units do not matter (the engine fits the model) |
| Texture | Power of two (32, 64 or 128 px, or a shared color atlas such as Kenney's 512 px `colormap.png`), one texel size across the model, transparent pixels only for holes |
| Color‑only models | `Kd` per material. Prefer **one material per model**, because each material is one draw per instance |
| Budget | Gun ≤ 1 500 triangles; enemy a few hundred |
| Credits | One line in `CREDITS.txt` per file, with author, source and license |

Git ignores `*.obj` by default. The `.gitignore` negation `!assets/models/**/*.obj` keeps model files under
`assets/models/` tracked, so new `.obj` files there are committed like the others.
