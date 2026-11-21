---
name: follow-dev-progress
description: Implement an approved plan phase by phase from a docs/*_DEV_PROGRESS.md tracker, with the TASK_04 workflow (one local branch per phase, one commit per checkbox, the developer tests, then merge into integration and rename to done/). Use when a task says "follow the development plan / DEV_PROGRESS", "start phase X", "continue", or when the developer gives a green light or test feedback on a phase.
---

# Follow a DEV_PROGRESS.md tracker (TASK_04 workflow)

You implement **one phase at a time**. The developer is in the loop: they answer questions, test every phase and
decide when it is merged. Source of the workflow: `docs/tasks/TASK_04.md`.

## Hard rules (never break these)

- Local branches only. **Never push.** Never delete a branch. Never `git commit --amend`, `rebase` or `reset --hard`.
- Never tick a **Done when** line, mark a phase ✅ or merge without the developer's explicit green light.
- One commit per checkbox. The **Done when** line is not a commit of yours. It is ticked in the validation commit.
- Never commit or discard changes you did not make (the developer's edits) without asking, except in step 6 below.
- Do only what the checkbox says. If the plan is wrong or unclear, **stop and ask** (step 3). Don't redesign.
- Say what you did **not** verify (for example interactive checks in the game). Never claim a check you did not run.
- Keep tool output small: pipe builds and logs through `tail`, `grep` or `head` (see `CLAUDE.md`).

## Files you use

- The tracker `docs/<Plan>_DEV_PROGRESS.md`: header links, overview, phase checklists, open questions.
- The history file linked in the tracker's header (for example `docs/devlogs/backup.md`): decision log, answered
  questions, risks, session log. Read it only when you need a decision's reason. Write to it in steps 5 and 6.
- The plan linked in the tracker's header. Read only the sections a checkbox cites (`§6.1` → `grep -n "### 6.1"`).

## The loop

### 1. Find where you are

```bash
git status --short && git branch --show-current
grep -n "Current focus\|Branch:\|🟡\|⬜" docs/<Plan>_DEV_PROGRESS.md | head -20
```

- A phase is 🟡 and its branch exists → check it out and continue with its first unticked checkbox.
- Everything ✅ → tell the developer the plan is done, and stop.
- Otherwise the next phase is the first ⬜ one in the overview.
- Uncommitted changes you did not make → ask the developer what to do with them before switching branches.

### 2. Start the phase

```bash
git checkout integration
git checkout -b <branch from the overview table>
```

Edit the tracker: phase status 🟡, the **Started** date (`YYYY-MM-DD`), `**Current focus:**` and `**Branch:**`.
This edit goes into the first checkbox commit.

### 3. One checkbox = one commit

For each unticked `- [ ]` line of the phase, in order (skip **Done when**):

1. Read the plan section it cites and the code it names. Match the style of the surrounding code.
2. Make the change. Keep it to what the line says.
3. Build Debug. Build Release too when the change touches build flags, `#if` guards or engine headers:
   ```bash
   cmake --build build/debug -j 2>&1 | grep -E "error|warning" | head -20
   cmake --build build/release -j 2>&1 | grep -E "error|warning" | head -20
   ```
   (Configure first if the folder is missing: `cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug`.)
   Fix every error and every new warning before you commit.
4. Commit only the files of this checkbox:
   ```bash
   git add <files>
   git commit -m "<PhaseID>: <what this commit does, in plain words>"
   ```
   Example: `E1: EnemyPool with capacity 32 (Spawn, FlushReleased, Clear)`. Add the attribution trailer your
   environment requires.

If you get stuck, or the plan conflicts with the code: **stop**. Add the question to the tracker's
**Open questions** table (with a default answer), then ask the developer. Wait for the answer. Record the answer in
the history file: the answered question, plus a **Decision** row if the plan changes.

### 4. Hand the phase over for testing

1. Clean build from scratch, both configurations, into fresh folders:
   ```bash
   rm -rf build/check-debug build/check-release
   cmake -S . -B build/check-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/check-debug -j 2>&1 | tail -5
   cmake -S . -B build/check-release -DCMAKE_BUILD_TYPE=Release && cmake --build build/check-release -j 2>&1 | tail -5
   ```
2. Run what you can without a person (start the app briefly, read the log for warnings).
3. One commit `<PhaseID>: tracker ticks and session log`:
   - Tick each done checkbox `[x]`. After the text, add a short note in brackets that starts with `+` for anything
     the developer should know: deviations, merged checkboxes, follow‑ups. Example:
     `(+ done in one commit with the next line: they depend on each other)`.
   - Session‑log row (history file, newest first): what was done, which builds passed, **what you did not run**,
     next step = "Developer runs the <PhaseID> **Done when** check".
4. **Stop.** Tell the developer: the branch name, the commits, what you checked, and the exact **Done when** list
   for them to test. Wait.

### 5. The developer reports problems

Fix them with **new** commits on the same branch (`<PhaseID>: fix <problem>`), never amend. Add a session‑log row.
Go back to step 4 (rebuild, hand over, wait).

### 6. The developer gives the green light

1. `git status --short`: the developer may have changed files while testing (a typo fix, a level tweak).
   If the changes clearly belong to the phase, include them in the commit below and name them in the session log.
   If they are unclear or unrelated, ask first.
2. One commit `<PhaseID> validated by the developer: Done when passed, tracker marks <PhaseID> done`:
   tick **Done when** `[x]`, phase heading and overview ✅, **Finished** date, Branch column `done/<branch>`,
   `**Current focus:**` → the next phase (or "none"), session‑log row
   ("Developer validated <PhaseID>; merged into `integration`, branch renamed `done/<branch>`").
   Write the merge and the rename in that row now: they happen next, in step 7.
3. Merge and rename:
   ```bash
   git checkout integration
   git merge --no-ff <branch> -m "Merge Phase <PhaseID>: <name> (<3-6 key items>)"
   git branch -m <branch> done/<branch>
   ```

### 7. Next phase

Back to step 1. TASK_04 continues straight into the next phase. If the developer asked you to stop after this
phase, stop and say which phase is next.

## Checklist before every stop

- [ ] `git status --short` is clean, or every leftover file is explained to the developer.
- [ ] The tracker and the history file match what is in git (ticks, status, dates, session log).
- [ ] Your message says what was verified and what was not.
