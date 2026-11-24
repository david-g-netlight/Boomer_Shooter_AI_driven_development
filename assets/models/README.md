# Models: Blockbench -> OBJ export rules

Source of the rules: `docs/deliverables/Rendering_OBJ_Gun_Spike_PLAN.md` §1.

| Item | Rule |
|---|---|
| Project type | *Generic Model* |
| Axes | **+Y up, barrel pointing north (-Z)**; the side the player sees on the left is -X, on top is +Y. A wrong axis is fixed live with `gun_rot`, then written back into the model |
| Origin / pivot | Anything. The code recenters on the bounds |
| Units / scale | Anything (Blockbench: 16 px = 1 block). The code scales to `GUN_LENGTH`; the load log prints the raw bounds |
| Faces | Quads are fine (raylib triangulates on load) |
| Texture | One PNG, **power of two** (32, 64 or 128 px), 16 px per block for a uniform texel size. Transparent pixels only where a hole is wanted. One texture = one material = one draw |
| Export | *File -> Export -> Export OBJ Model*: save `gun.obj` + `gun.mtl`, and the texture as `gun.png` in the **same folder** |
| `.mtl` check | `map_Kd gun.png` must be a **bare file name**, not an absolute path from your disk |
| Material color | `Kd 1 1 1` (`Kd` multiplies the texture) |
| Poly budget | <= 1 500 triangles |
| Credits | Add a line to `CREDITS.txt` (author, license). |

After a re-export: `cmake --build build --target copy_assets`, then `gun_reload` in the console (no restart).

## Layout

- `prototype/` — the developer's own work-in-progress models.
- `../textures/` — standalone textures (level geometry, tiling).
