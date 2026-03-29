# TASK-0000 - Repository Bootstrap and Governance Scaffold

## Objective
Create the initial `piFartBox` repository structure and establish the mandatory planning and documentation workflow before feature implementation begins.

## Success Criteria
- New repository root exists at `C:\piFartBox`.
- Core top-level directories exist for engine, Linux platform, Zynthian integration, control API, web UI, content, and docs.
- `AGENTS.md` defines strict AI workflow and context-hygiene rules.
- `docs/plans/`, `docs/todo.md`, and `docs/completedTasks.md` exist and are used.
- At least the first substantial follow-up tasks are tracked with full plan files.
- The repository has a minimal CMake scaffold for future native targets.

## Constraints
- Do not inherit Pico-specific runtime architecture into this repository.
- Keep active context docs compact to avoid noisy refresh behavior.
- Put full decision detail in plan files only.
- Browser audio and Web Serial workflows are out of scope for the new repository.

## Implementation Design
- Create the top-level repository folders and placeholder ownership READMEs.
- Add a root `README.md` that defines product direction and non-goals.
- Add a root `CMakeLists.txt` plus placeholder subsystem CMake files so the repo has a native build skeleton.
- Add strict `AGENTS.md` governance covering task IDs, plan-first workflow, and completion movement from `todo.md` to `completedTasks.md`.
- Seed the task system with initial planned architecture tasks.

## Interfaces / Types Affected
- Repository-level documentation and workflow only.
- No runtime schemas or engine types are introduced in this bootstrap task.

## Test Plan
- Verify required directories and docs exist.
- Verify `todo.md` and `completedTasks.md` are compact indexes rather than long-form documents.
- Verify each tracked task has a corresponding plan file.
- Verify the root CMake configure surface exists for later implementation.

## Assumptions / Defaults
- `piFartBox` is the permanent new project root.
- This bootstrap task can be treated as completed once the repository scaffold and workflow rules are in place.

## Out of Scope
- Native DSP implementation
- Zynthian engine integration code
- Control API endpoints
- Browser application implementation
- SL MkIII runtime mapping