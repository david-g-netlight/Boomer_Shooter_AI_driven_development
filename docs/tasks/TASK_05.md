Task 005-designing-a-simple-enemy-system

# Task:
Design a simple enemy system based on the following requirements, and present an architectural plan that extends the current work.

# Input:
Read the current `docs/Boomer_Shooter_ARCHITECTURE_PLAN.md`.

# Requieremnts:
Create an enemy system that considers:
Simple AI state machine: idle, wander, and chase behaviour (see details below).
Enemy Wave Spawner Manager
Enemy Pool
Simple ECS-inspired enemies.

### Example of the Enemy Architecture: ECS-inspired (subject to change)
EnemyRegistry
EnemyIds 
Enemy Components: Scan for nearby player, wander, avoidOther, Chase player, Life, Deal Damage To Player, Transform, Mesh, Collision detection. (Subject to change)
Enemy Systems: ScanningNearPlayerSystem, WanderSystem, DealingDamageSystem, AvoidingOtherSystem, ChasePlayerSystem, etc. (Subject to change)

### Simple AI state machine details:
Idle: Slow float up and down for 5 seconds, then change to wander.
Wander: Move forward until you find a wall, draw a random direction inside a unit sphere, check for room to move, update transform orientation, and take that as the new forward direction.
Chase: If the squared Euclidean distance to the player's position is less than the enemy's triggering threshold, the enemy must chase the player. While that is true, keep chasing. If not, be idle for 5 seconds.
Attack: When in range, the enemy deals a set amount of damage to the player and enters a 1-second cooldown. When out of range, it returns to chasing.

### Enemy Geometry:
No skeletal animation.
Kinematic animation for attack and states.
Use a sphere and a pyramid for the enemy body. It will change for complex meshes later.

## Criteria:
No third-party library; propose a vanilla C++ implementation; strive for simplicity.
Motivate cache/memory-related decisions.
It must spawn 20 enemies.
A gunshot hit deals damage to the enemy.
Include 3 enemy types, each more difficult than the previous one.
Explain where the AI system and the tiny ECS should live.

# Output:
Create an .md file called `docs/Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN.md` to present the design.
Include an answer to these doubts:
Should the AI system be an engine-level system? I'd like to add more AI behaviour in the future (like pathfinding), but not now.
Should the small ECS be an engine-level system? I'd like to use it for NPCs and effects, and explore other uses.

I will review the presented design and iterate on it.