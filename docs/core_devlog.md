# Devlog — `core`

**Phase:** 1 — Core: Engine, Application, Time, Window ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md))
**Branched from:** `integration` @ `b012765`
**Date:** 2026-11-06
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/core`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `de61528` | `eng::Application` | Abstract interface: pure `Init / Update / Destroy`; optional `FixedUpdate(fixedDt)` and `DrawHUD()`; 7 mode hooks (`OnStartRequested, OnPause, OnResume, OnEnterEditor, OnExitEditor, OnReturnToMainMenu, OnRestartRequested`) for Phase 8; `GetTitle()` (defaults to the engine name); `GetActiveLevel()` (forward‑declared `Level`, Phase 4); `SetNeedsToBeClosed / NeedsToBeClosed` as in the reference. |
| 2 | `55e2f4a` | `eng::Engine` singleton | Meyer's singleton with private ctor/dtor and deleted copy/move, like the reference. `SetApplication(Application*)` takes ownership, deletes the previous app, and ignores re‑setting the same pointer (no self‑delete). `Init(w, h)` validates the app and size, opens the window, calls `Application::Init()`. `Run()` is the loop. `Shutdown()` is idempotent and calls `Destroy()` only if `Init()` succeeded, then `CloseWindow()`. `~Engine` deletes the app. `DEFALT_` typo from the reference fixed to `DEFAULT_WINDOW_WIDTH/HEIGHT`. |
| 3 | `f2c1244` | Window flags, exit key, cwd | `SetConfigFlags(MSAA_4X | VSYNC | WINDOW_RESIZABLE | WINDOW_HIGHDPI)` before `InitWindow`; `SetExitKey(KEY_NULL)`; `ChangeDirectory(GetApplicationDirectory())`; `SetTargetFPS(60)`. |
| 4 | `83e329e` | `Time`, `WindowInfo`, `World`, `Log` | **Time**: raw dt clamped to `MAX_DELTA_TIME` 0.1 s, time scale, scaled total time, frame and fixed‑step counters, accumulator with `AdvanceAccumulator()` (separate, so pause/editor can skip it) + `ConsumeFixedStep()` at `FIXED_DELTA_TIME` 1/50 s, at most `MAX_FIXED_STEPS_PER_FRAME` 5 (backlog dropped), `GetFixedAlpha()` for later interpolation. No raylib dependency. **WindowInfo**: screen (points) vs render (pixels) size, DPI scale, `wasResized`, `isFocused`, `Refresh()` each frame. **World**: up/right/forward, gravity −9.81, sky color. **Log**: `Debug/Info/Warning/Error` printf‑style with compile‑time format checks, forwarded to `TraceLog` with an `ENG:` prefix; `SetLevel`. Engine loop: time → window refresh → fixed steps → `Update` → `BeginDrawing / Clear(sky) / DrawHUD / EndDrawing`. Engine getters `GetTime / GetWindow / GetWorld`. |
| 5 | `3fa414e` | `main.cpp` + temporary `ScaleGame` | `main.cpp` reduced to the reference pattern (`GetInstance → SetApplication(new ScaleGame) → Init → Run → Shutdown`), no `using namespace eng`. `src/game/ScaleGame.h/.cpp`: orbiting `Camera3D` (`CAMERA_ORBITAL`), red cube + grid, overlay with engine name/version, editor flag, FPS, dt, fixed steps/s, screen/render size, DPI. |
| 6 | *(this commit)* | — | This devlog; tracker marks Phase 1 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P1‑1 | Keep a raw owning `Application*` (not `std::unique_ptr`) | Mirrors the reference's ownership lesson; ownership is limited to `SetApplication` and `~Engine` |
| P1‑2 | `Destroy()` only runs if `Init()` returned true | An app that failed `Init()` cleans up after itself; avoids destroying half‑built state |
| P1‑3 | `Shutdown()` is idempotent; the window is closed in `Shutdown()`, the app is deleted in `~Engine` | Same split as the reference ("logical shutdown" vs memory) and safe if `main` calls it after a failed `Init()` |
| P1‑4 | **World forward is −Z** (reference used +Z) | Right‑handed Y‑up (raylib, OpenGL, Bullet) with +X right means forward is −Z; +Z would mix handedness |
| P1‑5 | `InitAudioDevice()` deferred to Phase 6 | The plan text listed it in Phase 1, but the Phase 1 checklist doesn't; `AudioManager` (Phase 6) will own the device's lifetime |
| P1‑6 | `Time` has no raylib calls; the Engine passes `GetFrameTime()` | Fixed‑step logic can be tested without a window |
| P1‑7 | Accumulation is a separate call (`AdvanceAccumulator`) | Phase 8 skips it while Paused / Menu / Editor without touching `Time` |
| P1‑8 | Max 5 fixed steps per frame, extra backlog dropped | With the 0.1 s dt clamp that's exactly enough; avoids a "spiral of death" after hitches |
| P1‑9 | `Log` forwards to `TraceLog` | One output stream and one log level for engine and raylib messages |
| P1‑10 | Phase 1 game draws its 3D scene inside `DrawHUD()` (`TODO(Phase 3)`) | No engine 3D pass until the `RenderQueue` exists; avoids adding a temporary virtual to `Application` |
| P1‑11 | Camera orbits automatically | No input module until Phase 2 |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug) | ✅; final build has **no warnings** (the old example's `-Wmissing-braces` is gone) |
| `Time` unit check (scratch test, `Time.cpp` only) | ✅ 60 frames @ 60 FPS → 50 fixed steps; 5 s hitch → dt 0.100 → 5 steps; time scale 0 → 0 steps; no `AdvanceAccumulator` → 0 steps |
| 4 s smoke run launched from `/` | ✅ cwd switched to the executable directory; HighDPI active (screen 1280×720, render 2560×1440); engine + game init logged; no warnings or errors |
| Window opens via `Engine::Run`, cube renders, clean shutdown (close button → `ScaleGame destroyed`, `Application destroyed`, `Engine shut down`) | ✅ validated by the developer |

## Notes for later phases
- Phase 2: `InputManager` is owned by value in `Engine` with a private ctor + `friend class Engine`; `Poll()` goes at the top of the loop, right after `Time::BeginFrame`.
- Phase 3: move the 3D pass out of `ScaleGame::DrawHUD()` into `Engine::Run()` and replace the raw `Camera3D` with `eng::Camera`.
- Phase 8: call `AdvanceAccumulator()` only in Play mode; use `WindowInfo::isFocused` for auto‑pause.
