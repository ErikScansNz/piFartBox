#pragma once

#include "pi_fartbox/engine/device_registry.hpp"

#include <cstdint>
#include <string_view>

namespace pi_fartbox::engine {

struct EngineRuntimeConfig {
  std::uint32_t slot_count = 4;
  std::uint32_t sample_rate_hz = 48000;
  bool browser_audio_enabled = false;
};

class EngineRuntime {
 public:
  explicit EngineRuntime(EngineRuntimeConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const EngineRuntimeConfig&;
  [[nodiscard]] auto registry() noexcept -> DeviceRegistry&;
  [[nodiscard]] auto registry() const noexcept -> const DeviceRegistry&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

  static auto default_slot_count() noexcept -> std::uint32_t;

 private:
  EngineRuntimeConfig config_;
  DeviceRegistry registry_;
};

}  // namespace pi_fartbox::engine
