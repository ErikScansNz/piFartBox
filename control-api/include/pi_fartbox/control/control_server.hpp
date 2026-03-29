#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pi_fartbox::control {

struct ControlServerConfig {
  std::string bind_host = "127.0.0.1";
  std::uint16_t bind_port = 8181;
  bool local_only = true;
};

class ControlServer {
 public:
  explicit ControlServer(ControlServerConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const ControlServerConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

 private:
  ControlServerConfig config_;
};

}  // namespace pi_fartbox::control
