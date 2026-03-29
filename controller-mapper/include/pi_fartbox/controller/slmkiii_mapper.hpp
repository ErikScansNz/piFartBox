#pragma once

#include "pi_fartbox/controller/controller_core.hpp"

#include <string_view>

namespace pi_fartbox::controller {

class SlmkiiiMapper {
 public:
  explicit SlmkiiiMapper(SlmkiiiMapperConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const SlmkiiiMapperConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;
  [[nodiscard]] auto midi_router() noexcept -> MidiRouter&;
  [[nodiscard]] auto midi_router() const noexcept -> const MidiRouter&;
  [[nodiscard]] auto controller_core() noexcept -> SlmkiiiControllerCore&;
  [[nodiscard]] auto controller_core() const noexcept -> const SlmkiiiControllerCore&;

 private:
  SlmkiiiMapperConfig config_;
  MidiRouter midi_router_{};
  SlmkiiiControllerCore controller_core_;
};

}  // namespace pi_fartbox::controller
