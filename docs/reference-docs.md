# Reference Docs

This file is the compact external reference index for `piFartBox`.

Use it as a link manifest, not as a place for copied research dumps.

## Zynthian
- Zynthian Development
  - https://wiki.zynthian.org/index.php/Zynthian_Development
  - Why: main development entry point for understanding Zynthian structure and workflows.
- Creating a New Zynthian Synth Engine
  - https://wiki.zynthian.org/index.php/Creating_a_new_Zynthian_Synth_Engine
  - Why: most relevant starting point for native engine integration planning.
- Zynthian Web Configuration Tool User Guide
  - https://wiki.zynthian.org/index.php/Zynthian_Web_Configuration_Tool_User_Guide
  - Why: explains the configuration/admin model users actually see.
- Zynthian UI User Guide
  - https://wiki.zynthian.org/index.php?title=Zynthian_UI_User%27s_Guide_-_Oram
  - Why: helps align our runtime and preset workflows with the native appliance UX.
- ZynthianOS repository
  - https://github.com/zynthian/ZynthianOS
  - Why: operating-system integration and deployment baseline.
- Zynthian UI repository
  - https://github.com/zynthian/zynthian-ui
  - Why: real engine integration patterns and runtime-facing code.
- Adding LV2 plugins
  - https://wiki.zynthian.org/index.php/Adding_LV2_plugins
  - Why: useful if our integration path touches Zynthian plugin conventions.

## Raspberry Pi 1 B+ and Audio
- Raspberry Pi board overview
  - https://static.raspberrypi.org/files/products/Board_Overview.pdf
  - Why: baseline hardware capabilities and model constraints.
- Raspberry Pi operating system documentation
  - https://www.raspberrypi.com/documentation/installation/operating-system.md
  - Why: useful for host setup assumptions and supported OS patterns.
- Choosing an audio option
  - https://pip-assets.raspberrypi.com/categories/1259-audio-camera-and-display/documents/RP-008124-WP/Choosing-an-Audio-option
  - Why: compares audio-output paths and highlights quality/performance tradeoffs.
- Using the I2S peripherals on Raspberry Pi SBCs
  - https://pip-assets.raspberrypi.com/categories/1259-audio-camera-and-display/documents/RP-009699-WP-1-Using%2520the%2520I2S%2520peripherals%2520on%2520Raspberry%2520Pi%2520SBCs.pdf
  - Why: important if we move away from onboard analog output.

## Linux Audio Stack
- JACK main site
  - https://jackaudio.org/
  - Why: core low-latency Linux audio architecture reference.
- JACK API docs
  - https://jackaudio.org/api/index.html
  - Why: reference for native audio-engine integration.
- ALSA PCM docs
  - https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
  - Why: playback/capture device API reference.
- ALSA Sequencer docs
  - https://www.alsa-project.org/alsa-doc/alsa-lib/seq.html
  - Why: MIDI/event routing reference.
- LV2 main site
  - https://lv2plug.in/
  - Why: plugin/engine model reference that may influence our Zynthian integration shape.
- LV2 C documentation
  - https://lv2plug.in/c/html/
  - Why: low-level spec/API documentation.
- LV2 specification repository
  - https://github.com/lv2/lv2
  - Why: authoritative spec source.

## Controller Integration
- Novation SL MkIII downloads
  - https://downloads.novationmusic.com/novation/sl-mkiii/61sl-mkiii
  - Why: official manuals, programmer guide, and controller support material.

## Existing Project References
- Current extracted SL MkIII programmer notes
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\slmkiii-programmers-guide-extracted.md`
  - Why: already-trimmed local working reference for InControl message structure.
- Current SL MkIII map proposal
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\docs\slmkiii-midi-map-proposal.md`
  - Why: captures controller mapping ideas and lessons from the existing project.
- Current SysEx helper implementation
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\slmkiii.js`
  - Why: compact reference implementation of the SysEx payload builders.
- Current Web MIDI / SL MkIII integration runtime
  - `C:\Users\Erik Christensen\Desktop\pico2w-synth\web\js\main.js`
  - Why: source of truth for the current working controller behavior.
