#pragma once

#include "pi_fartbox/engine/device_registry.hpp"
#include "pi_fartbox/engine/runtime_model.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pi_fartbox::engine {

struct EngineRuntimeConfig {
  std::uint32_t slot_count = 4;
  std::uint32_t default_polyphony = 8;
  std::uint32_t sample_rate_hz = 48000;
  bool browser_audio_enabled = false;
  bool demo_mode_enabled = false;
};

class EngineRuntime {
 public:
  explicit EngineRuntime(EngineRuntimeConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const EngineRuntimeConfig&;
  [[nodiscard]] auto registry() noexcept -> DeviceRegistry&;
  [[nodiscard]] auto registry() const noexcept -> const DeviceRegistry&;
  [[nodiscard]] auto compiler() const noexcept -> const InstrumentCompiler&;
  [[nodiscard]] auto slot_runtimes_snapshot() const -> std::vector<SlotRuntime>;
  [[nodiscard]] auto focused_slot_id() const noexcept -> std::uint32_t;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;
  [[nodiscard]] auto dispatch_note_event(const NoteEvent& event) -> bool;
  [[nodiscard]] auto focus_slot(std::uint32_t slot_id) -> bool;
  [[nodiscard]] auto assign_compiled_instrument(std::uint32_t slot_id, CompiledInstrument instrument) -> bool;
  [[nodiscard]] auto parameter_value(std::uint32_t slot_id, std::string_view device_id, std::string_view parameter_id) const -> std::optional<double>;
  [[nodiscard]] auto set_parameter_value(std::uint32_t slot_id, std::string_view device_id, std::string_view parameter_id, double value) -> bool;
  auto render_audio(float* interleaved_output, std::size_t frame_count, std::uint32_t channel_count, std::uint32_t sample_rate_hz) -> void;
  auto set_demo_mode(bool enabled) noexcept -> void;

  static auto default_slot_count() noexcept -> std::uint32_t;

 private:
  auto ensure_slot_defaults() -> void;
  auto allocate_voice(SlotRuntime& slot, const NoteEvent& event) -> VoiceInstance*;
  [[nodiscard]] auto find_slot(std::uint32_t slot_id) -> SlotRuntime*;
  [[nodiscard]] auto find_slot(std::uint32_t slot_id) const -> const SlotRuntime*;
  [[nodiscard]] auto parameter_key(std::string_view device_id, std::string_view parameter_id) const -> std::string;
  [[nodiscard]] auto parameter_definition(const CompiledInstrument& instrument, std::string_view device_id, std::string_view parameter_id) const
      -> const DeviceParameterDefinition*;
  auto initialize_slot_parameters(SlotRuntime& slot) -> void;
  auto advance_demo_sequence(std::size_t frame_count) -> void;
  auto render_slot(SlotRuntime& slot, float* interleaved_output, std::size_t frame_count, std::uint32_t channel_count, std::uint32_t sample_rate_hz)
      -> void;

  EngineRuntimeConfig config_;
  DeviceRegistry registry_;
  InstrumentCompiler compiler_;
  std::vector<SlotRuntime> slot_runtimes_;
  std::uint64_t voice_generation_counter_ = 0;
  mutable std::mutex mutex_;
  bool demo_mode_enabled_ = false;
  std::size_t demo_frames_until_event_ = 0;
  std::size_t demo_note_index_ = 0;
  bool demo_gate_on_ = false;
};

}  // namespace pi_fartbox::engine
