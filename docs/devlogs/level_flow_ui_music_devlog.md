# Devlog — `level_flow_ui_music`

**Phase:** 12 — Game: level flow, UI, music ([plan §5](../ARCHITECTURE_PLAN.md), [tracker](../DEV_PROGRESS.md))
**Branched from:** `integration` @ `bf81f7e`
**Date:** 2026-11-08
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/level_flow_ui_music`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `c5f0d99` | `TriggerZone`, `WinTrigger`, `LoseTrigger`, `NextLevelTrigger` | `src/game/flow/`. Volumes that react only to the player capsule: win and lose call `GameState`, the next-level trigger publishes `NextLevelRequestedEvent`. `GameContext` gives loader-made components the `GameState`. Registered for level files. |
| 2 | `d4dbd3a` | `GameplayManager` | Load / next / reload are requests applied in `Update()` (never inside a physics callback or a UI button). Every load resets `GameState` and `PlayerState`; the game starts by itself while the engine is in Play. Start loads level 1, Restart reloads the current level. |
| 3 | `8b88a0e` | `GameUI` | "Level Complete" (Next Level / Main Menu), "You did it!" (Play Again / Main Menu) after the last level, "You fell!" (Retry / Main Menu). Input is cut while a dialog is up and returns on `GameStartedEvent`. The old "Zone reached" demo is gone. |
| 4 | `4a4babb` | `MusicManager` + credits | Menu and gameplay themes follow `ModeChangedEvent`: slow fade-in for the first track, cross-fade after; pause, resume and the editor leave the track alone. The M demo key is gone. |
| 5 | `ec234dd` | Levels 1–3 | Drafts written with the editor's serializer from a script: L1 ledge + stretch, L2 shrink the wall + pit, L3 both; each with spawn, Win zone and kill volume. The inspector got **Add Component** (any registered type). |
| 6 | *(this commit)* | — | This devlog; tracker marks Phase 12 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P12-1 | Level loads are deferred to `Application::Update()` | A trigger fires inside the physics step and a button inside the UI draw; neither may clear the level |
| P12-2 | `NextLevelTrigger` publishes an event, win / lose call `GameState` directly | The next-level request must wait for the end of the frame; the state guard already handles double triggers |
| P12-3 | The game auto-starts a load only while the engine mode is Play | Start from the menu goes through the ModeMachine hook; retry, next and restart happen in Play |
| P12-4 | Levels are script-authored drafts | The editor GUI can't be driven in the build sandbox; the files use the same serializer and open in the editor |
| P12-5 | The inspector lists every registered component type | Level authors need Win / Lose / Scalable without code |

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Triggers: crate ignored, player wins / loses / requests the next level once | ✅ |
| Manager: next, retry, win → next, last level, menu → Start | ✅ |
| Dialogs: control cut and restored, screenshot of the win dialog | ✅ |
| Music: menu → game → pause / resume (not restarted) → menu | ✅ |
| Scripted playthroughs win levels 1, 2 (with the pit jump) and 3 | ✅ |
| Menu → L1 win → L2 fall → retry → win → Menu | ✅ validated by the developer |

## Notes for later phases
- Phase 13: the HUD still shows the debug panels of the sandbox; `GunHUD` and a clean HUD replace them.
- Phase 13: audio files are placeholders (see `assets/audio/CREDITS.txt`).
