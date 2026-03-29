# TASK-0016 - Pi-Hosted Web UI and Audio Probe

## Objective
Build the first real Pi-hosted web workbench for `piFartBox` and expose enough runtime/audio status to use it as the foundation for live device-side testing.

## Success Criteria
- The runtime exports a machine-readable status file in addition to the current text summary.
- The Linux host layer reports basic ALSA/proc-based audio facts from the Pi.
- The Pi-hosted web root presents a modular-workbench style UI rather than a placeholder page.
- The web page renders live runtime, slot, controller, module, and audio probe data from the Pi.
- The page preserves visual continuity with the old Pico synth workbench direction.

## Constraints
- Keep the runtime status export lightweight and file-based for this milestone.
- Do not introduce browser-side audio synthesis.
- Do not require the full control API server implementation yet.
- Keep the audio probe non-invasive and safe on the Pi.

## Implementation Design
- extend `LinuxHost` with lightweight `/proc/asound` probing
- write both text and JSON runtime status artifacts from `pi_fartbox_runtime`
- expose the JSON status file through nginx using a direct file alias
- replace the placeholder `web/index.html` with a themed local modular workbench page
- have the page fetch runtime JSON and render:
  - slot overview
  - controller pages
  - starter module palette
  - audio probe status
  - runtime notes / next steps

## Interfaces / Types Affected
- `platform/linux`
- `apps/runtime`
- `deploy/nginx/pifartbox.conf`
- `web/index.html`
- runtime docs

## Test Plan
- native on-device build succeeds after the runtime changes
- `pi_fartbox_runtime --oneshot` writes both text and JSON status files
- nginx serves the updated web page
- nginx serves the runtime JSON alias
- the web page loads on the Pi and renders the returned status payload

## Assumptions / Defaults
- the current live Pi remains the primary validation target
- nginx remains the static hosting layer
- the runtime status JSON path can live under `/var/lib/piFartBox`

## Out of Scope
- full browser patch-cable editor behavior
- realtime websocket control updates
- actual ALSA render callback implementation
