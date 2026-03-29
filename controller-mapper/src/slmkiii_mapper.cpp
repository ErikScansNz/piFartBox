#include "pi_fartbox/controller/slmkiii_mapper.hpp"

namespace pi_fartbox::controller {

SlmkiiiMapper::SlmkiiiMapper(SlmkiiiMapperConfig config) : config_(config) {}

auto SlmkiiiMapper::config() const noexcept -> const SlmkiiiMapperConfig& {
  return config_;
}

auto SlmkiiiMapper::subsystem_name() const noexcept -> std::string_view {
  return "controller-mapper";
}

}  // namespace pi_fartbox::controller
