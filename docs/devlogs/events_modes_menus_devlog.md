# Devlog — `events_modes_menus`

**Phase:** 8 — Events, modes, menus ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `86e7b55`
**Date:** 2026-11-07 → 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/events_modes_menus`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `ba8a2d8` | `EventBus` | `src/engine/core/EventBus.h/.cpp`: typed events, queued and dispatched at the end of the frame (after render, before `FlushDestroyed`). Owned by the Engine (`GetEvents()`). |
| 2 | `711af9a` | `ModeMachine` | `src/engine/modes/`: MainMenu / Play / Paused / Editor. Requests are validated at once and applied at the start of the next frame. On every transition: Application hook, cursor lock, audio duck, mode screen, `ModeChangedEvent`. ESC pauses / resumes, TAB toggles the editor (editor builds). `Application::GetStartMode`; the old C / P demo keys were removed. |
| 3 | `e336e1b` | `UIStack`, `UIScreen`, `UIStyle`, `RayGuiImpl.cpp` | `src/engine/ui/`: only the top screen is interactive, push / pop requested while drawing are deferred, an opaque screen cuts off those below. Single translation unit for the raygui implementation. |
| 4 | `ab5265e` | `MainMenuScreen`, `PauseMenuScreen`, `MessageScreen` | The ModeMachine shows each mode's screen (editor gets a placeholder banner). Restart rebuilds the sandbox. The sandbox starts in the main menu; an EventBus → dialog demo. |
| 5 | `ad72ca0` | Auto‑pause on focus loss | `ModeMachine::HandleFocus(bool)` pauses Play on the focus‑lost edge only; it never resumes by itself and skips the pause if a request is already pending. |
| 6 | `a3a162f` | Physics not stepped outside Play | Engine advances the fixed accumulator, `Level::Update` and particles only while `IsSimulating()`. `Application::Update` still runs (menu backdrop orbit). The sandbox ignores pellets, aim, beams and SFX clicks outside Play. |
| 7 | *(this commit)* | — | This devlog; tracker marks Phase 8 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P8‑1 | Mode requests are deferred to the start of the next frame | A menu button can request a change while its own screen is being drawn |
| P8‑2 | Events are queued and dispatched at the end of the frame | Handlers never run in the middle of an update; actors marked this frame are still readable |
| P8‑3 | UI uses unscaled time | Menus keep working while the game is frozen |
| P8‑4 | `Application::Update` is not frozen outside Play | The game decides what runs in menus (camera orbit, editor camera); the engine freezes only what it owns (physics, level, particles) |
| P8‑5 | Focus loss pauses; focus gain does not resume | Avoids dropping the player back into the game unprepared |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Smoke run after commit 6: starts in MainMenu, no errors | ✅ |
| Menu → Start → Play → ESC (frozen, crates mid‑air) → Resume → Menu → Quit | ✅ validated by the developer |

## Notes for later phases
- Phase 9: the editor runs in `EngineMode::Editor`, where level and physics are frozen; Reset must restore positions **and** velocities.
- Phase 10: game input code must check the mode (as `ScaleGame` does) since `Application::Update` runs in every mode.
