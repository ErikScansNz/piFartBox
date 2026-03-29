#include "pi_fartbox/platform/linux_host.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

namespace pi_fartbox::platform {

namespace {

auto trim(std::string value) -> std::string {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

}  // namespace

LinuxHost::LinuxHost(LinuxHostConfig config) : config_(config), audio_probe_(probe_audio_state()) {}

auto LinuxHost::config() const noexcept -> const LinuxHostConfig& {
  return config_;
}

auto LinuxHost::subsystem_name() const noexcept -> std::string_view {
  return "platform/linux";
}

auto LinuxHost::audio_probe() const -> const AudioProbeInfo& {
  return audio_probe_;
}

auto LinuxHost::probe_audio_state() -> AudioProbeInfo {
  AudioProbeInfo info;

  const auto cards_path = std::filesystem::path("/proc/asound/cards");
  const auto pcm_path = std::filesystem::path("/proc/asound/pcm");
  info.proc_asound_available = std::filesystem::exists(cards_path) || std::filesystem::exists(pcm_path);

  if (std::ifstream cards(cards_path); cards.is_open()) {
    std::string line;
    while (std::getline(cards, line)) {
      if (!line.empty() && std::isdigit(static_cast<unsigned char>(line.front()))) {
        info.cards.push_back(trim(line));
      }
    }
  }

  if (std::ifstream pcm(pcm_path); pcm.is_open()) {
    std::string line;
    while (std::getline(pcm, line)) {
      const auto cleaned = trim(line);
      if (!cleaned.empty()) {
        info.pcm_devices.push_back(cleaned);
      }
    }
  }

  return info;
}

}  // namespace pi_fartbox::platform
