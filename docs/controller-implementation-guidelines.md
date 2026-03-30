# Controller Implementation Guidelines

## Purpose
This document turns the useful controller and synth-control lessons from the old `pico2w-synth` project into implementation guidance for `piFartBox`.

It is not a request to copy the old controller scheme directly.

The goal is:
- keep the best proven SL MkIII InControl patterns
- avoid the old browser-first ownership model
- define a hybrid controller model for `piFartBox`
- make future controller work faster and more consistent

## Reference Sources Reviewed
Old project root:
- [README](C:\Users\Erik Christensen\Desktop\pico2w-synth\README.md)

Old project docs:
- [midi-cc-map.md](C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\midi-cc-map.md)
- [slmkiii-midi-map-proposal.md](C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\slmkiii-midi-map-proposal.md)
- [slmkiii-programmers-guide-extracted.md](C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\slmkiii-programmers-guide-extracted.md)

Old project source areas:
- [synth_engine.cpp](C:\Users\Erik Christensen\Desktop\pico2w-synth\src\synth\synth_engine.cpp)
- [midi_mapper.cpp](C:\Users\Erik Christensen\Desktop\pico2w-synth\src\midi\midi_mapper.cpp)
- [midi_input.cpp](C:\Users\Erik Christensen\Desktop\pico2w-synth\src\midi\midi_input.cpp)
- [slmkiii.js](C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\slmkiii.js)
- [main.js](C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\main.js)
- [config.js](C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\config.js)

## What Worked Well In The Old Project

### 1. Clear instrument pages by function
The old SL MkIII implementation grouped the eight encoder columns into functional pages:
- `OSC`
- `FILT/LFO`
- `AMP/PITCH`
- `CHORUS/FX`
- `DELAY/RVB`

This was strong because:
- the pages were easy to learn
- each page had a coherent color/theme
- the screen text stayed musical rather than technical
- the pages matched how a player thinks about sound design

This pattern should carry forward.

### 2. Controller screen columns were rich, not just labels
Each old column definition carried:
- CC number
- target parameter
- row 1 label
- row 3 value text
- value `0..127`
- per-column colors

That was the right level of richness for InControl.

For `piFartBox`, this should become engine-owned display metadata plus runtime value state, not browser-owned config state.

### 3. Hybrid “fixed transport, dynamic content” already emerged
Even in the old browser controller, the most effective parts were not purely dynamic or purely hard-wired:
- page navigation buttons were fixed
- options/overlay behavior was fixed
- the actual screen contents changed by page and focused function
- LEDs were partly fixed by physical role and partly updated dynamically

This is the right direction for `piFartBox`.

### 4. Tolerant SL MkIII InControl detection mattered
The old system correctly learned that Windows might expose the InControl port as generic names like:
- `MIDIIN2`
- `MIDIOUT2`

So exact string matching on `InControl` was not enough.

This must be preserved in `piFartBox`.

### 5. Proven SysEx message model already exists
The old project successfully used the Novation MkIII SysEx header:

`F0 00 20 29 02 0A 01 ... F7`

And these command families:
- set layout
- set properties
- set LED
- notification

Those helpers should remain the baseline communication model in `piFartBox`.

### 6. Visual grouping by synth area was effective
The old controller pages used strong thematic grouping:
- oscillator pages in blue
- filter pages in red
- LFO/mod pages in green
- amp/fx/system pages in other distinct accents

This was valuable because it made the controller readable at a glance.

This should carry forward.

## What Should Not Be Carried Forward Directly

### 1. Browser-owned controller truth
In the old project, the browser controller effectively owned too much:
- page definitions
- live hardware state
- parameter formatting
- controller feedback scheduling

For `piFartBox`, this must be inverted:
- engine/runtime/session state is canonical
- controller state is derived from engine/runtime/session state
- browser and SL MkIII are both clients

### 2. Hard-wiring every page as a static JS table
The old fixed page tables were useful for rapid development, but they do not scale to:
- multiple instruments
- dynamic slot focus
- module-based instruments
- generated pages per instrument/module

