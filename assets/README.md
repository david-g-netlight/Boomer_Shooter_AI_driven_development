# Assets

Runtime data for the game. The whole folder is copied next to the executable after every build
(`<build dir>/assets/`), and the engine loads paths relative to the executable's directory.

| Folder | Content |
|---|---|
| `levels/` | `.lvl` level files saved by the editor |
| `models/` | `.glb` / `.gltf` / `.obj` models |
| `shaders/` | `.vs` / `.fs` GLSL shaders |
| `audio/` | Sound effects (`.wav`) and music (`.ogg`), plus `CREDITS.txt` |
| `fonts/` | `.ttf` fonts and raygui styles |
