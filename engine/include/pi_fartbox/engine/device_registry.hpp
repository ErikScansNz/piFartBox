#pragma once

#include "pi_fartbox/engine/device_schema.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pi_fartbox::engine {

class DeviceRegistry {
 public:
  DeviceRegistry();
  explicit DeviceRegistry(std::vector<DeviceTypeDefinition> device_types);

  [[nodiscard]] auto device_types() const noexcept -> const std::vector<DeviceTypeDefinition>&;
  [[nodiscard]] auto find_device_type(std::string_view type_id) const -> const DeviceTypeDefinition*;
  [[nodiscard]] auto validate_instrument(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue>;

  void replace_device_types(std::vector<DeviceTypeDefinition> device_types);
  void register_device_type(DeviceTypeDefinition device_type);

 private:
  std::vector<DeviceTypeDefinition> device_types_;
};

class InstrumentCompiler {
 public:
  explicit InstrumentCompiler(const DeviceRegistry& registry);

  [[nodiscard]] auto validate(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue>;
  [[nodiscard]] auto compile(const InstrumentDefinition& instrument) const -> bool;

 private:
  const DeviceRegistry& registry_;
};

}  // namespace pi_fartbox::engine
