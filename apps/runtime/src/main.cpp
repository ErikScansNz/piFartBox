#include "pi_fartbox/control/control_server.hpp"
#include "pi_fartbox/controller/slmkiii_mapper.hpp"
#include "pi_fartbox/engine/engine_runtime.hpp"
#include "pi_fartbox/platform/linux_host.hpp"
#include "pi_fartbox/session/session_manager.hpp"
#include "pi_fartbox/zynthian/zynthian_adapter.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

std::atomic_bool g_should_run{true};

auto current_timestamp() -> std::string {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm{};
#if defined(_WIN32)
  gmtime_s(&utc_tm, &time);
#else
  gmtime_r(&time, &utc_tm);
#endif
  std::ostringstream out;
  out << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

void handle_signal(int) {
  g_should_run.store(false);
}

void install_signal_handlers() {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);
}

void ensure_directory(const std::filesystem::path& path) {
  std::error_code error;
  std::filesystem::create_directories(path, error);
}

void write_runtime_state(const std::filesystem::path& state_dir, const std::string& summary) {
  ensure_directory(state_dir);
  std::ofstream state_file(state_dir / "runtime-status.txt", std::ios::trunc);
  state_file << summary;
}

auto make_summary(const pi_fartbox::engine::EngineRuntime& engine,
                  const pi_fartbox::session::SessionManager& sessions,
                  const pi_fartbox::platform::LinuxHost& host,
                  const pi_fartbox::control::ControlServer& control,
                  const pi_fartbox::controller::SlmkiiiMapper& controller,
                  const pi_fartbox::zynthian::ZynthianAdapter& zynthian) -> std::string {
  const auto controller_context = controller.controller_core().make_context(
      engine,
      pi_fartbox::controller::PortRoleStatus{
          .midi_input_name = "auto",
          .midi_output_name = "auto",
          .input_connected = false,
          .output_connected = false,
          .sysex_enabled = controller.config().enable_sysex_feedback,
      });
  std::ostringstream out;
  out << "piFartBox runtime\n";
  out << "started_at: " << current_timestamp() << "\n";
  out << "engine: " << engine.subsystem_name() << "\n";
  out << "slots: " << engine.config().slot_count << "\n";
  out << "sample_rate_hz: " << engine.config().sample_rate_hz << "\n";
  out << "device_types: " << engine.registry().device_types().size() << "\n";
  out << "session_root: " << sessions.paths().session_root << "\n";
  out << "content_root: " << sessions.paths().content_root << "\n";
  out << "install_root: " << host.config().install_root << "\n";
  out << "audio_backend: " << host.config().audio_backend << "\n";
  out << "control_endpoint: " << control.config().bind_host << ":" << control.config().bind_port << "\n";
  out << "controller: " << controller.subsystem_name() << "\n";
  out << "controller_device: " << controller.config().device_name << "\n";
  out << "focused_slot: " << controller_context.focused_slot_id << "\n";
  out << "controller_pages: " << controller_context.pages.size() << "\n";
  for (const auto& slot : engine.slot_runtimes()) {
    out << "slot[" << slot.slot_id << "]: channel=" << static_cast<int>(slot.midi_channel)
        << " focused=" << (slot.focused ? "yes" : "no")
        << " voices=" << slot.voices.size();
    if (slot.compiled_instrument.has_value()) {
      out << " instrument=" << slot.compiled_instrument->instrument.name
          << " pages=" << slot.compiled_instrument->generated_pages.size();
    }
    out << "\n";
  }
  out << "zynthian_adapter: " << zynthian.subsystem_name() << "\n";
  return out.str();
}

}  // namespace

auto main(int argc, char** argv) -> int {
  bool one_shot = false;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--oneshot") {
      one_shot = true;
    } else if (argument == "--help") {
      std::cout << "Usage: pi_fartbox_runtime [--oneshot]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << "\n";
      return 1;
    }
  }

  pi_fartbox::engine::EngineRuntime engine;
  pi_fartbox::session::SessionManager sessions;
  pi_fartbox::platform::LinuxHost host;
  pi_fartbox::control::ControlServer control;
  pi_fartbox::controller::SlmkiiiMapper controller;
  pi_fartbox::zynthian::ZynthianAdapter zynthian;

  const auto summary = make_summary(engine, sessions, host, control, controller, zynthian);
  std::cout << summary;

  const auto state_dir = std::filesystem::path(sessions.paths().session_root).parent_path();
  write_runtime_state(state_dir, summary);

  if (one_shot) {
    return 0;
  }

  install_signal_handlers();
  while (g_should_run.load()) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}
