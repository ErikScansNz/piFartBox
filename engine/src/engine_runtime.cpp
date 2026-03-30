#include "pi_fartbox/engine/engine_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace pi_fartbox::engine {

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi_v<double>;
constexpr std::uint8_t kDemoNotes[] = {48, 55, 60, 67};
const std::string kOscWaveformKey = "oscillator.waveform";
const std::string kOscShapeKey = "oscillator.shape";
const std::string kFilterCutoffKey = "filter.cutoff";
const std::string kFilterResonanceKey = "filter.resonance";
const std::string kVcaLevelKey = "vca.level";
const std::string kAmpAttackKey = "amp_env.attack_ms";
const std::string kAmpDecayKey = "amp_env.decay_ms";
const std::string kAmpSustainKey = "amp_env.sustain";
const std::string kAmpReleaseKey = "amp_env.release_ms";

auto clamp(double value, double minimum, double maximum) -> double {
  return std::max(minimum, std::min(maximum, value));
}

auto note_to_frequency(std::uint8_t midi_note) -> double {
  return 440.0 * std::pow(2.0, (static_cast<double>(midi_note) - 69.0) / 12.0);
}

auto envelope_step_time(double milliseconds, std::uint32_t sample_rate_hz) -> double {
  const auto samples = std::max(1.0, milliseconds * 0.001 * static_cast<double>(sample_rate_hz));
  return 1.0 / samples;
}

auto waveform_sample(std::int32_t waveform, double phase, double shape) -> double {
  const auto wrapped = phase - std::floor(phase);
  switch (waveform) {
    case 1: {
      const auto duty = clamp(0.1 + shape * 0.8, 0.1, 0.9);
      return wrapped < duty ? 1.0 : -1.0;
    }
    case 2:
      return 4.0 * std::abs(wrapped - 0.5) - 1.0;
    case 3:
      return std::sin(kTwoPi * wrapped);
    case 0:
    default: {
      const auto skew = clamp(shape, 0.0, 1.0);
      const auto saw = 2.0 * wrapped - 1.0;
      const auto bent = wrapped < skew ? (wrapped / std::max(0.01, skew)) : (1.0 - wrapped) / std::max(0.01, 1.0 - skew);
      return clamp((saw * 0.7) + ((bent * 2.0 - 1.0) * 0.3), -1.0, 1.0);
    }
  }
}

auto update_envelope(VoiceInstance& voice,
                     double attack_ms,
                     double decay_ms,
                     double sustain_level,
                     double release_ms,
                     std::uint32_t sample_rate_hz) -> double {
  const auto attack_step = envelope_step_time(attack_ms, sample_rate_hz);
  const auto decay_step = envelope_step_time(decay_ms, sample_rate_hz);
  const auto release_step = envelope_step_time(release_ms, sample_rate_hz);
  const auto sustain = clamp(sustain_level, 0.0, 1.0);

  switch (voice.envelope_stage) {
    case EnvelopeStage::idle:
      voice.envelope_level = 0.0;
      break;
    case EnvelopeStage::attack:
      voice.envelope_level = std::min(1.0, voice.envelope_level + attack_step);
      if (voice.envelope_level >= 0.999) {
        voice.envelope_stage = EnvelopeStage::decay;
      }
      break;
    case EnvelopeStage::decay:
      voice.envelope_level = std::max(sustain, voice.envelope_level - decay_step);
      if (voice.envelope_level <= sustain + 0.0001) {
        voice.envelope_stage = EnvelopeStage::sustain;
      }
      break;
    case EnvelopeStage::sustain:
      voice.envelope_level = sustain;
      break;
    case EnvelopeStage::release:
      voice.envelope_level = std::max(0.0, voice.envelope_level - release_step);
      if (voice.envelope_level <= 0.0001) {
        voice.envelope_level = 0.0;
        voice.envelope_stage = EnvelopeStage::idle;
        voice.state = VoiceState::idle;
        voice.midi_note.reset();
        voice.midi_channel.reset();
      }
      break;
  }
  return voice.envelope_level;
}

}  // namespace

