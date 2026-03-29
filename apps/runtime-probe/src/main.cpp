#include "pi_fartbox/control/control_server.hpp"
#include "pi_fartbox/controller/slmkiii_mapper.hpp"
#include "pi_fartbox/engine/engine_runtime.hpp"
#include "pi_fartbox/platform/linux_host.hpp"
#include "pi_fartbox/session/session_manager.hpp"
#include "pi_fartbox/zynthian/zynthian_adapter.hpp"

#include <iostream>

int main() {
  pi_fartbox::engine::EngineRuntime engine;
  pi_fartbox::session::SessionManager sessions;
  pi_fartbox::platform::LinuxHost host;
  pi_fartbox::control::ControlServer control;
  pi_fartbox::controller::SlmkiiiMapper controller;
  pi_fartbox::zynthian::ZynthianAdapter zynthian;

  std::cout << "piFartBox runtime probe\n";
  std::cout << "engine: " << engine.subsystem_name() << "\n";
  std::cout << "slots: " << engine.config().slot_count << "\n";
  std::cout << "device_types: " << engine.registry().device_types().size() << "\n";
  std::cout << "session_root: " << sessions.paths().session_root << "\n";
  std::cout << "audio_backend: " << host.config().audio_backend << "\n";
  std::cout << "control_server: " << control.subsystem_name() << "\n";
  std::cout << "controller: " << controller.subsystem_name() << "\n";
  std::cout << "zynthian: " << zynthian.subsystem_name() << "\n";
  return 0;
}
