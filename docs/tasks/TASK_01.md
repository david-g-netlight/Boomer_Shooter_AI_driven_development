# TASK_01
Create a Software Architecture plan for a game engine using Raylib, with reference to an engine repo and the gameplay repo.

Read the engine architecture of this repo: `https://github.com/david-g-netlight/p_Game_Engine_Cpp_OpenGL.git`. Consider this only as a reference for the engine architecture; the actual implementation of modules like shaders, camera and others is provided by Raylib.

Outline the software architecture needed to build a simpler version using the raylib library, while keeping the existing arrangement in this repo.

The Engine should include the same features as the reference, plus the following additional features:
Enter and exit editor mode.
Main menu to start, pause and resume.
Follow the basic needs of the Game.

This is the gameplay repo in Unity: `https://github.com/DavidGiraldoCode/p_gmtk_game_jam_build_to_scale`, where the C# classes will give the context of the functionalities.
It's an FPS game with funny puzzle mechanics. Scale things up and down with your gun click, and change the scale factor with a scroll.
You can scale up with a left click, scale down with a right click, and change the scale factor with a scroll.
You can ask me about the game if you don't manage to access the files.

# Output:
Architecture plan in a .mf file inside the repo. It should contain:
The implementation plan in order from the initial stages necessary to set things up, to the latest Application layer implementations.
ASCII diagram of the architecture
ASCII diagram of the game loop
List of classes for the Engine
List of classes for the Game