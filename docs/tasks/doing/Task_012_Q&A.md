# Task 012: Questions and answers

Plan: [`Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md`](Custom_Mesh_Rendering_ARCHITECTURE_PLAN.md).
Each question has a **default**. The plan follows the default until you answer otherwise. Write your answer in the
last column.

| # | Question | Default (what the plan does) | Answer |
|---|---|---|---|
 Q1 The spike's V3 decision **Go** was inferred from you moving on to V4. Is it right? Yes, not the standard.

 Q2 Which gun ships: the Kenney blaster (`Kd` colors) or a Blockbench gun with a PSX texture? The Kenney blaster, moved to `assets/models/gun/`

 Q3 The tuned constants (offset, rotation, length, muzzle) were set on the earlier gun. Should M2 keep spike‑style live tuning to re‑check them? Keep it.

 Q4 Keep `gun_obj 0|1` (switch between the model and the primitive gun) in Debug builds? | Yes, Debug only. The primitive gun stays in the code as the fallback anyway

 Q5 Who makes the enemy models, and are they ready before M4? I provide them in `assets/models/enemies`. The other types on the sphere and pyramid fallback. Consider: target-small is the Drifter, and target-detail is the Brute.


 Q6 How should a model fit its hit sphere? the model's **largest extent = the diameter** (corners may poke out a little)

 Q7 The hit flash on a color‑only enemy is a white silhouette (its `Kd` colors are lost for about 0.1 s). Is that fine? | Yes. Keeping the colors would need a per‑material flash or a tint uniform, which the plan does not include

 Q8 Which level textures go to `integration`? | `wall.png`, `floor.png`, `ceiling.png`. The `_test` textures and `crate.png` stay on the spike branch

 Q9 Should model hot reload (`gun_reload`, `ReloadModel`) come back as a generic Debug command such as `model_reload <path>` for art iteration? No. Out of scope; restarting the game is enough for now

 Q10 Is a `model=` key for level props (an OBJ placed in the editor) needed soon? | No. Out of scope; `MeshRenderer` already draws a model in code, only the `.lvl` key is missing

 Q11 Licenses of the Kenney pack and of your wall, floor and ceiling textures are `TODO: check`. Can you confirm them? | Ship with `TODO: check`, as for the audio; confirm before a release
 
 Q12 Phase order: M2 (gun) before M3 (levels) and M4 (enemies)? M0 → M1 → M2 → M3 → M4 is fine.

 Extra:
 Remove the days estimation from the plan.

 ---

 Applied in the plan (iteration 1): Q3 keeps the `gun_*` ConVars in Debug (M‑D13), Q5 sets the Drifter to
 `target-small` and the Brute to `target-detail` with the Stalker on the fallback (M‑D12, §3.4), Q6 uses the largest
 extent, and the day estimates are removed.

 Follow‑up questions (from reviewing `assets/models/enemies/`):

 Q13 The three `.mtl` files point to `map_Kd Textures/colormap.png`, but `assets/models/enemies/Textures/` does not exist. Can you add Kenney's `colormap.png` there? Added

 Q14 The targets are thin discs that face ±X, not −Z. Should `modelYawDeg` turn the flat side toward the player (90°, sign checked in M4)? Default: yes, a constant per type (M‑D15), with no live ConVar.

 Q15 `target-large` has no type. Should it be the Stalker instead of the fallback? Yes, it should be the Stalker.

 Q16 The `.obj` files are ignored by git on `integration` until M0 adds `!assets/models/**/*.obj`, and only the `.mtl` files show in `git status`. Should M0 commit your enemy files together with the rule? Default: yes, M0 commits the three targets (and `colormap.png` if it is there).