`piFartBox` should use a hybrid model:
- some controller regions are hard-wired by workstation role
- some controller regions are dynamically populated from the focused instrument/module exports

### 3. One giant flat synth mapping
The old synth was ultimately one instrument with many pages.

`piFartBox` is different:
- multiple slots
- multiple instruments
- curated modules
- session/workstation behavior

So the controller must be organized around:
- workstation layer
- slot layer
- focused instrument layer
- focused module/page layer

## Core piFartBox Controller Model

The controller should be split into four layers.

### 1. Hard-Wired Workstation Layer
These controls should always mean the same thing:
- slot focus/select
- page navigation
- system/diagnostics
- transport/utility
- shift/options overlays

This keeps the instrument system understandable while everything else changes.

### 2. Semi-Fixed Performance Layer
These controls should have a stable role, but their exact target depends on the focused slot or session:
- slot mixer
- slot mute
- slot arm
- slot send
- scene/performance actions later

### 3. Dynamic Instrument Layer
These controls should be populated from the focused instrument’s exported controls and preferred pages.

Examples:
- oscillator page
- filter page
- envelope page
- FX page

### 4. Dynamic Module/Overlay Layer
These are special-case overlays for focused module details or deeper edits:
- oscillator-specific modulation overlay
- arp overlay
- delay sync/options overlay
- module-specific advanced page

## Recommended Hybrid Mapping

### Encoders `CC21..28`
Role:
- dynamic
- always bound to the active instrument page or active module overlay

Rules:
- the page family is fixed by the workstation
- the actual labels, values, and targets are generated from the focused instrument/module exports
- default order should follow the instrument’s declared page order
- colors should reflect the active module family

This matches the strongest part of the old project and is already close to the current `piFartBox` direction.

### Sliders `CC41..48`
This should be a hybrid fixed/dynamic split.

Recommended rule:
- `CC41..44` (`sliders 1..4`): reserved for the focused instrument
- `CC45..48` (`sliders 5..8`): reserved for global slot mix control

Detailed recommendation:
- `slider 1..4`:
  - dynamically assigned from the focused instrument
  - prefer high-value performance parameters such as:
    - oscillator or source levels
    - filter amount
    - chorus mix
    - delay mix
    - reverb mix
  - instrument definitions should declare a preferred `performance_fader` set
- `slider 5..8`:
  - fixed to slot volumes
  - `slot 1 volume`
  - `slot 2 volume`
  - `slot 3 volume`
  - `slot 4 volume`

Why:
- this preserves live workstation stability
- it still gives the active instrument tactile performance control
- it avoids burying the global mix in page changes

This should be treated as a first-class design rule going forward.

### Soft Buttons `CC51..58`
Recommended role:
- dynamic instrument/page actions
- but constrained to a stable “current page action row”

Examples:
- waveform stepping
- mode cycling
- parameter reset
- enable/bypass actions
- page-specific toggles

Rules:
- these should not be fully arbitrary per instrument
- define a small set of allowed action kinds so users learn the row

### Button Rows `CC59..66` and `CC67..74`
The old project already treated these rows as grouped clusters.

For `piFartBox`, keep that principle:
- top row: mode/select/context actions
- second row: utility/reset/performance actions

Recommended hard-wired meanings:
- page family changes
- slot actions
- quick module actions
- panic/reset
- page-local utilities

Do not make both rows fully dynamic.
They should remain mostly workstation-stable.

### Page Navigation `CC81/82`
Keep these fixed as instrument/module page navigation for the encoder screen pages.

This was one of the better decisions from the old project.

### Options / Shift
Keep these as overlays/modifiers, not as primary operation modes.

This is how we support deeper module edits without losing the core page system.

## Recommended Page Population Strategy

### Workstation Pages
Always fixed:
- slot overview
- mixer
- controller/system

### Instrument Pages
Generated from exported controls.

