#pragma once

#include <cstdint>
#include <memory>
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
  std::string alsa_device = "default";
  std::uint32_t audio_sample_rate_hz = 48000;
  std::uint32_t audio_channels = 2;
  std::uint32_t audio_period_frames = 256;
  std::uint32_t audio_period_count = 3;
  std::uint32_t audio_realtime_priority = 60;
  bool audio_test_tone_enabled = false;
  double audio_test_tone_hz = 220.0;
  double audio_test_tone_level = 0.08;
};

struct AlsaPlaybackStatus {
  bool compiled_with_alsa = false;
  bool device_opened = false;
  bool thread_running = false;
  bool realtime_thread = false;
  bool memory_locked = false;
  bool tone_enabled = false;
  std::string requested_device = "default";
  std::string active_device;
  std::string sample_format = "S16_LE";
  std::uint32_t sample_rate_hz = 0;
  std::uint32_t channels = 0;
  std::uint32_t period_frames = 0;
  std::uint32_t period_count = 0;
  std::uint64_t xrun_count = 0;
  std::uint64_t render_cycle_count = 0;
  double tone_frequency_hz = 0.0;
  std::string last_error;
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

class AlsaPlaybackEngine {
 public:
  explicit AlsaPlaybackEngine(LinuxHostConfig config = {});
  ~AlsaPlaybackEngine();

  auto start() -> bool;
  auto stop() -> void;

  [[nodiscard]] auto status() const -> AlsaPlaybackStatus;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace pi_fartbox::platform
