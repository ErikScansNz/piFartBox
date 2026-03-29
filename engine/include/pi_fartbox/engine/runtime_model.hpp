#pragma once

#include "pi_fartbox/engine/device_schema.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pi_fartbox::engine {

enum class CuratedModSource {
  amp_envelope,
  filter_envelope,
  lfo,
  velocity
};

enum class CuratedModTarget {
  vca_level,
  filter_cutoff,
  oscillator_pitch
};

struct CuratedModRoute {
  CuratedModSource source = CuratedModSource::amp_envelope;
  CuratedModTarget target = CuratedModTarget::vca_level;
  double depth = 1.0;
};

struct DeviceExecutionNode {
  std::string device_id;
  std::string type_id;
  DeviceCategory category = DeviceCategory::utility;
};

struct RuntimeGraph {
  std::vector<DeviceExecutionNode> event_nodes;
  std::vector<DeviceExecutionNode> mod_nodes;
  std::vector<DeviceExecutionNode> audio_nodes;
};

struct VoiceDefinition {
  std::string composite_device_id;
  std::uint32_t polyphony = 8;
  std::vector<CuratedModRoute> mod_routes;
};

enum class VoiceState {
  idle,
  active,
  released
};

struct VoiceInstance {
  std::uint32_t voice_id = 0;
  VoiceState state = VoiceState::idle;
  std::optional<std::uint8_t> midi_note;
  std::optional<std::uint8_t> midi_channel;
  double velocity = 0.0;
  std::uint64_t generation = 0;
};

struct CompiledInstrumentPage {
  std::string id;
  std::string title;
  std::vector<ExportedControlDefinition> controls;
};

struct CompiledInstrument {
  InstrumentDefinition instrument;
  RuntimeGraph runtime_graph;
  VoiceDefinition voice_definition;
  std::vector<ExportedControlDefinition> exported_controls;
  std::vector<CompiledInstrumentPage> generated_pages;
};

enum class NoteEventType {
  note_on,
  note_off
};

struct NoteEvent {
  NoteEventType type = NoteEventType::note_on;
  std::uint8_t midi_channel = 1;
  std::uint8_t midi_note = 60;
  double velocity = 1.0;
};

struct SlotRuntime {
  std::uint32_t slot_id = 1;
  std::uint8_t midi_channel = 1;
  bool focused = false;
  bool enabled = true;
  bool muted = false;
  double mix_level = 1.0;
  double pan = 0.0;
  double send_level = 0.0;
  std::optional<CompiledInstrument> compiled_instrument;
  std::vector<VoiceInstance> voices;
};

[[nodiscard]] auto starter_subtractive_instrument_definition() -> InstrumentDefinition;

}  // namespace pi_fartbox::engine
