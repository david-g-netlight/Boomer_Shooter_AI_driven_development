# Architecture Plan — Gameplay Audio (sound FX and music)

Builds on [`Boomer_Shooter_ARCHITECTURE_PLAN.md`](Boomer_Shooter_ARCHITECTURE_PLAN.md) and
[`Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN.md`](Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN.md).
Source task: [`tasks/done/TASK_08.md`](../tasks/done/TASK_08.md). Asset list and authors: [`tasks/done/sound_database.md`](../tasks/done/sound_database.md).

**Goal:** replace the placeholder audio (one synthesized ping, two synthesized themes) with the real files of the
sound database, and play them on 8 gameplay events plus 2 music themes. The engine's audio layer does not change.
Only the game's `SoundFXBank`, a few call sites and the asset folders change.

**Status:** draft for the developer's review. Nothing is implemented, and no file has been moved or deleted yet.

---

## 0. Decisions taken (to review)

| # | Decision | Choice | Why |
|---|---|---|---|
| A‑D1 | Engine changes | **None.** `AudioManager`, `MusicPlayer`, `ResourceManager` stay as they are | They already do voices, pitch, volume, cross‑fade, ducking. The raylib build has the OGG (`stb_vorbis`) and MP3 (`dr_mp3`) decoders (checked in `libraylib.a`) |
| A‑D2 | Where sound characteristics are authored | **One table in `SoundFXBank.cpp`** (`SFX_TABLE`): file, volume, pitch range, per event. Music: two path constants and `VOLUME` / `FADE_IN` / `CROSSFADE` in `MusicManager.h` (§3) | One place to tune by ear. No new file format and no loader to write |
| A‑D3 | Bank API | `Play(Sfx)` reads the table. The four wrappers (`PlayShot`, `PlayEnemyHit`, `PlayEnemyDeath`, `PlayPlayerHurt`) go away, because their volume and pitch move into the table | Less code. Four call sites change from `PlayX()` to `Play(Sfx::X)` |
| A‑D4 | Who triggers each sound | Existing events where they exist; a direct `Play(...)` call where the code already is, the same pattern as `ShotFXSpawner` and `LifeSystem` (§2) | No new event types |
| A‑D5 | Walking | A **loop that the game restarts**: while the player is grounded and moving, replay the clip when its voice ends. `AudioManager` has no loop flag and none is added | Keeps the engine untouched. The gap on restart is below one fixed step if the clip is trimmed tight |
| A‑D6 | Shot‑hit sound (`Sfx::EnemyHit`) | **Kept, with its own file cut from the same growl.** `Sfx::EnemyHit` plays seconds **0–2** of the growl, `Sfx::EnemyDeath` plays seconds **3–7**: two files, two events. The call in `EnemyManager.cpp` stays | Developer answer (TASK_08 discussion, R1 and R3, clarified). Pitch of each is a table value, tuned by ear |
| A‑D7 | Credits | `assets/audio/CREDITS.txt` is rewritten from the database, and an in‑game Credits screen is recommended (§6) | Answers the attribution question |

---

## 1. Asset organisation

Target layout (from the `path` column of the database; the extension is the one of the source file):

```
assets/audio/
├── CREDITS.txt                                           (rewritten, §6)
├── music/
│   ├── Final_Boss_Battle_Rod_Kim.mp3                     Music Menu
│   └── Duty_Calls_Rod_Kim.mp3                            Music Game
└── sound_fx/
    ├── player/
    │   ├── 761501__valhallaproject__energy-pistol-fireexplosion-short.wav   Gunshot
    │   ├── 469567__far_box_creature__die8.wav                               Player dies
    │   ├── 386486__iimck__jump.wav                                          Player jumps
    │   └── 583714__dexd73__running-on-metal-loop.wav                        Player walks
    ├── enemy/
    │   ├── 711986__gizemhozdemir__hit-man-with-noise.wav                    Enemy deals damage to player
    │   ├── 874627__duhduhduh__slow-pitch-growl.wav                          Enemy dies (growl 3–7 s)
    │   └── 874627__duhduhduh__slow-pitch-growl-hit.wav                      Shot hits enemy (growl 0–2 s)
    └── announcements/
        ├── 767627__valhallaproject__announcer-you-win.ogg                   Win announcement
        └── 767626__valhallaproject__announcer-match-begins.ogg              Match begins announcement
```

