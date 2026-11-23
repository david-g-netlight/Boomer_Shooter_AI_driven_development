## 8. Open questions for the developer

Q1: The shot‑hit tick (`Sfx::EnemyHit`) has no asset. Remove it, or reuse the gunshot at a higher pitch? R1: Reuse the `Sfx::EnemyDeath` at higher pitch and just the frist 2 seconds. 

Q2: Add a console command `sfx <name>` to audition sounds?  R2: No

Q3: Trim the 7.8 s growl to ~1 s as part of the move? R3: Yes, but trim from 3 seconds, in S1, note it in the credits as "trimmed". Also mention `fireexplosion-short` is trimmed, hende the `short`.

Q4: Build the in‑game Credits screen in this task, or only the `CREDITS.txt`? R4: `CREDITS.txt` now, screen in a follow‑up 

Q5: Should "Match begins" also play when the next wave starts? R5: No, once per match.

BTW: Do not delete the Python script for placeholder sounds, it is useful.