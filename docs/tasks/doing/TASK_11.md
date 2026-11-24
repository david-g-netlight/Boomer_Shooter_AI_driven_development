# Task 011-rendering-prototype-obj-gun-spike

## Task:
Write a prototyping plan for a tech spike: fix the rendering engine gaps found in Task 010, and add the minimum needed
to load and render an OBJ model with its texture as the player's gun.

Core idea: validate that the OBJ looks good in game. It must read as a gun, and its texture must show correctly.

# Input:
Workload assessment: `docs/deliverables/Custom_rendering_capabilities_workload_assesment.md`. Read the gaps (§2),
Option A (§3.1), Option C1 (§3.3a), the decisions (§5) and the developer's answers (§6).
Rendering classes: `src/engine/render`, `src/engine/resources/ResourceManager.h/.cpp`, `assets/shaders/lit.vs/.fs`.
Current gun: `src/game/player/GunView.h/.cpp` (view-model pass, `Application::DrawViewModel`).
Plan style example: `docs/deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md`.
Test asset: a gun made in Blockbench, exported as `.obj` + `.mtl` + `.png`, stored in `assets/models/prototype/gun/`.

# Requirements:
This is a pre-production tech spike on its own branch (e.g. `spike/obj-gun-rendering`), branched from `integration`.
It is for learning, not for shipping; keep the code simple and easy to throw away or port.
Include every rendering fix from the assessment: G1, G2, G3, G4, G6, G7 and G8. G5 (skinning) is out of scope.
Gun only: enemies keep their sphere + pyramid in this spike.
Use raylib's built-in OBJ loader. No new third-party library. It must stay OpenGL 3.3 compatible.
PSX look: point-filtered, low-res textures (developer's answer Q1). PSX shader effects (vertex snapping, affine
texture mapping) are out of scope.
The developer provides the test asset (`.obj` + `.mtl` + `.png`); the plan does not create a placeholder model.
The gun keeps its current behaviour: kick, bob, tilt, muzzle flash and the ready light.

## Criteria:
Order the phases so the OBJ gun is visible in game as early as possible. Put the fixes the gun does not need
(G6, G8: level textures) after the gun validation.
Each phase must say how the developer validates it, with a short in-game checklist (e.g. silhouette reads as a gun
at the default FOV, texture is not stretched, flipped or blurry, the model is lit by the sun, no back faces drawn over
front faces, no clipping into walls, no leak warning on shutdown).
Explain the Blockbench → OBJ export rules: units, axes, origin / pivot, scale, triangulation, texture size and path in the `.mtl`.
Explain how the model is placed in the view-model pass (offset, scale from the bounds, muzzle position).
Add a fallback: if the OBJ fails to load, the current primitive gun is drawn and a warning is logged.
Add a console command or a key to switch between the OBJ gun and the primitive gun, so both can be compared.
List the spike's risks, and what is ported to `integration` and what is thrown away when the spike ends.

# Output:
Create an .md file called `docs/deliverables/Rendering_OBJ_Gun_Spike_PLAN.md` to present the plan.