The original file names are kept: they carry the Freesound id and the author, which is the credit key (§6).

### 1.1 What is removed at the end

| Removed | Why |
|---|---|
| `assets/audio-to-re-organize/` (the whole folder; the task spells it `re-organise`, the folder on disk is `re-organize`) | The task asks for it once the move is done |
| In it, **unused**: `761501__valhallaproject__energy-pistol-fireexplosion.wav` (8.1 s, 2 MB; the `-short` version is the one in the database), `583715__dexd73__walking-on-metal-loop.wav` (the database picks the `running` clip) | Not in the database |
| `assets/audio/sfx_ping.wav`, `music_menu.wav`, `music_game.wav` | Placeholders, replaced |

Size note: the two MP3s are 4 MB each and the two growl / gunshot sources are 2 MB WAVs. `slow-pitch-growl.wav` is 7.8 s: in S1 it is cut into **two files**: `slow-pitch-growl-hit.wav` (seconds 0–2, for `EnemyHit`) and `slow-pitch-growl.wav` itself rewritten as seconds 3–7 (for `EnemyDeath`, so the database path stays valid). The credits mark both "trimmed". The gunshot file in the database is already a trimmed version of the original, which is what `short` in its name means; the credits say so too.

---

## 2. Events and triggers

| Event (task) | `Sfx` / music | Trigger (where) | Notes |
|---|---|---|---|
| Music Menu | `MusicManager::Theme::Menu` | `ModeChangedEvent` → MainMenu (**exists**) | Path change only |
| Music Game | `MusicManager::Theme::GamePlay` | `ModeChangedEvent` → Play (**exists**) | Path change only |
| Gunshot | `Sfx::Shot` | `ShotFXSpawner` on `ShotFiredEvent` (**exists**) | Table has a small pitch range, as `PlayShot` does today |
| Player dies | `Sfx::PlayerDie` | `ArenaGame`, subscriber of `PlayerDiedEvent` (**exists**, already calls `LoseGame`) | One added line in the handler |
| Player jumps | `Sfx::PlayerJump` | `FirstPersonController::FixedUpdate`, where `ConsumeJump()` succeeds and `motor->Jump()` is called | Fires once per real jump |
| Player walks | `Sfx::PlayerStep` (loop) | `FirstPersonController::FixedUpdate`: grounded **and** horizontal speed above 0.5 m/s → keep the clip playing; otherwise stop it | The `VoiceHandle` is kept in the controller. Stops on jump, fall, standing still, death, level reload and pause (`PauseAll`) |
| Enemy deals damage to player | `Sfx::PlayerHurt` (renamed `EnemyHitsPlayer`) | `ArenaGame::OnPlayerHit` (**exists**, already plays `PlayPlayerHurt`) | Same place, new clip |
| Enemy dies | `Sfx::EnemyDeath` | `LifeSystem` (**exists**) | Same place, new clip |
| (extra) Shot hits an enemy | `Sfx::EnemyHit` | `EnemyManager` shot handler (**exists**, plays `PlayEnemyHit`) | Own file: growl seconds 0–2 (A‑D6) |
| Win announcement | `Sfx::Win` | `ArenaGame`, subscriber of `GameWonEvent` (**exists**) | One added subscription |
| Match begins announcement | `Sfx::MatchBegins` | `ArenaGame`, subscriber of `GameStartedEvent` (**exists**, published by `GameState::StartGame`) | Plays at every start, restart and retry: a "match" begins each time |

All 8 events already have a hook. No new event type is needed.

---

## 3. Where audio characteristics are authored

