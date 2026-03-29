#include "pi_fartbox/engine/engine_runtime.hpp"

#include <algorithm>

namespace pi_fartbox::engine {

EngineRuntime::EngineRuntime(EngineRuntimeConfig config)
    : config_(config), compiler_(registry_) {
  ensure_slot_defaults();
  if (const auto instrument = compiler_.compile(starter_subtractive_instrument_definition())) {
    (void)assign_compiled_instrument(1, *instrument);
  }
}

auto EngineRuntime::config() const noexcept -> const EngineRuntimeConfig& {
  return config_;
}

auto EngineRuntime::registry() noexcept -> DeviceRegistry& {
  return registry_;
}

auto EngineRuntime::registry() const noexcept -> const DeviceRegistry& {
  return registry_;
}

auto EngineRuntime::compiler() const noexcept -> const InstrumentCompiler& {
  return compiler_;
}

auto EngineRuntime::slot_runtimes() const noexcept -> const std::vector<SlotRuntime>& {
  return slot_runtimes_;
}

auto EngineRuntime::focused_slot() const noexcept -> const SlotRuntime* {
  const auto it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [](const SlotRuntime& slot) { return slot.focused; });
  return (it != slot_runtimes_.end()) ? &(*it) : nullptr;
}

auto EngineRuntime::subsystem_name() const noexcept -> std::string_view {
  return "engine-core";
}

auto EngineRuntime::dispatch_note_event(const NoteEvent& event) -> bool {
  auto slot_it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [&event](const SlotRuntime& slot) {
    return slot.midi_channel == event.midi_channel;
  });
  if (slot_it == slot_runtimes_.end() || !slot_it->compiled_instrument.has_value()) {
    return false;
  }

  auto& slot = *slot_it;
  if (event.type == NoteEventType::note_off) {
    for (auto& voice : slot.voices) {
      if (voice.state == VoiceState::active && voice.midi_note == event.midi_note) {
        voice.state = VoiceState::released;
      }
    }
    return true;
  }

  if (auto* voice = allocate_voice(slot, event)) {
    voice->state = VoiceState::active;
    voice->midi_note = event.midi_note;
    voice->midi_channel = event.midi_channel;
    voice->velocity = event.velocity;
    voice->generation = ++voice_generation_counter_;
    return true;
  }

  return false;
}

auto EngineRuntime::focus_slot(std::uint32_t slot_id) -> bool {
  bool found = false;
  for (auto& slot : slot_runtimes_) {
    slot.focused = slot.slot_id == slot_id;
    found = found || slot.focused;
  }
  return found;
}

auto EngineRuntime::assign_compiled_instrument(std::uint32_t slot_id, CompiledInstrument instrument) -> bool {
  auto slot_it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [slot_id](const SlotRuntime& slot) {
    return slot.slot_id == slot_id;
  });
  if (slot_it == slot_runtimes_.end()) {
    return false;
  }

  auto& slot = *slot_it;
  slot.compiled_instrument = std::move(instrument);
  slot.voices.clear();
  slot.voices.reserve(config_.default_polyphony);
  for (std::uint32_t voice_id = 0; voice_id < config_.default_polyphony; ++voice_id) {
    slot.voices.push_back(VoiceInstance{.voice_id = voice_id});
  }
  return true;
}

auto EngineRuntime::default_slot_count() noexcept -> std::uint32_t {
  return 4;
}

auto EngineRuntime::ensure_slot_defaults() -> void {
  slot_runtimes_.clear();
  slot_runtimes_.reserve(config_.slot_count);
  for (std::uint32_t index = 0; index < config_.slot_count; ++index) {
    slot_runtimes_.push_back(SlotRuntime{
        .slot_id = index + 1,
        .midi_channel = static_cast<std::uint8_t>(index + 1),
        .focused = index == 0,
    });
  }
}

auto EngineRuntime::allocate_voice(SlotRuntime& slot, const NoteEvent& event) -> VoiceInstance* {
  auto idle = std::find_if(slot.voices.begin(), slot.voices.end(), [](const VoiceInstance& voice) {
    return voice.state == VoiceState::idle;
  });
  if (idle != slot.voices.end()) {
    return &(*idle);
  }

  auto released = std::min_element(slot.voices.begin(), slot.voices.end(), [](const VoiceInstance& left, const VoiceInstance& right) {
    if (left.state != VoiceState::released) {
      return false;
    }
    if (right.state != VoiceState::released) {
      return true;
    }
    return left.generation < right.generation;
  });
  if (released != slot.voices.end() && released->state == VoiceState::released) {
    return &(*released);
  }

  auto active = std::min_element(slot.voices.begin(), slot.voices.end(), [](const VoiceInstance& left, const VoiceInstance& right) {
    return left.generation < right.generation;
  });
  return (active != slot.voices.end()) ? &(*active) : nullptr;
}

}  // namespace pi_fartbox::engine
