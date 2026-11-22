# Task 04:
Follow the development plan in `docs/Boomer_Shooter_DEV_PROGRESS.md`.

## Criteria  
- Use only local branches. 
- Follow the development plan.
- Do not delete the previous branches.
- New branches name depend on development stages, for example, for "Project setup and build system," the branch would be project_setup_and_build_system.
- Each checkbox should be a distinct commit  containing the necessary work

## Workflow loop:
Follow the steps:
1. Check out the latest integration branch and create a new branch for the stage you will be working on.
2. Commit work per sub-stage (or checkbox), excluding the "done when." You may prompt the developer for clarifications or necessary plan adjustments.
3. When ready, wait for the developer to tests the branch.
4. If changes, iterate and create new commit, do not ammend, go back to step `3`.
5. If they give the green light, check for changes, and commit the devs approval.
6. Merge into the integration branch.
6. Rename the feature branch by adding the prefix done/.
7. Go back to step `1`.

# Input:
`docs/Boomer_Shooter_ARCHITECTURE_PLAN.md`
`docs/Boomer_Shooter_DEV_PROGRESS.md`
You may read the .h and .cpp files for more context if needed.

# Output:
Commits with testable code changes.