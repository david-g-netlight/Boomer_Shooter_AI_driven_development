# Development Log — Gameplay Audio

Tracker: [`tasks/done/Gameplay_Audio_DEV_PROGRESS.md`](../tasks/done/Gameplay_Audio_DEV_PROGRESS.md). Plan: [`deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md`](../deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md).

## Decision log

| # | Date | Decision | Plan section |
|---|---|---|---|
| A‑D1 | 2026-11-23 | Engine audio is unchanged (`AudioManager`, `MusicPlayer`, `ResourceManager`) | §0 |
| A‑D2 | 2026-11-23 | Sound characteristics are authored in one table, `SFX_TABLE` in `SoundFXBank.cpp`; music in constants in `MusicManager` | §0, §3 |
| A‑D3 | 2026-11-23 | One `Play(Sfx)`; the four `PlayX()` wrappers go | §0 |
| A‑D4 | 2026-11-23 | Triggers use existing events or the code that is already there; no new event type | §0, §2 |
| A‑D5 | 2026-11-23 | Footsteps are a loop the game restarts; no loop flag in the engine | §0 |
| A‑D6 | 2026-11-23 | ~~`Sfx::EnemyHit` removed (no asset)~~ Revised by the developer (Q1): `EnemyHit` stays and has its own file: growl seconds 0–2 (`EnemyDeath` uses seconds 3–7) | §0 |
| A‑D7 | 2026-11-23 | `CREDITS.txt` rewritten from the database; the in‑game Credits screen is a follow‑up | §0, §6 |
| A‑D8 | 2026-11-23 | ~~`tools/generate_placeholder_audio.py` is deleted~~ Revised by the developer's edit of the plan: the script stays | §1.1, §4 |
| A‑D9 | 2026-11-24 | The S4 grep check skips `tools/` (answer to Q6). The placeholder script and its names stay | §5 (S4) |

## Questions for the developer

Answers are in `tasks/done/TASK_08_Dicussions.md`.

| # | Question | Answer |
|---|---|---|
| Q1 | ~~Remove `Sfx::EnemyHit` or reuse the gunshot?~~ | **Answered:** reuse the growl for the hit sound, first 2 seconds (A‑D6). Clarified: two files, hit = 0–2 s, death = 3–7 s |
| Q2 | ~~Console command `sfx <name>`?~~ | **Answered:** no |
| Q3 | ~~Trim the 7.8 s growl?~~ | **Answered:** yes, in S1, mark "trimmed" in the credits, and say that `fireexplosion-short` is trimmed (hence `short`). Clarified: hit = seconds 0–2, death = seconds 3–7 of the growl |
| Q4 | ~~Credits screen in this task?~~ | **Answered:** `CREDITS.txt` now, screen in a follow‑up |
| Q5 | ~~"Match begins" on every wave?~~ | **Answered:** no, once per match |
| — | Python placeholder script | **Answered:** do not delete it (A‑D8) |
| Q6 | ~~Grep check hits `tools/generate_placeholder_audio.py`~~ | **Answered:** skip `tools/` in the grep (A‑D9) |

## Known issues and risks being watched

| Issue | Phase | Status | Notes |
|---|---|---|---|
| R1 Footstep loop gaps or double voices | S3 | open | Replay only when `!IsPlaying`; trim the clip at a footstep boundary if there is a click |
| R2 The loop file is named *running* but is the "walks" sound | S3 | open | Tune `pitchMin/Max` in the table |
| R3 Whole OGG / WAV decoded into memory | S1 | accepted | The growl is trimmed in S1; the gunshot source used is already the 0.66 s `-short` clip |
| R4 Announcer clips overlap music and the death sound | S3 | open | Tune `volume` in the table |
| R5 `MatchBegins` plays on every restart and retry | S3 | accepted | Intended |
| R6 Licenses are not in the database; Rod Kim's terms are unverified | S1 | open | `TODO: check` in `CREDITS.txt`; check every license before release; replace any CC BY‑NC sound if the game is sold |
| R7 Removing the placeholders breaks references | S4 | open | Checked by the `grep` in the S4 Done when |

