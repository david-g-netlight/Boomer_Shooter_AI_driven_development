# Devlog — `input`

**Phase:** 2 — Input ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `afe0ac2`
**Date:** 2026-11-06
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/input`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `5695829` | `InputManager::Poll()` | `src/engine/input/InputManager.h/.cpp`. Private ctor + deleted copy/move + `friend class Engine`, as in the reference. `Poll()` (private, called by the Engine right after `Time::BeginFrame`) stores down / pressed / released for keys `1..KEY_KB_MENU` (349 slots) and mouse buttons `0..MOUSE_BUTTON_BACK` (7 slots), plus mouse position, delta and wheel. Getters bounds‑check codes (invalid → false). Kept the reference's `IsLeft/RightMouseButtonDown`. `Engine::GetInputManager()`. |
| 2 | `50612b7` | `InputMap` | `src/engine/input/InputMap.h/.cpp`, owned by `InputManager` (`GetMap()`). `InputBinding::Key/Mouse` → named **actions** (many bindings each; `IsActionDown / Pressed / Released`). `AxisBinding::Keys(neg, pos) / MouseX / MouseY / Wheel` with a scale → named **1D axes** (sources summed). `BindAxis2D(name, x, y, clamp)` → **2D axes**, optionally clamped to unit length. `Unbind`, `Clear`, `HasAction`, `HasAxis`. Unknown names return false/0 and log one warning per name. `ScaleGame` binds Move (WASD + arrows), Look (mouse), Jump (Space), Fire (LMB), AltFire (RMB), Scroll (wheel) and draws an input panel with press counters. |
| 3 | `4646842` | Cursor lock / unlock | `InputManager::SetCursorLocked(bool)` → `DisableCursor / EnableCursor`, `IsCursorLocked()`. The mouse delta of the frame after a change is zeroed (raylib recenters the cursor and reports a fake jump). `ScaleGame`: temporary `ToggleCursor` action on **C**; locked = first‑person look (0.1°/pt via `UpdateCameraPro`, pitch clamped) + WASD fly at 5 m/s; unlocked = orbit camera. |
| 4 | *(this commit)* | — | This devlog; tracker marks Phase 2 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P2‑1 | Method names follow raylib semantics: `IsKeyDown` = held, `IsKeyPressed` = this frame, `IsKeyReleased` = this frame | The reference's `IsKeyPressed` meant "held"; matching raylib avoids confusion when reading raylib docs |
| P2‑2 | Store a per‑frame snapshot instead of calling raylib directly from game code | All systems see the same state in a frame; later the UI can consume input before the game sees it (Phase 8) |
| P2‑3 | Pressed/released come from raylib's own `IsKeyPressed/Released`, not from diffing our arrays | Same result, no duplicate bookkeeping |
| P2‑4 | `InputMap` is owned by `InputManager` and queried live (no cached values) | Always consistent with the snapshot; trivial cost for a handful of actions |
| P2‑5 | Bindings are registered by the game (`ScaleGame::BindInput`), not hard‑coded in the engine | The engine stays game‑agnostic |
| P2‑6 | 2D axes are built from two named 1D axes | One mechanism for keys, mouse and wheel; a gamepad stick can be added later as another 1D source |
| P2‑7 | `IsActionReleased` is true only when no binding of the action is still held | Releasing Space while LMB is still held for the same action must not count as released |
| P2‑8 | Unbound names warn once instead of asserting | Typos are visible in the log without crashing |
| P2‑9 | Suppress one frame of mouse delta after a lock change | Prevents the camera jumping when the cursor is captured |
| P2‑10 | Temporary `C` toggles the cursor lock | ESC is reserved for Pause; Phase 8 (`ModeMachine`) takes over cursor control |
| P2‑11 | Mouse look stays in `ScaleGame` (`UpdateCameraPro`), marked `TODO(Phase 3/10)` | `eng::Camera` arrives in Phase 3, `FirstPersonLook` in Phase 10 |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| 4 s smoke run | ✅ engine + game init logged, no `InputMap: '...' is not bound` warnings, no errors |
| Overlay shows action states; mouse look works with the cursor locked | ✅ validated by the developer |

## Notes for later phases
- Phase 3: replace the raw `Camera3D` and `UpdateCameraPro` in `ScaleGame` with `eng::Camera` (`Rotate(yaw, pitch)`).
- Phase 8: the `ModeMachine` owns cursor locking (Play = locked; Menu / Pause / Editor = unlocked); remove the `C` toggle. UI screens should block game input while open.
- Phase 10: `PlayerInput` reads the same action names (`Move`, `Look`, `Jump`, `Fire`, `AltFire`, `Scroll`); Fire presses are latched in `Update` and consumed in `FixedUpdate`.
