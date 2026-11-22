# TASK_05_Iteration

Architecture plan approved with comments; see answers to questions.

# Input: Answers to the Open questions for the review
- **Q1** (E‑D4): 20 enemies in total over 3 waves, or 20 alive at once? 
20 enemies for each wave, with this new distribution:
Wave 1: { Drifter, 7 }, { Stalker, 0 }, { Brute, 0 }
Wave 2: { Drifter, 5 }, { Stalker, 2 }, { Brute, 0 } 
Wave 3: { Drifter, 3 }, { Stalker, 2 }, { Brute, 1 } 

- **Q2** (E‑D6): add hysteresis to Chase → Idle (exit at 1.2 × trigger radius) so enemies at the edge do not flicker
 between Chase and Idle? Yes, avoid flickering.

- **Q3**: Should a shot enemy start chasing even outside its trigger radius ("aggro on hit")? Fake it: if a player shoots an enemy outside chasing range, keep the wander state but change the enemy's forward direction to the player's position when they shot.

- **Q4**: the 5‑minute `ArenaTimer` (survive = win). Add it as phase E6 here, or keep it as its own small plan? Keep it as a small plan, but document the timer's current state and what it would require.

# Output:
Update Boomer_Shooter_Enemy_System_v1_ARCHITECTURE_PLAN accordingly.
Create `Enemy_System_v1_DEV_PROGRESS.md` to track development and implementation.
Stage changes (including the level file) so I can make updates as needed, and then you can run `git diff` to fetch only what is new.