| What to tune | Where | Form |
|---|---|---|
| Which file plays for an event | `SFX_TABLE` in `game/state/SoundFXBank.cpp` | `path` |
| Volume, random pitch range, per sound | same table | `volume`, `pitchMin`, `pitchMax` (equal values = fixed pitch) |
| Footstep detection threshold | `FirstPersonController.cpp` | `STEP_MIN_SPEED` constant |
| Music files, base volume, fade in, cross‑fade | `game/audio/MusicManager.h/.cpp` | `MENU_TRACK`, `GAME_TRACK`, `VOLUME`, `FADE_IN`, `CROSSFADE` |
| Master / SFX / music bus volume (player) | Settings screen (**exists**) | Unchanged |
| Pause / editor ducking | `AudioManager::DUCK_LEVEL` | Unchanged |
| Pitch / volume at runtime for quick tests | Developer console, optional command `sfx <name> [volume] [pitch]` | **Not built** (developer answer R2) |

Sketch of the table (the only new structure):

```cpp
enum class Sfx { Shot, PlayerDie, PlayerJump, PlayerStep, EnemyHitsPlayer, EnemyHit, EnemyDeath, Win, MatchBegins, Count };

struct SfxDef { const char* path; float volume; float pitchMin; float pitchMax; };

static constexpr SfxDef SFX_TABLE[] =           // same order as the enum
{
	{ "assets/audio/sound_fx/player/761501__...-short.wav",  0.30f, 0.95f, 1.05f },     // Shot
	{ "assets/audio/sound_fx/player/469567__...die8.wav",    0.80f, 1.00f, 1.00f },     // PlayerDie
	...
};
```

`Load()` loops the table; `Play(sfx)` picks a pitch in `[pitchMin, pitchMax]` and calls `AudioManager::Play`. A sound
that fails to load stays skipped silently after the loader's warning (as today).

---

## 4. What changes, by file

| File | Change |
|---|---|
| `game/state/SoundFXBank.h/.cpp` | New `Sfx` list, `SFX_TABLE`, one `Play(Sfx)`; wrappers removed; `Play` returns the `VoiceHandle` so the footstep can be stopped |
| `game/audio/MusicManager.h/.cpp` | Two new track paths (and `.mp3` instead of `.wav`) |
| `game/player/FirstPersonController.cpp/.h` | Jump sound; footstep loop (`VoiceHandle m_stepVoice`) |
| `game/ArenaGame.cpp` | Add the `PlayerDie`, `Win`, `MatchBegins` plays; `PlayerHurt` → `EnemyHitsPlayer`; unsubscribe in the destructor like the other tokens |
| `game/fx/ShotFXSpawner.cpp`, `game/enemies/systems/LifeSystem.cpp`, `game/enemies/EnemyManager.cpp` | `PlayX()` → `Play(Sfx::X)`; `EnemyHit` stays, with its own growl cut (A‑D6) |
| `assets/audio/` | Move and delete as in §1 |
| `assets/audio/CREDITS.txt`, optional Credits screen | §6 |
| `README.md`, `assets/README.md` | Fix mentions of the placeholders |
| Engine (`src/engine/**`) | **No change** |

---

## 5. Implementation plan

| Phase | Branch | Work | Done when |
|---|---|---|---|
| S1 | `audio_assets` | Move the files (§1), rewrite `CREDITS.txt`, point `MusicManager` at the MP3s | The menu and the game each play their Rod Kim track; no load warning in the log |
| S2 | `audio_sfx_table` | `SoundFXBank` table and `Play(Sfx)`; migrate the 4 existing call sites; gunshot, enemy damage and enemy death use the new clips | Shot, enemy dies and "player takes damage" sound like the database clips |
| S3 | `audio_player_events` | Jump, footstep loop, player death, win and match‑begins | All 8 events are heard. The footstep stops when standing still, in the air, on pause and on death |
| S4 | `audio_cleanup` | Delete `audio-to-re-organize/`, the unused files, and the placeholder audio files (`tools/generate_placeholder_audio.py` stays); fix docs | `grep -rn "sfx_ping\|music_menu\|music_game\|audio-to-re-organize" .` finds nothing outside the docs history; Debug and Release build |

One commit per task, the developer tests each phase, as in TASK_04.

---

## 6. Answer: what does the industry standard say about attributing the authors?

