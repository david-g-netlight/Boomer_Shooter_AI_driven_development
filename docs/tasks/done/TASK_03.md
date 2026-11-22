# Task: Create a plan to change the architecture of the current game engine:

##  Global Context of the project:
This game engine will be promoted to support the minimal game loop of a Boomer-Shooter, where the player spawns inside an arena; a counter starts (5 minutes), and several waves (~3) of enemies appear every time the player clears the room. The game still has 2 win conditions: kill all waves before the time runs out or survive the whole 5 minutes.

##  Context of the task:
This task covers planning the removal of the current Scale game logic and rules from the Game Engine to allow the development team to implement the boomer shooter.

## Criteria:
Remove and simplified
Remove the Scale game-related logic: scalable components.
Remove the editor GUI. 
Keep:
First-person controller, gun and shoot interaction.
In-game HUD and GUI
Set only one Level, but keep functionality to add more levels and transitions.
Keep editor capabilities.
Add:
Add an in-game developer console to input commands to edit the game. This should be under a precompile directive, so it is removed once the game is released.
Use a 16x6x16 cube with inverted normals (x, y, z) as the Level for now. 

# Input:
Prioritise reading:
`docs/ARCHITECTURE_PLAN.md`
`docs/DEV_PROGRESS.md`
You may read the .h and .cpp files for more context if needed.

# Output:
Create the following plans:
`docs/Boomer_Shooter_ARCHITECTURE_PLAN.md`
`docs/Boomer_Shooter_DEV_PROGRESS.md`