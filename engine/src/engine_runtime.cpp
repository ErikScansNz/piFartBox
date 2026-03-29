#include "pi_fartbox/engine/engine_runtime.hpp"

namespace pi_fartbox::engine {

EngineRuntime::EngineRuntime(EngineRuntimeConfig config) : config_(config) {}

auto EngineRuntime::config() const noexcept -> const EngineRuntimeConfig& {
  return config_;
}

auto EngineRuntime::registry() noexcept -> DeviceRegistry& {
  return registry_;
}

auto EngineRuntime::registry() const noexcept -> const DeviceRegistry& {
  return registry_;
}

auto EngineRuntime::subsystem_name() const noexcept -> std::string_view {
  return "engine-core";
}

auto EngineRuntime::default_slot_count() noexcept -> std::uint32_t {
  return 4;
}

}  // namespace pi_fartbox::engine
