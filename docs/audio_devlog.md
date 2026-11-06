# Devlog — `audio`

**Phase:** 4 — Audio ([plan §5](ARCHITECTURE_PLAN.md), [tracker](DEV_PROGRESS.md)). Moved before Scene/Physics by the developer (D9).
**Branched from:** `integration` @ `58be85e`
**Date:** 2026-11-06
**Status:** ✅ validated by the developer, merged into `integration`, branch renamed to `done/audio`

---

## Commits

| # | Commit | Checkbox | Description |
|---|---|---|---|
| 1 | `31c5141` | `AudioManager` | `src/engine/audio/AudioManager.h/.cpp`, owned by value in `Engine` (`GetAudio()`). `Init()` (after the window, before `Application::Init`; a missing device is logged and the game runs silently) / `Shutdown()`. **Voice pool:** each `Play(sound, volume, pitch, pan)` uses a `LoadSoundAlias` of the sound (shared samples), up to `VOICES_PER_SOUND` 4 per sound and `MAX_VOICES` 64 total; a sound with no free voice restarts its oldest; a full pool re‑points the oldest voice overall. Returns a `VoiceHandle{index, generation}` (`Stop`, `IsPlaying`, `SetVoiceVolume`, `SetVoicePan`; stale handles are ignored). `StopAll / PauseAll / ResumeAll`. **Volumes:** master (raylib), SFX bus (applied to voices), music bus. **Duck:** `Duck(bool)` fades a factor to 0.3 over 0.25 s (unscaled time). `ResourceManager::LoadSound`. Engine loop step 3 = audio update; shutdown order `ReleaseVoices → ResourceManager::UnloadAll → AudioManager::Shutdown → CloseWindow`. `tools/generate_placeholder_audio.py` (stdlib only, deterministic) → `sfx_stretch.wav`, `sfx_shrink.wav`; `assets/audio/CREDITS.txt`. Demo: LMB / RMB play stretch / shrink at Unity's pitches, wheel picks the factor, **P** = pause demo (duck + pause SFX). |
| 2 | `c62b14a` | `AudioSource` | `src/engine/audio/AudioSource.h/.cpp`, a standalone class (D10). Sound (not owned), volume, pitch, pitch variation (±, per `Play`), spatial flag, position, range (min 2 m, max 30 m default). Gain = 1 inside min, 0 beyond max, quadratic falloff between; pan = 0.8 · dot(direction, listener right). `Update()` re‑applies gain/pan while playing. `AudioManager::SetListener/GetListenerPosition/GetListenerRight`, set by the Engine from the active camera each frame. `sfx_ping.wav` (bell). Demo: one source per cube, own pitch; **K** pings a random cube; HUD shows cube, distance, gain, pan. |
| 3 | `47d1e26` | `MusicPlayer` | `src/engine/audio/MusicPlayer.h/.cpp`, owned by `AudioManager` (`GetMusic()`), updated every frame with `busGain = music bus · duck`. `Play(music, fadeSeconds, loop)` starts from the beginning and fades in; if another track plays, it fades out over the same time (cross‑fade); playing the outgoing track again swaps it back instead of restarting; playing the current track again only re‑fades it. `Stop(fade)`, `Pause/Resume`, track `SetVolume`, `GetCurrent/GetFade/GetTimePlayed`. Fully faded‑out tracks are stopped and forgotten. `Release()` runs in `ReleaseVoices()` before resources are unloaded. `ResourceManager::LoadMusic`. `music_menu.wav` (60 bpm) and `music_game.wav` (120 bpm), 16 s seamless loops. Demo: menu theme fades in over 4 s, **M** cross‑fades (2 s), **P** ducks. |
| 4 | *(this commit)* | — | This devlog; tracker marks Phase 4 done (the developer ticked **Done when**). |

---

## Decisions

| # | Decision | Why |
|---|---|---|
| P4‑1 | Voice pool built on `LoadSoundAlias` | A raylib `Sound` can only play once at a time; aliases share the sample data, so overlapping shots cost no extra memory |
| P4‑2 | Steal the oldest voice of the same sound first | Rapid fire keeps the newest shots audible; other sounds keep their voices |
| P4‑3 | `VoiceHandle` with a generation counter | A handle to a voice that was reused can't stop or change the new sound |
| P4‑4 | **Engine pan is −1 left .. +1 right**; converted to raylib's | In raylib 5.6‑dev the left channel's level equals `pan` (1 = full left), the opposite of what most code expects |
| P4‑5 | `Duck()` only affects the music bus; SFX are held with `PauseAll()` | Matches the plan (Paused: music ducked, SFX paused) while keeping both available separately |
| P4‑6 | Audio and duck use **unscaled** time | Fades must keep running while the game is paused (time scale 0 later) |
| P4‑7 | No audio device is not fatal | The game still runs (e.g. on a machine without output); calls become no‑ops |
| P4‑8 | `AudioSource` is standalone until Phase 5 (D10) | `Component` doesn't exist yet; Phase 5 makes it a component positioned by its actor |
| P4‑9 | Quadratic distance falloff, pan capped at 0.8 | Closer to perceived loudness; fully hard‑panned sounds feel unnatural on headphones |
| P4‑10 | `MusicPlayer` is owned by `AudioManager` | One place applies bus volume and duck and calls `UpdateMusicStream` every frame |
| P4‑11 | One `Music` in one place at a time; replaying the outgoing track swaps it back | A raylib music stream has one playback position; restarting it mid‑cross‑fade would click |
| P4‑12 | **Placeholder audio is synthesized** by `tools/generate_placeholder_audio.py`, not copied from the jam | The original tracks are third‑party ("Nightmares Inn" by RKVC, "Owls" by LishGrooves) with no license verified; real files can replace the placeholders under the same names |
| P4‑13 | Music as 16‑bit mono WAV at 22.05 kHz (~700 KB per 16 s loop) | No encoder dependency; `LoadMusicStream` streams WAV; small enough to commit |

---

## Verification

| Check | Result |
|---|---|
| Every commit builds (Debug), no warnings | ✅ |
| Placeholder generator is deterministic (re‑run leaves committed files unchanged) | ✅ |
| Voice pool (temporary self‑closing run): stretch ×3 overlapping + shrink | ✅ 4 voices created, 2 still active at the expected time; shutdown: aliases released → 5 resources unloaded → device closed |
| `AudioSource` gain / pan (headless, real Engine singleton, listener at origin facing −Z) | ✅ right 5 m → gain 0.756 pan +0.80; left → pan −0.80; inside min → gain 1; beyond max → 0; non‑spatial → gain 1 pan 0 |
| Music timing (temporary self‑closing run) | ✅ fade 0.25 at 1 s (4 s fade‑in); cross‑fade mid‑point 0.50; duck settles at 0.30; 8 resources unloaded before the device closed |
| Pitched SFX on key press, music fades in, pause ducks music | ✅ validated by the developer |

## Notes for later phases
- Phase 5: `AudioSource` becomes a `Component` (position from its actor's world transform, `Update()` from the component update).
- Phase 8: `ModeMachine` calls `Duck(true)` + `PauseAll()` on Pause/Editor and the reverse on Resume; remove the **P** demo key. Music themes switch on mode changes (Phase 12 `MusicManager`).
- Phase 11: beam sounds (`sfx_beam_stretch.wav`, `sfx_beam_shrink.wav`) can be added to the generator; Unity played beams at volume 0.3 and scaling at 0.25.
