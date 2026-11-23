# Assets

Runtime data for the game. The whole folder is copied next to the executable after every build
(`<build dir>/assets/`), and the engine loads paths relative to the executable's directory.

| Folder | Content |
|---|---|
| `levels/` | `.lvl` level files: `arena_01.lvl`, the only level, saved by the editor |
| `shaders/` | `.vs` / `.fs` GLSL shaders |
| `audio/` | `music/` (`.mp3`), `sound_fx/` (`.wav`, `.ogg`: `player/`, `enemy/`, `announcements/`), plus `CREDITS.txt` |
