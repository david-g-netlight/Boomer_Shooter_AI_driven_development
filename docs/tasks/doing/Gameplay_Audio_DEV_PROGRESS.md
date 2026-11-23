# Development Progress — Gameplay Audio

Tracks the implementation of [`Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md`](../../deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md). Phase names and **Done when** criteria match
§5 of that plan. The previous work's history stays in [`Enemy_System_v1_DEV_PROGRESS.md`](../done/Enemy_System_v1_DEV_PROGRESS.md).
History of this plan (decision log, answered questions, risks, session log): [`devlogs/Gameplay_Audio_DEV_LOG.md`](../../devlogs/Gameplay_Audio_DEV_LOG.md).

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- At the end of every work session, add a line to the **Session log** in the history file. When the plan changes, add a **Decision** (A‑Dn) there too.
- Open questions for the developer go in an **Open questions** section in this file. Once a question is answered, move it to the history file.
- Branches: one per phase from `integration`, one commit per checkbox (except **Done when**, checked by the developer); a finished branch is renamed `done/<branch>` ([`tasks/done/TASK_04.md`](../done/TASK_04.md)).
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| S1 | Assets and music | `done/audio_assets` | ✅ | 2026-11-24 | 2026-11-24 | Move the files, trim the growl, credits file, two Rod Kim tracks |
| S2 | Sound table | `done/audio_sfx_table` | ✅ | 2026-11-24 | 2026-11-24 | `SFX_TABLE`, `Play(Sfx)`, shot / enemy hits player / enemy dies |
| S3 | Player and match events | `audio_player_events` | 🟡 | 2026-11-24 | | Jump, footsteps, player dies, win, match begins |
| S4 | Cleanup | `audio_cleanup` | ⬜ | | | Delete the source folder, unused files, placeholders |

**Current focus:** S3 Player and match events.
**Branch:** `audio_player_events`

---

## Phase checklists

