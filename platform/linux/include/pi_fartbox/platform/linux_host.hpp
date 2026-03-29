#pragma once

#include <string>
#include <string_view>

namespace pi_fartbox::platform {

struct LinuxHostConfig {
  std::string install_root = "/opt/piFartBox";
  std::string usb_update_root = "/media/usb0/piFartBox-update";
  std::string audio_backend = "jack";
};

class LinuxHost {
 public:
  explicit LinuxHost(LinuxHostConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const LinuxHostConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

 private:
  LinuxHostConfig config_;
};

}  // namespace pi_fartbox::platform