EngineRuntime::EngineRuntime(EngineRuntimeConfig config)
    : config_(config), compiler_(registry_), demo_mode_enabled_(config.demo_mode_enabled) {
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

auto EngineRuntime::slot_runtimes_snapshot() const -> std::vector<SlotRuntime> {
  std::lock_guard<std::mutex> lock(mutex_);
  return slot_runtimes_;
}

auto EngineRuntime::focused_slot_id() const noexcept -> std::uint32_t {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [](const SlotRuntime& slot) { return slot.focused; });
  return (it != slot_runtimes_.end()) ? it->slot_id : 1;
}

auto EngineRuntime::subsystem_name() const noexcept -> std::string_view {
  return "engine-core";
}

auto EngineRuntime::dispatch_note_event(const NoteEvent& event) -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
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
        voice.envelope_stage = EnvelopeStage::release;
      }
    }
    return true;
  }

  if (auto* voice = allocate_voice(slot, event)) {
    voice->state = VoiceState::active;
    voice->envelope_stage = EnvelopeStage::attack;
    voice->midi_note = event.midi_note;
    voice->midi_channel = event.midi_channel;
    voice->velocity = event.velocity;
    voice->generation = ++voice_generation_counter_;
    voice->phase = 0.0;
    voice->envelope_level = 0.0;
    voice->filter_state = 0.0;
    return true;
  }

  return false;
}

auto EngineRuntime::focus_slot(std::uint32_t slot_id) -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
  bool found = false;
  for (auto& slot : slot_runtimes_) {
    slot.focused = slot.slot_id == slot_id;
    found = found || slot.focused;
  }
  return found;
}

auto EngineRuntime::assign_compiled_instrument(std::uint32_t slot_id, CompiledInstrument instrument) -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
  auto* slot = find_slot(slot_id);
  if (!slot) {
    return false;
  }

  slot->compiled_instrument = std::move(instrument);
  slot->voices.clear();
  slot->voices.reserve(config_.default_polyphony);
  for (std::uint32_t voice_id = 0; voice_id < config_.default_polyphony; ++voice_id) {
    slot->voices.push_back(VoiceInstance{.voice_id = voice_id});
  }
  initialize_slot_parameters(*slot);
  return true;
}

auto EngineRuntime::parameter_value(std::uint32_t slot_id, std::string_view device_id, std::string_view parameter_id) const -> std::optional<double> {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto* slot = find_slot(slot_id);
  if (!slot) {
    return std::nullopt;
  }
  const auto key = parameter_key(device_id, parameter_id);
  const auto it = slot->parameter_values.find(key);
  if (it == slot->parameter_values.end()) {
    return std::nullopt;
  }
  return it->second;
}

auto EngineRuntime::set_parameter_value(std::uint32_t slot_id, std::string_view device_id, std::string_view parameter_id, double value) -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
  auto* slot = find_slot(slot_id);
  if (!slot || !slot->compiled_instrument.has_value()) {
    return false;
  }
  const auto* definition = parameter_definition(*slot->compiled_instrument, device_id, parameter_id);
  if (!definition) {
    return false;
  }
  slot->parameter_values[parameter_key(device_id, parameter_id)] = clamp(value, definition->minimum, definition->maximum);
  return true;
}

auto EngineRuntime::render_audio(float* interleaved_output,
                                 std::size_t frame_count,
                                 std::uint32_t channel_count,
                                 std::uint32_t sample_rate_hz) -> void {
  std::fill(interleaved_output, interleaved_output + (frame_count * channel_count), 0.0f);

  std::lock_guard<std::mutex> lock(mutex_);
  advance_demo_sequence(frame_count);
  for (auto& slot : slot_runtimes_) {
    render_slot(slot, interleaved_output, frame_count, channel_count, sample_rate_hz);
  }
}