Each instrument should declare:
- page order
- preferred page titles
- preferred short labels
- preferred control type
- optional color/theme hint
- optional performance-fader priority

### Module Overlays
Defined by module type, not individual instrument instances.

Examples:
- oscillator advanced page
- filter advanced page
- arp page
- FX sync/options page

This keeps the system consistent while still allowing depth.

## Recommended Screen Model

### Row 1
Short control label:
- `Wave`
- `Cut`
- `Atk`
- `Mix`

### Row 3
Current formatted value:
- `Saw`
- `4.2kHz`
- `250ms`
- `64%`

### Knob value field
Still send the normalized `0..127` knob value for the native MkIII knob graphic.

### Center or preview area
Use it for the currently focused page or selected control family:
- page title
- focused slot/instrument
- optional compact module visualization later

Do not try to copy the old browser graphics directly.
Use compact semantic previews instead.

## Recommended LED Model

### Fixed-role LEDs
Always indicate workstation state:
- slot focus
- page nav availability
- options/shift held
- mixer/system modes

### Dynamic LEDs
Show page/action state:
- enabled/bypassed
- active module function
- current page action buttons
- armed overlay actions

### Color policy
Preserve strong color families:
- oscillator/source: blue
- filter/tone: red
- LFO/mod: green
- amp/mix: amber
- FX: magenta/violet
- workstation/system: white or neutral

## Synth Parameter Guidance From The Old Engine
Useful synth-control lessons from the old `src/synth/synth_engine.cpp`:
- performance controls often benefited from perceptual remapping
  - filter ranges
  - LFO rate curves
  - sub/noise shaping
- oscillator shape/wave selections were intentionally quantized into musical buckets
- FX and filter controls worked best when mapped to performance-friendly ranges rather than raw DSP ranges

For `piFartBox`, this means:
- instrument/module exports should expose player-friendly ranges
- the controller should display musical values, not raw internal values
- performance mappings should be authored intentionally per module type

## MIDI Routing Lessons From The Old Firmware
Useful lessons from the old `src/midi` code:
- MIDI routing must stay deterministic
- controller actions and note events should not share ambiguous ownership
- fixed channel contracts are easier to reason about than dynamically shifting channels

For `piFartBox`, keep:
- fixed slot channel mapping for the first milestone
- controller focus separate from note-routing channel
- one normalized routing layer before engine event dispatch

## Concrete Rules To Implement Next

### Rule 1
`CC41..44` are focused-instrument performance faders.

### Rule 2
`CC45..48` are always global slot volume faders for slots `1..4`.

### Rule 3
`CC21..28` always reflect the active focused instrument/module page.

### Rule 4
The active page must come from engine-owned page/export metadata, not browser-owned page tables.

### Rule 5
The SL MkIII screen payload must be built from runtime state:
- focused slot
- focused instrument
- active page
- current parameter values

### Rule 6
The controller must support both:
- generated instrument pages
- hard-wired workstation pages

### Rule 7
Dynamic controller content should be limited to curated regions, not the whole surface.

## Suggested Near-Term Implementation Order
1. Implement the slider split:
   - dynamic `1..4`
   - global slot volume `5..8`
2. Add engine/runtime metadata for `performance_faders`.
3. Add controller page families:
   - `workstation`
   - `instrument`
   - `module overlay`
4. Add active page switching from the controller runtime.
5. Add live SL MkIII output transport so the existing runtime-generated SysEx preview becomes real hardware output.
6. Add page-local action rows using curated action kinds.

## Bottom Line
The old project proved three important things:
- the SL MkIII InControl display model works well when pages are musically grouped
- live labels, values, and LED colors make the controller genuinely useful
- a hybrid fixed-plus-dynamic controller is better than either extreme

For `piFartBox`, the right implementation is:
- hard-wired workstation structure
- dynamic instrument/module population inside that structure
- engine-owned controller truth
- fixed global slot-mix controls
- dynamic focused-instrument performance controls

That should be treated as the controller implementation baseline from here.