**Short answer:** credit the author, always, even when the license does not demand it. For Creative Commons material the
accepted format is **TASL: Title, Author, Source, License**, shown where players or buyers can find it. For a game that
means a **Credits screen plus a credits file shipped with the build**, and the store page if there is one. How
much is *legally required* depends on the license of each sound, and the database does not list the licenses.

**What the standard says**

1. **Creative Commons** ([Recommended practices for attribution](https://wiki.creativecommons.org/Best_practices_for_attribution)) recommends TASL: the *title* of the work, its *author* (the real creator, not the platform), the *source* (a link to the original page) and the *license* (name, with a link). If a piece of the information is missing, give as much as you have. For audio and video it suggests a credits page, which maps to a game's credits screen and a credits file. It also says to indicate **changes** you made (a trimmed or pitch‑shifted clip is a modification; "trimmed from the original" is enough).
2. **Freesound** puts one license on each sound, so there is no single rule for the whole database: **CC0** (no attribution needed, crediting is still polite), **CC BY** (credit required) and **CC BY‑NC** (credit required **and no commercial use**). It is the license on the sound's own page that counts.
3. **Games in practice:** credits are usually collected in one place (a scrolling or static *Credits* / *Third‑party* screen reachable from the main menu, a `CREDITS` / `THIRD_PARTY_NOTICES` file next to the executable, and the store description). Music and sound credits are normally grouped under "Audio". Each line gives who made it, what it is, where it came from and under what license.

**What this project should do**

| Item | Action |
|---|---|
| Format of each credit line | `"<Title>" by <Author> — <Source URL> — <License>` plus `(trimmed)` / `(pitch‑shifted at runtime)` if modified. Marked "trimmed": the growl (two cuts: 0–2 s for the hit, 3–7 s for the death) and the gunshot (`short` = a trimmed version of the original) |
| Where | `assets/audio/CREDITS.txt` (shipped by `package_release.sh`, which copies `assets/`) **and** a Credits screen from the main menu (small: a `UIScreen` that prints the file's text; follow‑up task, not this one) |
| Source data | The `sound_database.md` table is the source of truth. Keep it in sync with the files; at the end of S4 `CREDITS.txt` is generated from it by hand |
| **Missing today: the license column** | The database has author and URL but **no license**. Before release, open each Freesound page and record the license. Any **CC BY‑NC** sound must be replaced if the game is ever sold |
| Entries to fix in the database | (a) the Gunshot row has a broken, duplicated link (`...data ; URL: https://freesound.org/people/ValhallaProject/sounds/767627/` inside the first link; 761501 is the sound, 767627 belongs to the "win" announcer); (b) the Valhalla rows list the GitHub project instead of a person: credit as **ValhallaProject (Project Valhalla)** with both links; (c) the Rod Kim rows have the author and site but no track title page or license |
| Rod Kim's music | **License unverified.** I could not find terms for these two tracks. Ask for or check the permission on [rodkim.com](https://www.rodkim.com/) before shipping. The old `CREDITS.txt` already warned about this |

I could not confirm any license online (the searches did not find the Freesound pages and Freesound asks for a login), so the license of each sound is **unknown to me** and is not stated above.

---

## 7. Risks and notes

| # | Risk | Mitigation |
|---|---|---|
| R1 | Footstep loop gaps or double voices (replay while the old voice still plays) | Replay only when `!IsPlaying(handle)`; the clip is cut at a footstep boundary (trim in S3 if there is an audible click) |
| R2 | The loop file is called *running* but the key is *walks* | Played at pitch < 1 if it sounds too fast: `pitchMin/Max` in the table |
| R3 | `LoadSound` decodes the whole OGG / WAV into memory; the 2 MB growl and gunshot sources stay as WAV | 2 MB each is small; trim later if load time matters |
| R4 | The announcer clips overlap the music and the death sound | Different buses are not needed; tune `volume` in the table |
| R5 | `MatchBegins` plays on every restart and retry | Intended; remove the subscription if it grates |
| R6 | Licensing (§6): shipping with a NC or unlicensed sound | Check every license before release; replace anything that does not fit |
| R7 | Removing placeholders breaks tests or references | S4 `grep` check; tests that load `sfx_ping.wav` (if any) updated in S2 |