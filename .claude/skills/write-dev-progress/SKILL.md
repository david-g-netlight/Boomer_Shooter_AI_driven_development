---
name: write-dev-progress
description: Write a DEV_PROGRESS.md tracker (and its history file) from an approved architecture plan in docs/. Use when a plan's phases are approved and the developer asks for a progress / tracking file, or when a task says "create *_DEV_PROGRESS.md".
---

# Write a DEV_PROGRESS.md tracker

A tracker turns an approved architecture plan into a checklist that an agent can follow one commit at a time, with
the developer testing each phase. The tracker is **lean**: it only holds what an agent needs to act *now*. History
lives in a separate file.

Reference example: `docs/Enemy_System_v1_DEV_PROGRESS.md` (tracker) and `docs/devlogs/backup.md` (its history).

## Before writing

1. Read the plan's **implementation plan** section (phases, bullets, **Done when**) and its **decisions** table (§0).
   Find them with `grep -n "^## \|^### Phase\|Done when" docs/<PLAN>.md`, then Read those line ranges.
2. Read the example tracker above to copy its format exactly.
3. Write **no** checkbox that is not in the plan. If a phase seems to be missing work, ask the developer and do not
   invent it.

## Files to create

| File | Holds |
|---|---|
| `docs/<Plan_Name>_DEV_PROGRESS.md` | Header, how to use, overview, phase checklists, **open** questions only |
| `docs/devlogs/<Plan_Name>_DEV_LOG.md` | Decision log, answered questions, risks, session log (the history) |

The task may give other names. Use the task's names, and link the two files to each other in their headers.

## Tracker structure (in this order)

Fill [`template.md`](template.md). Section by section:

1. **Title and header.** `# Development Progress — <Plan name>`. Then one line linking the plan and the section
   where its phases are defined, one line linking the previous tracker (if any), and one line linking the history
   file.
2. **How to use this file.** Copy the template's bullets. Change only the phase letter, the decision prefix and the
   file names.
3. **Overview table.** One row per phase: `| <ID> | <Name> | \`<branch>\` | ⬜ | | | <short note> |`.
   - Phase IDs use one letter + a number, unique across the repo (B0…, E0…). Check older trackers so a letter
     is not reused.
   - The branch name is the phase name in `snake_case`, with a short prefix for the feature (`enemy_ecs_core`).
   - Under the table: `**Current focus:**` and `**Branch:**` lines.
4. **Phase checklists.** One `### Phase <ID> — <Name> ⬜` per phase:
   - One `- [ ]` per **commit**: each line is one piece of work that builds and makes sense alone. Merge bullets
     that cannot compile without each other. Split a bullet that touches unrelated systems.
   - Name the real files, classes and functions (`` `EnemyPool` ``, `` `game/enemies/` ``). An agent must be able to
     start without reading the whole plan again. Add the plan section in brackets: `(§6.1)`.
   - The last line is always `- [ ] **Done when:** <checks copied from the plan>`. Only the developer ticks it.
   - Write the checks as things a person can see or run: "a 33rd spawn logs 'pool full'", not "the pool works".
5. **Open questions** (only if there are some). Use `| # | Question | Default if no answer |`. A question always has
   a default, so work is never blocked without a way forward.

Do **not** put a decision log, answered questions, a risk table or a session log in the tracker. They go in the
history file.

## History file structure

`# Development Log — <Plan name>`, a link back to the tracker, then these sections in order:

- `## Decision log`: `| # | Date | Decision | Plan section |`. Copy every decision from the plan's §0 with a short
  summary. For a revised decision, strike the old value through and say why: `~~6 / 7 / 7~~ Revised by the review (Q1): …`.
- `## Questions for the developer`: answered questions, with the old text struck through and **Answered:**.
- `## Known issues and risks being watched`: `| Issue | Phase | Status | Notes |` (status: open / accepted / resolved).
- `## Session log`: newest first, `| Date | Phase | What was done | Next step | Blockers |`.

## Writing style

- Short, plain sentences. Dates as `YYYY-MM-DD`. Status emoji only from the legend: ⬜ 🟡 ✅ ⛔.
- Use the same names as the plan and the code. Put code names in backticks.
- Never mark anything done in a new tracker.

## Finish

1. Check that every link resolves: `ls` each linked path.
2. Check that the phases and Done‑when lines match the plan: grep the plan's `Done when` lines and compare.
3. Add the first session‑log row to the history file: `<date> | — | Tracker written for <plan> | Developer reviews, gives the go for <first phase> | —`.
4. Stage or commit only as the task says (default: stage, don't commit). Report the two file paths to the developer.
