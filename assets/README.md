# Assets

Runtime data for the game. The whole folder is copied next to the executable after every build
(`<build dir>/assets/`), and the engine loads paths relative to the executable's directory.

| Folder | Content |
|---|---|
| `levels/` | `.lvl` level files (`level_01` .. `level_03`), saved by the editor |
| `shaders/` | `.vs` / `.fs` GLSL shaders |
| `audio/` | Sound effects and music (`.wav`), plus `CREDITS.txt` |