### Phase S1 — Assets and music ✅
- [x] Move the 10 used files from `assets/audio-to-re-organize/` to `assets/audio/music/`, `sound_fx/player/`, `sound_fx/enemy/` and `sound_fx/announcements/`, keeping the file names (§1). Leave the unused files in the source folder until S4 (+ in the same commit as the tracker's start edit, `d66f259`)
- [x] Cut `874627__duhduhduh__slow-pitch-growl.wav` (7.8 s) into two files (A‑D6, Q1/Q3 in the history file): seconds 0–2 saved as `874627__duhduhduh__slow-pitch-growl-hit.wav` (`EnemyHit`), seconds 3–7 saved over `874627__duhduhduh__slow-pitch-growl.wav` (`EnemyDeath`). Both in `sound_fx/enemy/`. Note "trimmed" for both in the credits, and that `fireexplosion-short` is itself a trimmed version of the original (`short`)
- [x] Rewrite `assets/audio/CREDITS.txt` from `docs/tasks/doing/sound_database.md`: one line per file as `"<Title>" by <Author> — <Source URL> — <License>`, with a `(trimmed)` mark where it applies. The license is `TODO: check` where it is not known (§6). Fix the database rows listed in §6 (the Gunshot link, the Valhalla author lines) (+ the old file's track names, "Nightmares Inn" and "Owls", differ from the database's Rod Kim tracks; the database wins, see the session log. The Rod Kim rows still have no track page or license, plan §6 (c), so they stay `TODO: check`. Added a database row for the hit growl, which had none)
- [x] `MusicManager`: `MENU_TRACK` and `GAME_TRACK` constants pointing at the two Rod Kim MP3s; `Init` loads them (§3) (+ the constants are in `MusicManager.h`, next to `VOLUME`, `FADE_IN` and `CROSSFADE`)
- [x] **Done when:** the main menu plays `Final_Boss_Battle_Rod_Kim` and Play plays `Duty_Calls_Rod_Kim`, with the cross‑fade as before; no "failed to load" warning in the log; Debug and Release build

### Phase S2 — Sound table ✅
- [x] `SoundFXBank.h/.cpp`: new `Sfx` list (`Shot`, `PlayerDie`, `PlayerJump`, `PlayerStep`, `EnemyHitsPlayer`, `EnemyHit`, `EnemyDeath`, `Win`, `MatchBegins`), `SfxDef` and `SFX_TABLE` (path, volume, pitchMin, pitchMax; `EnemyHit` and `EnemyDeath` each have their own growl cut), `Load` loops the table, one `Play(Sfx)` that returns the `VoiceHandle` and picks a pitch in the range; the four `PlayX()` wrappers are removed (§3, A‑D3, A‑D6) (+ the four `PlayX()` wrappers stay as one-line forwards to `Play` in this commit, so every commit builds; the call sites move and the wrappers go in the next commit. The table's values for `PlayerDie`, `PlayerJump`, `PlayerStep`, `Win` and `MatchBegins` are first guesses, tuned in S3)
- [x] Migrate the call sites: `ShotFXSpawner.cpp` → `Play(Sfx::Shot)`; `LifeSystem.cpp` → `Play(Sfx::EnemyDeath)`; `ArenaGame::OnPlayerHit` → `Play(Sfx::EnemyHitsPlayer)`; `EnemyManager.cpp` → `Play(Sfx::EnemyHit)`. Volumes and pitch ranges in the table are tuned by ear (§4) (+ `EnemyManager.cpp` calls `Play(Sfx::EnemyHit)`, `LifeSystem.cpp` `Play(Sfx::EnemyDeath)`, `ShotFXSpawner.cpp` `Play(Sfx::Shot)`, `ArenaGame::OnPlayerHit` `Play(Sfx::EnemyHitsPlayer)`; no `PlayX` name is left in `src/`)
- [x] **Done when:** the gunshot, the enemy bite on the player and an enemy death each play the clip from the database (no ping); shooting an enemy plays the short growl (0–2 s cut) and killing it plays the longer one (3–7 s cut); the three clips can be tuned only by editing `SFX_TABLE`; Debug and Release build

### Phase S3 — Player and match events 🟡
- [ ] `FirstPersonController`: play `Sfx::PlayerJump` where `ConsumeJump()` succeeds and `motor->Jump()` is called (§2)
- [ ] `FirstPersonController`: footstep loop. Keep a `VoiceHandle m_stepVoice`; while grounded and the horizontal speed is above `STEP_MIN_SPEED`, replay `Sfx::PlayerStep` when `!IsPlaying(m_stepVoice)`; otherwise `Stop` it (A‑D5, R1)
- [ ] `ArenaGame`: subscribe to `PlayerDiedEvent` (play `Sfx::PlayerDie`), `GameWonEvent` (`Sfx::Win`) and `GameStartedEvent` (`Sfx::MatchBegins`); unsubscribe in the destructor like the other tokens. Stop the footstep voice on death and on level reload (§2)
- [ ] **Done when:** all 8 events are heard (2 themes + 8 sounds): jump once per jump; footsteps play while walking on the ground and stop when standing still, in the air, on pause and on death; "match begins" plays on Start, Restart and Retry; "you win" plays on "Arena cleared"; the death sound plays when HP reaches 0; Debug and Release build

### Phase S4 — Cleanup ⬜
- [ ] Delete `assets/audio-to-re-organize/` (with the unused `761501…explosion.wav` and `583715…walking…wav`) and the placeholders `assets/audio/sfx_ping.wav`, `music_menu.wav`, `music_game.wav`. `tools/generate_placeholder_audio.py` stays (§1.1)
- [ ] Fix the docs: `README.md` and `assets/README.md` stop mentioning placeholders and list the new folders; check `tools/package_release.sh` still copies `assets/` (§4)
- [ ] **Done when:** `grep -rn "sfx_ping\|music_menu\|music_game\|audio-to-re-organize" .` finds nothing outside the docs history; `assets/audio-to-re-organize` does not exist; `assets/audio/` holds only `CREDITS.txt`, `music/` and `sound_fx/`; a clean Debug and Release build runs with no audio warning

---

## Open questions

None. The plan's Q1–Q5 are answered in `tasks/doing/TASK_08_Dicussions.md` and recorded in the history file.
