#pragma once

#include "pi_fartbox/engine/device_registry.hpp"
#include "pi_fartbox/engine/runtime_model.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace pi_fartbox::engine {

struct EngineRuntimeConfig {
  std::uint32_t slot_count = 4;
  std::uint32_t default_polyphony = 8;
  std::uint32_t sample_rate_hz = 48000;
  bool browser_audio_enabled = false;
};

class EngineRuntime {
 public:
  explicit EngineRuntime(EngineRuntimeConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const EngineRuntimeConfig&;
  [[nodiscard]] auto registry() noexcept -> DeviceRegistry&;
  [[nodiscard]] auto registry() const noexcept -> const DeviceRegistry&;
  [[nodiscard]] auto compiler() const noexcept -> const InstrumentCompiler&;
  [[nodiscard]] auto slot_runtimes() const noexcept -> const std::vector<SlotRuntime>&;
  [[nodiscard]] auto focused_slot() const noexcept -> const SlotRuntime*;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;
  [[nodiscard]] auto dispatch_note_event(const NoteEvent& event) -> bool;
  [[nodiscard]] auto focus_slot(std::uint32_t slot_id) -> bool;
  [[nodiscard]] auto assign_compiled_instrument(std::uint32_t slot_id, CompiledInstrument instrument) -> bool;

  static auto default_slot_count() noexcept -> std::uint32_t;

 private:
  auto ensure_slot_defaults() -> void;
  auto allocate_voice(SlotRuntime& slot, const NoteEvent& event) -> VoiceInstance*;

  EngineRuntimeConfig config_;
  DeviceRegistry registry_;
  InstrumentCompiler compiler_;
  std::vector<SlotRuntime> slot_runtimes_;
  std::uint64_t voice_generation_counter_ = 0;
};

}  // namespace pi_fartbox::engine
