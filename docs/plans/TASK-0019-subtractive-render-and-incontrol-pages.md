# TASK-0019 - Subtractive Render And InControl Pages

## Objective
Replace the standalone tone-path milestone with the first real subtractive synth render path, and make the focused instrument page produce dynamic SL MkIII InControl display state directly from the engine-owned exported controls.

## Success Criteria
- The runtime can render the starter subtractive instrument through ALSA instead of only silence or a fixed tone.
- The engine owns live parameter values for compiled instruments and uses them during audio rendering.
- A simple demo-performance path exists so the new synth render can be heard on device before full MIDI I/O lands.
- The controller layer exposes the active instrument page as dynamic InControl display columns derived from exported controls and current parameter values.
- Runtime JSON and the Pi-hosted workbench show the active InControl page preview.

## Constraints
- Keep the first voice architecture simple and deterministic.
- Use the existing starter subtractive instrument definition as the first target.
- Do not add a full modulation matrix or open modular editor in this task.
- Keep the controller display logic derived from engine/runtime state rather than browser-owned state.

## Implementation Design
- add engine-owned slot parameter storage and a small subtractive voice renderer
- add a demo note pattern path for audible validation without depending on full MIDI hardware input yet
- update the ALSA playback engine to accept an engine render callback
- extend the controller core to build an active InControl page snapshot from the focused instrument page
- generate real Novation layout/property SysEx payloads from that display snapshot
- expose the active display page and payload count in runtime JSON and the workbench UI

## Interfaces / Types Affected
- `engine/include/pi_fartbox/engine/runtime_model.hpp`
- `engine/include/pi_fartbox/engine/engine_runtime.hpp`
- `engine/src/engine_runtime.cpp`
- `platform/linux/include/pi_fartbox/platform/linux_host.hpp`
- `platform/linux/src/linux_host.cpp`
- `controller-mapper/include/pi_fartbox/controller/controller_core.hpp`
- `controller-mapper/src/controller_core.cpp`
- `apps/runtime/src/main.cpp`
- `web/index.html`

## Test Plan
- build natively on the Pi
- run `pi_fartbox_runtime --synth-demo 4` and confirm the subtractive render path opens and runs
- verify runtime JSON shows an active InControl page with exported control labels and values
- verify the workbench displays the active InControl preview from runtime JSON
- verify the long-running service still starts and writes valid runtime JSON

## Assumptions / Defaults
- the starter instrument remains the first subtractive target
- the focused slot remains slot 1 by default
- the first generated InControl page is based on the focused slot’s current exported-control page
- the first synth render path can be modest and still count as success

## Out of Scope
- live ALSA MIDI device input
- physical SL MkIII port output integration
- free-form module graph editing
