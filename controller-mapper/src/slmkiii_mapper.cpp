#include "pi_fartbox/controller/slmkiii_mapper.hpp"

namespace pi_fartbox::controller {

SlmkiiiMapper::SlmkiiiMapper(SlmkiiiMapperConfig config)
    : config_(config), controller_core_(config_) {}

auto SlmkiiiMapper::config() const noexcept -> const SlmkiiiMapperConfig& {
  return config_;
}

auto SlmkiiiMapper::subsystem_name() const noexcept -> std::string_view {
  return "controller-mapper";
}

auto SlmkiiiMapper::midi_router() noexcept -> MidiRouter& {
  return midi_router_;
}

auto SlmkiiiMapper::midi_router() const noexcept -> const MidiRouter& {
  return midi_router_;
}

auto SlmkiiiMapper::controller_core() noexcept -> SlmkiiiControllerCore& {
  return controller_core_;
}

auto SlmkiiiMapper::controller_core() const noexcept -> const SlmkiiiControllerCore& {
  return controller_core_;
}

}  // namespace pi_fartbox::controller
