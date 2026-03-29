#include "pi_fartbox/engine/device_registry.hpp"

#include <algorithm>

namespace pi_fartbox::engine {

DeviceRegistry::DeviceRegistry() : device_types_(starter_device_palette()) {}

DeviceRegistry::DeviceRegistry(std::vector<DeviceTypeDefinition> device_types)
    : device_types_(std::move(device_types)) {}

auto DeviceRegistry::device_types() const noexcept -> const std::vector<DeviceTypeDefinition>& {
  return device_types_;
}

auto DeviceRegistry::find_device_type(std::string_view type_id) const -> const DeviceTypeDefinition* {
  const auto it = std::find_if(device_types_.begin(), device_types_.end(), [type_id](const DeviceTypeDefinition& device_type) {
    return device_type.id == type_id;
  });
  return (it != device_types_.end()) ? &(*it) : nullptr;
}

auto DeviceRegistry::validate_instrument(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue> {
  return validate_instrument_definition(instrument, device_types_);
}

void DeviceRegistry::replace_device_types(std::vector<DeviceTypeDefinition> device_types) {
  device_types_ = std::move(device_types);
}

void DeviceRegistry::register_device_type(DeviceTypeDefinition device_type) {
  const auto existing = std::find_if(device_types_.begin(), device_types_.end(), [&device_type](const DeviceTypeDefinition& current) {
    return current.id == device_type.id;
  });
  if (existing != device_types_.end()) {
    *existing = std::move(device_type);
    return;
  }
  device_types_.push_back(std::move(device_type));
}

InstrumentCompiler::InstrumentCompiler(const DeviceRegistry& registry) : registry_(registry) {}

auto InstrumentCompiler::validate(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue> {
  return registry_.validate_instrument(instrument);
}

auto InstrumentCompiler::compile(const InstrumentDefinition& instrument) const -> bool {
  return validate(instrument).empty();
}

}  // namespace pi_fartbox::engine
