# Development Log — Gameplay Audio

Tracker: [`tasks/doing/Gameplay_Audio_DEV_PROGRESS.md`](../tasks/doing/Gameplay_Audio_DEV_PROGRESS.md). Plan: [`deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md`](../deliverables/Boomer_Shooter_Gameplay_Audio_ARCHITECTURE_PLAN.md).

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

## Questions for the developer

Answers are in `tasks/doing/TASK_08_Dicussions.md`.

| # | Question | Answer |
|---|---|---|
| Q1 | ~~Remove `Sfx::EnemyHit` or reuse the gunshot?~~ | **Answered:** reuse the growl for the hit sound, first 2 seconds (A‑D6). Clarified: two files, hit = 0–2 s, death = 3–7 s |
| Q2 | ~~Console command `sfx <name>`?~~ | **Answered:** no |
| Q3 | ~~Trim the 7.8 s growl?~~ | **Answered:** yes, in S1, mark "trimmed" in the credits, and say that `fireexplosion-short` is trimmed (hence `short`). Clarified: hit = seconds 0–2, death = seconds 3–7 of the growl |
| Q4 | ~~Credits screen in this task?~~ | **Answered:** `CREDITS.txt` now, screen in a follow‑up |
| Q5 | ~~"Match begins" on every wave?~~ | **Answered:** no, once per match |
| — | Python placeholder script | **Answered:** do not delete it (A‑D8) |

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
| 2026-11-23 | — | Developer's answers read; `EnemyHit` kept (shares the growl), growl cut into two files (0–2 s hit, 3–7 s death); plan, tracker and log updated | Developer gives the go for S1 | — |
| 2026-11-23 | — | Tracker written for the Gameplay Audio plan | Developer reviews, gives the go for S1 | — |
