#include "pi_fartbox/control/control_server.hpp"

namespace pi_fartbox::control {

ControlServer::ControlServer(ControlServerConfig config) : config_(config) {}

auto ControlServer::config() const noexcept -> const ControlServerConfig& {
  return config_;
}

auto ControlServer::subsystem_name() const noexcept -> std::string_view {
  return "control-server";
}

}  // namespace pi_fartbox::control