## Session log

| Date | Phase | What was done | Next step | Blockers |
|---|---|---|---|---|
| 2026-11-24 | S3 | Developer validated S3; merged into `integration`, branch renamed `done/audio_player_events` | Start S4 (Cleanup) | — |
| 2026-11-24 | S4 | Developer validated S4; merged into `integration`, branch renamed `done/audio_cleanup`. All phases S1–S4 done | Developer decides on wrap-up (tracker to `tasks/done/`) | — |
| 2026-11-24 | S4 | Q6 answered: the grep skips `tools/` (A‑D9). With that scope the grep finds nothing | Developer runs the S4 **Done when** check | — |
| 2026-11-24 | S4 | Deleted `assets/audio-to-re-organize/` (incl. the 2 unused files), `sfx_ping.wav`, `music_menu.wav`, `music_game.wav` (`6f27835`). README and `assets/README.md` describe the real files (`edeeef6`). Clean Debug and Release builds: 0 errors, 0 warnings. Both apps ran 8 s: 36 loads, 0 failed, 0 WARN. Grep check: outside `docs/`, only `tools/generate_placeholder_audio.py` matches (open question Q6) | Developer decides Q6, then runs the S4 **Done when** check | Q6 |
| 2026-11-24 | S3 | Jump sound (`fcea387`); footstep loop with `StopFootsteps` (`e11239c`); arena plays Player dies, Win and Match begins, footsteps stop on death and on level load (`099da6c`). Clean Debug and Release builds: 0 errors, 0 warnings. Each app ran 8 s: nine sound files loaded, 0 failed, 0 warnings. **Not run:** hearing the sounds, the footstep rhythm, and the jump timing (need a person). Pause and death are checked by reading the code only | Developer runs the S3 **Done when** check | — |
| 2026-11-24 | S2 | Developer validated S2; merged into `integration`, branch renamed `done/audio_sfx_table` | Start S3 (Player and match events) | — |
| 2026-11-24 | S2 | Table and `Play(Sfx)` (`2b0804b`); call sites moved, wrappers removed (`6ad9c7e`). Clean Debug and Release builds: 0 errors, 0 warnings. Each app ran 8 s: all nine sound files loaded, arena 0 warnings. **Not run:** hearing the clips (needs a person). The pitch and volume of the five sounds not played yet are first guesses | Developer runs the S2 **Done when** check | — |
| 2026-11-24 | S1 | Developer validated S1; merged into `integration`, branch renamed `done/audio_assets` | Start S2 (Sound table) | — |
| 2026-11-24 | S1 | Developer's staged clean-up committed as `f940bb0`. Branch `audio_assets` made from `integration`. Used files moved (`d66f259`); growl cut to `…growl-hit.wav` (0–2 s) and `…growl.wav` (3–7 s) (`8c96c4b`); `CREDITS.txt` rewritten and database rows fixed (`14a2a41`); `MusicManager` loads the MP3s (`d851210`). Clean Debug and Release builds (`build/check-debug`, `build/check-release`): 0 errors, 0 warnings. Each app ran 8 s: both MP3s "loaded successfully", arena 13 actors, 0 warnings. **Not run:** hearing the tracks, the cross-fade, and the level-start music (need a person). The old `CREDITS.txt` track names ("Nightmares Inn", "Owls") differ from the database; the database was used | Developer runs the S1 **Done when** check | — |
| 2026-11-23 | — | Developer's answers read; `EnemyHit` kept (shares the growl), growl cut into two files (0–2 s hit, 3–7 s death); plan, tracker and log updated | Developer gives the go for S1 | — |
| 2026-11-23 | — | Tracker written for the Gameplay Audio plan | Developer reviews, gives the go for S1 | — |
