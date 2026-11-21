# Development Progress — <Plan name>

Tracks the implementation of [`<PLAN_FILE>.md`](<PLAN_FILE>.md). Phase names and **Done when** criteria match
§<N> of that plan. <Optional: the previous work's history stays in [`<OLD_TRACKER>.md`](<OLD_TRACKER>.md).>
History of this plan (decision log, answered questions, risks, session log): [`devlogs/<Plan_Name>_DEV_LOG.md`](devlogs/<Plan_Name>_DEV_LOG.md).

**How to use this file**
- Tick a task `[x]` when it is merged on the working branch, not just when it is written.
- A phase is ✅ only after its **Done when** check passes on a clean build (`cmake --build` from scratch), Debug **and** Release.
- At the end of every work session, add a line to the **Session log** in the history file. When the plan changes, add a **Decision** (<X>‑Dn) there too.
- Open questions for the developer go in an **Open questions** section in this file. Once a question is answered, move it to the history file.
- Branches: one per phase from `integration`, one commit per checkbox (except **Done when**, checked by the developer); a finished branch is renamed `done/<branch>` ([`tasks/TASK_04.md`](tasks/TASK_04.md)).
- Status legend: ⬜ not started · 🟡 in progress · ✅ done · ⛔ blocked

---

## Overview

| Phase | Name | Branch | Status | Started | Finished | Notes |
|---|---|---|---|---|---|---|
| <X>0 | <Name> | `<feature>_<name>` | ⬜ | | | <one-line scope> |
| <X>1 | <Name> | `<feature>_<name>` | ⬜ | | | |

**Current focus:** none. Waiting for the go to start <X>0.
**Branch:** `integration`

---

## Phase checklists

### Phase <X>0 — <Name> ⬜
- [ ] <one commit: files, classes, what they do> (§<plan section>)
- [ ] <one commit>
- [ ] **Done when:** <observable checks copied from the plan>

### Phase <X>1 — <Name> ⬜
- [ ] <one commit>
- [ ] **Done when:** <observable checks>

---

## Open questions

| # | Question | Default if no answer |
|---|---|---|
| Q1 | <question> | <what the agent does if there is no answer> |
