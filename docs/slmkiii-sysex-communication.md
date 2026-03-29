# Novation SL MkIII SysEx Communication Notes

This note captures the working SL MkIII InControl communication pattern proven in the current `pico2w-synth` browser controller.

It is not the final controller UX for `piFartBox`, but it is the best currently verified implementation reference.

## Primary Rule
All screen and LED device messages must go to the SL MkIII `InControl USB` port.

Local reference:
- `C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\slmkiii-programmers-guide-extracted.md`

Important related behavior:
- the browser must obtain Web MIDI access with `sysex: true`
- without SysEx permission, input can still work, but screen/LED feedback will not
- Windows may expose the InControl ports with generic names like `MIDIIN2` / `MIDIOUT2`, so port detection should not rely only on the literal word `InControl`

## Proven Current Source Locations
- SysEx helper builders:
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\slmkiii.js`
- SL MkIII constants, pages, CC labels, LED IDs:
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\config.js`
- Web MIDI permission, port selection, LCD sync, notifications, LED feedback:
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\main.js`

## Current Message Model
The current implementation uses the Novation InControl SysEx header:

```text
F0 00 20 29 02 0A 01 ... F7
```

Current command IDs in use:
- `0x01` set layout
- `0x02` set properties
- `0x03` set LED
- `0x04` notification

Current helper implementation names:
- `buildSlmkiiiSetLayoutSysex(...)`
- `buildSlmkiiiSetPropertiesSysex(...)`
- `buildSlmkiiiNotificationSysex(...)`
- `buildSlmkiiiSetLedRgbSysex(...)`

## What the Current Implementation Successfully Does
### 1. Screen layout sync
The controller switches the device into knob-layout mode before pushing text/value properties for the eight knob columns.

Relevant code:
- `buildSlmkiiiLcdSyncPayloads()`
- `sendSlmkiiCcLabelSysex()`

### 2. Text and value updates
The implementation pushes:
- row 1 text
- row 3 text
- knob-value field data

This is how the MkIII screen follows the current parameter labels and live values.

### 3. Center-screen preview
The current code also drives the center screen with focused preview content for the active parameter area:
- oscillator
- filter
- envelope
- LFO
- FX / system context

Relevant code:
- `buildSlmkiiiCenterPreviewPayload(...)`
- `buildSlmkiiiPreviewForParam(...)`

### 4. Notification messages
Short transient notifications are sent for mode changes and controller feedback using the dedicated notification command.

Relevant code:
- `sendSlmkiiiNotificationThrottled(...)`

### 5. LED color updates
The current code drives LED feedback by sending RGB SysEx messages using the Novation LED SysEx IDs from the programmer guide.

Relevant code:
- `syncSlmkiiiMappedLeds()`
- `buildSlmkiiiSetLedRgbSysex(...)`

## Current Device Assumptions
### Relative rotary knobs
InControl knobs `CC21..28` send two's-complement relative deltas, not absolute values.

The existing implementation handles:
- small positive deltas
- small negative deltas
- auto-detection of relative mode for likely SL MkIII control ports

Relevant code:
- `decodeRelativeMidiDelta(...)`
- `webMidiEffectiveCcMode()`

### Buttons and faders
- buttons send `127` on press and `0` on release
- faders send absolute `0..127`

The current browser controller uses this to mix:
- relative knob handling
- absolute fader handling
- binary button actions

## Practical Lessons We Should Preserve
### 1. Port detection must be tolerant
Do not assume port names literally include `InControl`.

Observed practical issue:
- the working control output was discovered as `MIDI OUT 2`, not an explicitly named InControl port

For `piFartBox`, controller-port discovery should support:
- exact `InControl` names when available
- generic `MIDIIN2` / `MIDIOUT2` style names
- explicit user override/selection

### 2. SysEx permission is a separate runtime state
The transport must clearly expose whether:
- MIDI input is active
- MIDI output is selected
- SysEx permission is granted

Without that separation, the system can appear “mostly connected” while screen/LED feedback is silently unavailable.

### 3. Controller-specific logic should stay isolated
The best part of the current refactor is that SysEx builders live in a dedicated helper file and controller constants live in config.

For `piFartBox`, keep SL MkIII behavior isolated behind a dedicated controller adapter rather than scattering it through general UI/runtime code.

### 4. UI state should not own hardware truth
The future repo should invert the old browser-first model:
- engine/session state is canonical
- controller feedback is derived from engine/session state
- browser and SL MkIII are both clients of the same runtime truth

## Likely Carry-Forward Responsibilities in `piFartBox`
The future controller layer should likely preserve these concepts:
- output-port selection and capability detection
- explicit SysEx-enabled state
- page-driven knob-screen layout sync
- live value feedback per focused slot/instrument
- LED color/state feedback for slot and action availability
- a dedicated SL MkIII adapter module rather than embedding controller logic in generic UI code

## Explicit Non-Goals for This Note
- This document does not define the final `piFartBox` CC/page map.
- This document does not require the old Pico-era controller layout to be preserved.
- This document is a communication-implementation reference, not a final workstation UX spec.