auto EngineRuntime::set_demo_mode(bool enabled) noexcept -> void {
  std::lock_guard<std::mutex> lock(mutex_);
  demo_mode_enabled_ = enabled;
  demo_frames_until_event_ = 0;
  demo_note_index_ = 0;
  demo_gate_on_ = false;
}

auto EngineRuntime::default_slot_count() noexcept -> std::uint32_t {
  return 4;
}

auto EngineRuntime::ensure_slot_defaults() -> void {
  std::lock_guard<std::mutex> lock(mutex_);
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

auto EngineRuntime::allocate_voice(SlotRuntime& slot, const NoteEvent&) -> VoiceInstance* {
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

auto EngineRuntime::find_slot(std::uint32_t slot_id) -> SlotRuntime* {
  const auto it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [slot_id](const SlotRuntime& slot) {
    return slot.slot_id == slot_id;
  });
  return (it != slot_runtimes_.end()) ? &(*it) : nullptr;
}

auto EngineRuntime::find_slot(std::uint32_t slot_id) const -> const SlotRuntime* {
  const auto it = std::find_if(slot_runtimes_.begin(), slot_runtimes_.end(), [slot_id](const SlotRuntime& slot) {
    return slot.slot_id == slot_id;
  });
  return (it != slot_runtimes_.end()) ? &(*it) : nullptr;
}

auto EngineRuntime::parameter_key(std::string_view device_id, std::string_view parameter_id) const -> std::string {
  return std::string(device_id) + "." + std::string(parameter_id);
}

auto EngineRuntime::parameter_definition(const CompiledInstrument& instrument,
                                         std::string_view device_id,
                                         std::string_view parameter_id) const -> const DeviceParameterDefinition* {
  const auto instance_it = std::find_if(
      instrument.instrument.devices.begin(), instrument.instrument.devices.end(), [device_id](const DeviceInstance& instance) {
        return instance.id == device_id;
      });
  if (instance_it == instrument.instrument.devices.end()) {
    return nullptr;
  }
  const auto* device_type = registry_.find_device_type(instance_it->type_id);
  if (!device_type) {
    return nullptr;
  }
  const auto param_it = std::find_if(
      device_type->parameters.begin(), device_type->parameters.end(), [parameter_id](const DeviceParameterDefinition& parameter) {
        return parameter.id == parameter_id;
      });
  return (param_it != device_type->parameters.end()) ? &(*param_it) : nullptr;
}

auto EngineRuntime::initialize_slot_parameters(SlotRuntime& slot) -> void {
  slot.parameter_values.clear();
  if (!slot.compiled_instrument.has_value()) {
    return;
  }

  for (const auto& device : slot.compiled_instrument->instrument.devices) {
    const auto* device_type = registry_.find_device_type(device.type_id);
    if (!device_type) {
      continue;
    }
    for (const auto& parameter : device_type->parameters) {
      auto value = parameter.default_value;
      const auto override_it = device.parameter_values.find(parameter.id);
      if (override_it != device.parameter_values.end()) {
        value = override_it->second;
      }
      slot.parameter_values.emplace(parameter_key(device.id, parameter.id), value);
    }
  }
}

auto EngineRuntime::advance_demo_sequence(std::size_t frame_count) -> void {
  if (!demo_mode_enabled_ || slot_runtimes_.empty()) {
    return;
  }

  if (demo_frames_until_event_ > frame_count) {
    demo_frames_until_event_ -= frame_count;
    return;
  }

  const auto midi_channel = slot_runtimes_.front().midi_channel;
  const auto midi_note = kDemoNotes[demo_note_index_ % (sizeof(kDemoNotes) / sizeof(kDemoNotes[0]))];
  auto& slot = slot_runtimes_.front();

  if (demo_gate_on_) {
    for (auto& voice : slot.voices) {
      if (voice.state == VoiceState::active && voice.midi_note == midi_note) {
        voice.state = VoiceState::released;
        voice.envelope_stage = EnvelopeStage::release;
      }
    }
  } else if (auto* voice = allocate_voice(slot, NoteEvent{.type = NoteEventType::note_on, .midi_channel = midi_channel, .midi_note = midi_note, .velocity = 0.82})) {
    voice->state = VoiceState::active;
    voice->envelope_stage = EnvelopeStage::attack;
    voice->midi_note = midi_note;
    voice->midi_channel = midi_channel;
    voice->velocity = 0.82;
    voice->generation = ++voice_generation_counter_;
    voice->phase = 0.0;
    voice->envelope_level = 0.0;
    voice->filter_state = 0.0;
  }

  demo_gate_on_ = !demo_gate_on_;
  if (!demo_gate_on_) {
    ++demo_note_index_;
    demo_frames_until_event_ = static_cast<std::size_t>(config_.sample_rate_hz * 0.20);
  } else {
    demo_frames_until_event_ = static_cast<std::size_t>(config_.sample_rate_hz * 0.40);
  }
}

auto EngineRuntime::render_slot(SlotRuntime& slot,
                                float* interleaved_output,
                                std::size_t frame_count,
                                std::uint32_t channel_count,
                                std::uint32_t sample_rate_hz) -> void {
  if (!slot.enabled || slot.muted || !slot.compiled_instrument.has_value()) {
    return;
  }

  const auto find_value = [&slot](const std::string& key, double fallback) {
    const auto it = slot.parameter_values.find(key);
    return it != slot.parameter_values.end() ? it->second : fallback;
  };

  const auto waveform = static_cast<std::int32_t>(find_value(kOscWaveformKey, 0.0));
  const auto shape = find_value(kOscShapeKey, 0.5);
  const auto cutoff_hz = find_value(kFilterCutoffKey, 8000.0);
  const auto resonance = find_value(kFilterResonanceKey, 0.15);
  const auto amp_level = find_value(kVcaLevelKey, 1.0);
  const auto attack_ms = find_value(kAmpAttackKey, 20.0);
  const auto decay_ms = find_value(kAmpDecayKey, 180.0);
  const auto sustain_level = find_value(kAmpSustainKey, 0.75);
  const auto release_ms = find_value(kAmpReleaseKey, 250.0);

  for (std::size_t frame = 0; frame < frame_count; ++frame) {
    double mixed = 0.0;

    for (auto& voice : slot.voices) {
      if ((voice.state == VoiceState::idle && voice.envelope_stage == EnvelopeStage::idle) || !voice.midi_note.has_value()) {
        continue;
      }

      const auto envelope = update_envelope(voice, attack_ms, decay_ms, sustain_level, release_ms, sample_rate_hz);
      if (voice.state == VoiceState::idle && envelope <= 0.0) {
        continue;
      }

      const auto frequency = note_to_frequency(*voice.midi_note);
      voice.phase += frequency / static_cast<double>(sample_rate_hz);
      if (voice.phase >= 1.0) {
        voice.phase -= std::floor(voice.phase);
      }

      const auto osc = waveform_sample(waveform, voice.phase, shape);
      const auto modulated_cutoff = clamp(cutoff_hz * (0.35 + envelope * 0.90), 20.0, 18000.0);
      const auto alpha = 1.0 - std::exp((-2.0 * std::numbers::pi_v<double> * modulated_cutoff) / static_cast<double>(sample_rate_hz));
      const auto filter_input = osc - (voice.filter_state * clamp(resonance, 0.0, 1.0) * 0.35);
      voice.filter_state += alpha * (filter_input - voice.filter_state);

      mixed += voice.filter_state * envelope * amp_level * (0.35 + voice.velocity * 0.65);
    }

    mixed = clamp(mixed * slot.mix_level * 0.25, -1.0, 1.0);
    for (std::uint32_t channel = 0; channel < channel_count; ++channel) {
      interleaved_output[(frame * channel_count) + channel] += static_cast<float>(mixed);
    }
  }
}

}  // namespace pi_fartbox::engine
