# AGENTS.md

## Purpose
This repository builds the new production synth workstation `piFartBox` for Raspberry Pi 1 B+ running `ZynthianOS`.

## Product Truths
- `ZynthianOS` is the deployment base.
- The only audio runtime is the native C++ engine.
- The browser is a Pi-hosted editor/control surface only.
- The engine schema is the source of truth for modules, instruments, slots, sessions, and controller metadata.
- The product model is:
  - `Module`
  - `Instrument`
  - `Slot`
  - `Session`
- The first release target is 4 simultaneous slots.
- The modular model is curated, not fully free-form patch cabling.
- Novation SL MkIII is a first-class controller surface.

## Repository Layout
- `engine/`: native DSP engine and core model contracts
- `session-manager/`: session persistence, slot assignment state, and controller mapping ownership
- `platform/linux/`: Linux audio, threading, filesystem, and host glue
- `integrations/zynthian/`: Zynthian-native adapter/integration code
- `control-api/`: Pi-hosted local API for browser/control surfaces
- `controller-mapper/`: dedicated hardware-controller adapters and page/mapping logic
- `web/`: Pi-hosted browser editor and performance UI
- `content/`: factory instruments, controller profiles, default sessions
- `docs/`: plans, task indexes, architecture notes, and operating rules

## Required Documentation Structure
The following files and directories are mandatory and must remain in use:
- `docs/plans/`
- `docs/todo.md`
- `docs/completedTasks.md`
- `docs/updater-system.md`

## LLM Workflow Rules
1. Explore the repository first before proposing or making changes.
2. For every substantial task, create or update a full plan file in `docs/plans/` before implementation.
3. Each substantial task must map to exactly one plan file.
4. Each task must have a stable task ID using the format `TASK-0001`.
5. Add or update the task entry in `docs/todo.md` when work starts or scope changes.
6. When the task is complete, remove it from `docs/todo.md` and add it to `docs/completedTasks.md`.
7. Treat the plan file as the authoritative decision record for the task.
8. Document architecture assumptions before code changes when those assumptions affect interfaces or runtime boundaries.

## Plan File Standard
Each plan file in `docs/plans/` must include:
- task ID
- title
- objective
- success criteria
- constraints
- implementation design
- interfaces/types affected
- test plan
- assumptions/defaults
- explicit out-of-scope items

Recommended naming:
- `TASK-0001-zynthian-engine-foundation.md`
- `TASK-0002-engine-schema-and-session-model.md`

## Task Index Rules
`docs/todo.md` must remain compact and index-like. Every entry must include:
- task ID
- title
- status
- owner/role
- plan link
- last updated date

`docs/completedTasks.md` must remain compact and index-like. Every entry must include:
- task ID
- title
- plan link
- completion date
- concise outcome summary

## Context Hygiene Rules
- Do not store large planning conversations, raw research dumps, or oversized implementation logs in files that are likely to be picked up during context refresh.
- Keep `AGENTS.md`, `docs/todo.md`, and `docs/completedTasks.md` concise.
- Put detailed task context only in the specific file under `docs/plans/`.
- Do not create ad hoc large working-memory files at the repository root.
- If research is needed, summarize decisions in the task plan instead of copying large source material.
- Do not use `todo.md` or `completedTasks.md` for long-form design notes.

## Architecture Rules
- Do not reintroduce Pico-specific runtime assumptions into this repository.
- Do not add browser-side audio synthesis or a browser emulator as a source of truth.
- The engine owns the canonical schema; browser/UI code consumes exported metadata.
- Slot, session, and controller workflows must be designed for multitimbral live use.
- Zynthian integration should be treated as a primary platform contract, not an afterthought.
- The first live Pi should be treated as a legacy ARMv6 runtime target, not the primary development build host.
- Prefer cross-compile + deploy for the Pi 1 B+ baseline unless a later task explicitly changes that decision.
- Prefer `Ubuntu 22.04` under WSL as the first supported local Windows cross-build host baseline.

## Updater Rules
- `piFartBox` must maintain a documented codebase update path for the deployed Pi system.
- The preferred update transport is USB mass storage media, not direct USB gadget/device mode.
- Do not assume Raspberry Pi 1 B+ can be updated by plugging it into a host computer as a USB device.
- The updater workflow must remain simple enough for field updates without network dependency.
- Repository changes that alter deployment, install layout, or runtime startup must update `docs/updater-system.md` in the same patch.
- Updater tooling should preserve:
  - a clear install root
  - a predictable backup/rollback path
  - a recorded deployed revision or bundle identifier
  - minimal operator steps
- When cross-built artifacts are introduced, document how artifact deployment relates to source-repo deployment and update scripts/docs together.
- Updater scripts and manifests must be versioned in the repository and referenced from task plans when changed.

## Local Build Host Rules
- The preferred local Windows build path is WSL, not native Windows compilation.
- Keep the first supported WSL host baseline documented in `docs/wsl-cross-build-host.md`.
- If build host assumptions change, update `docs/wsl-cross-build-host.md`, `docs/cross-compile-and-deploy.md`, and any helper scripts in the same patch.

## Definition of Done For Substantial Tasks
- A full plan exists in `docs/plans/`.
- The task is indexed correctly in `docs/todo.md` while active.
- Code and docs align with the task plan.
- Interfaces and assumptions are documented.
- The task is moved to `docs/completedTasks.md` when finished.
