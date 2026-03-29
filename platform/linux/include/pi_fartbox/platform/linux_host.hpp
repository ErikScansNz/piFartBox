#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace pi_fartbox::platform {

struct AudioProbeInfo {
  bool proc_asound_available = false;
  std::vector<std::string> cards;
  std::vector<std::string> pcm_devices;
};

struct LinuxHostConfig {
  std::string install_root = "/opt/piFartBox";
  std::string usb_update_root = "/media/usb0/piFartBox-update";
  std::string audio_backend = "alsa";
};

class LinuxHost {
 public:
  explicit LinuxHost(LinuxHostConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const LinuxHostConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;
  [[nodiscard]] auto audio_probe() const -> const AudioProbeInfo&;

 private:
  [[nodiscard]] static auto probe_audio_state() -> AudioProbeInfo;

  LinuxHostConfig config_;
  AudioProbeInfo audio_probe_;
};

}  // namespace pi_fartbox::platform
