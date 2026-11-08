# Devlog — `resources_and_rendering`

**Phase:** 3 — Resources and rendering ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `b16d6de`
**Date:** 2026-11-06
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/resources_and_rendering`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `bf32e6e` | `ResourceManager` | `src/engine/resources/ResourceManager.h/.cpp`, owned by value in `Engine` (`GetResources()`). Path‑keyed caches for `Model`, `Texture2D` (mipmaps + trilinear), `Shader` (key `vs|fs`, `""` = raylib default stage), `Font` (key `path@size`). Pointers stay valid until `UnloadAll()` (`unordered_map` nodes don't move). Failed loads → `nullptr` + warning, not cached. `GetPrimitive(Primitive::Cube/Plane/Sphere/Cylinder)` generates unit meshes once. `Engine::Shutdown()` calls `UnloadAll()` before `CloseWindow()`; the destructor only warns if something is still loaded. |
| 2 | `3f53463` | `Camera`, `ActiveCamera` | `src/engine/render/Camera.h/.cpp`. `eng::Camera`: position + yaw/pitch in degrees (yaw 0 = −Z, +yaw turns right, pitch clamped ±89°), `Forward/Right/Up`, `LookAt`, `MoveLocal(forward, right, up)`, `PerspectiveParams` (fovY 60, near 0.05, far 500), `ToRaylib()` → `Camera3D`, `GetViewMatrix()`. `ActiveCamera`: non‑owning pointer, non‑copyable, `Begin3D/End3D`. The Engine owns a default camera so the active camera is never null. `ScaleGame` switched from raw `Camera3D` + `UpdateCameraPro` to `eng::Camera`. |
| 3 | `0d73fc9` | `Material` | `src/engine/render/Material.h/.cpp`. Wraps `::Material` from `LoadMaterialDefault()`; `SetShader` (nullptr = default), `SetTint`, `SetTexture` (nullptr = default white), `SetParameter/GetParameter` (float uniforms, locations cached), `Bind()`, `Get()`. Move‑only. The destructor frees only the `maps` array (`MemFree`). |
| 4 | `17f4d50` | `RenderCommand`, `RenderQueue` | `src/engine/render/RenderQueue.h/.cpp`. `RenderCommand{mesh, material, rawMaterial, transform}` (transform stored by value). `Submit(command)`, `Submit(mesh, material, transform)`, `SubmitModel(model, override, transform)` (one command per mesh, the model's own materials when no override, `model.transform` applied first). `Draw()` / `Clear()`. Engine render pass: `app->SubmitRenderables(queue)` → `activeCamera.Begin3D()` → `queue.Draw()` → `End3D()` → `DrawHUD()`; queue cleared after `EndDrawing()`. New `Application::SubmitRenderables(RenderQueue&)`. `ScaleGame`: ring of 10 cubes, one material each (hue + size). |
| 5 | `b3315ff` | `RenderTarget`, `DebugDraw` | `RenderTarget`: `RenderTexture2D` wrapper (`Create/Resize/Unload`, `Begin/End`, `DrawToScreen` with Y flip), move‑only; the destructor unloads only while `IsWindowReady()`. `DebugDraw` (Engine‑owned, `GetDebugDraw()`, on by default when `ENG_EDITOR=1`): `Line, WireBox, WireSphere, Axes, Grid` in the 3D pass, `Label` projected in 2D (skipped when behind the camera), optional duration in unscaled seconds, ignored while disabled. Added `Camera::Begin3D/End3D` (ActiveCamera delegates). `ScaleGame`: top‑down minimap rendered into a `RenderTarget` by drawing the same `RenderQueue` again with a second camera; grid, axes, wire sphere and labels; **F2** toggles debug draw. |
| 6 | `45fe8d9` | Optional lit shader | `assets/shaders/lit.vs/.fs` (GLSL 330, raylib default attribute/uniform names): Lambert sun + ambient + Blinn specular, material uniforms `emissive`, `specularStrength`. `World` gained `sunDirection`, `sunColor`, `ambientColor`; `RenderQueue::SetLighting(SceneLighting)` and `Draw(const Camera&)` upload `sunDirection / sunColor / ambientColor / viewPosition` once per custom shader per draw (locations cached by shader id). **Fix:** unset material parameters are reset to 0 per shader before binding (see P3‑9). **Fix:** assets are copied by an always‑run `copy_assets` target. `ScaleGame`: even cubes + ground lit, odd cubes unlit, cube 0 pulses `emissive`. |
| 7 | *(this commit)* | — | This devlog; tracker marks Phase 3 done; phase order changed by the developer (D9) and renumbered in the plan, tracker and code comments; D10. |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P3‑1 | `Sound` / `Music` loading moved to the Audio phase | They need `InitAudioDevice()`, owned by the audio manager |
| P3‑2 | Resources are returned as raw non‑owning pointers into the cache | Stable (`unordered_map` nodes don't move); one owner; matches the reference's ownership style |
| P3‑3 | `eng::Camera` stores yaw/pitch instead of a target point | Mouse look and `FirstPersonLook` (Phase 10) work in angles; the target is derived |
| P3‑4 | Clip planes applied per camera in `Begin3D()` via `rlSetClipPlanes` | rlgl's near/far are global; each camera (main, minimap, editor) can differ |
| P3‑5 | `eng::Material` never calls `UnloadMaterial()` | raylib's `UnloadMaterial` also unloads the shader and every texture in its maps, which are shared via the `ResourceManager`; only `maps` is ours |
| P3‑6 | Game submits in a new `Application::SubmitRenderables()` hook, not in `Update()` | Rendering must still happen while paused or in the editor (Phase 8/9), when `Update()` doesn't run. Phase 5 adds `Level::SubmitRenderables` next to it |
| P3‑7 | `RenderCommand` stores the transform by value (reference stored `mat4*`) | 64 bytes, no dangling pointer to a temporary matrix |
| P3‑8 | `RenderQueue::Draw()` is separate from `Clear()` and takes the drawing camera | The minimap redraws the same commands with another camera; specular needs that camera's position |
| P3‑9 | **Material parameters a material doesn't set are reset to 0** before it binds | GL uniforms keep their last value per program, so the first screenshot showed cube 0's `emissive` and the cubes' `specularStrength` leaking into every later material on the lit shader (washed‑out cubes, fake highlight on the minimap ground) |
| P3‑10 | `RenderTarget` / `DebugDraw` unloading guarded by `IsWindowReady()` | Objects destroyed after `CloseWindow()` (static Engine) must not touch a dead GL context |
| P3‑11 | Assets copied by a `copy_assets` custom target instead of `POST_BUILD` | `POST_BUILD` only ran when `my_app` relinked, so shader‑only edits never reached the build directory |
| P3‑12 | Ground plane drawn 1 cm below y = 0 | Avoids z‑fighting with the debug grid |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Camera math (headless test): yaw 0 → forward −Z, right +X, up +Y; yaw +90 → +X; pitch clamp 200 → 89; `LookAt` round trip; `MoveLocal(1,2,3)` → (2, 3, −1) | ✅ |
| Screenshot run #1 (temporary self‑closing patch, not committed) | Found P3‑9 and an overflowing help line; both fixed in commit 6 |
| Screenshot run #2 | ✅ lit vs unlit cubes correct, minimap ground evenly lit, `draws 11`, help line fits |
| Clean shutdown via `NeedsToBeClosed` | ✅ `ScaleGame destroyed` → `Application destroyed` → `ResourceManager: unloaded 3 resources` → `Engine shut down`, exit 0 |
| 10 cubes with different materials render through the `RenderQueue` | ✅ validated by the developer |

One screenshot attempt printed no output at all; an identical rerun worked and the cause wasn't found.

## Notes for later phases
- Phase 4 (Audio): `ResourceManager` gets `LoadSound/LoadMusic`; `UnloadAll()` must run before `CloseAudioDevice()`.
- Phase 5 (Scene): `MeshRenderer` submits through `RenderQueue::Submit/SubmitModel`; `Engine` also calls `app->GetActiveLevel()->SubmitRenderables(queue)`.
- Phase 9 (Editor): `DebugDraw` and a `RenderTarget` editor viewport are ready; picking can use `Camera::ToRaylib()` with `GetScreenToWorldRay`.
- `GetFixedAlpha()` (Phase 1) can interpolate dynamic bodies when Phase 6 adds physics.
