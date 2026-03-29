#include "pi_fartbox/platform/linux_host.hpp"

namespace pi_fartbox::platform {

LinuxHost::LinuxHost(LinuxHostConfig config) : config_(config) {}

auto LinuxHost::config() const noexcept -> const LinuxHostConfig& {
  return config_;
}

auto LinuxHost::subsystem_name() const noexcept -> std::string_view {
  return "platform/linux";
}

}  // namespace pi_fartbox::platform
