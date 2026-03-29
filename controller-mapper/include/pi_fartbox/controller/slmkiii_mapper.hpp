#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pi_fartbox::controller {

struct SlmkiiiMapperConfig {
  std::string device_name = "Novation SL MkIII";
  std::uint32_t slot_count = 4;
  bool enable_sysex_feedback = true;
};

class SlmkiiiMapper {
 public:
  explicit SlmkiiiMapper(SlmkiiiMapperConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const SlmkiiiMapperConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

 private:
  SlmkiiiMapperConfig config_;
};

}  // namespace pi_fartbox::controller
