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

auto json_escape(std::string_view input) -> std::string {
  std::string escaped;
  escaped.reserve(input.size() + 8);
  for (const auto ch : input) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

void write_runtime_state_json(const std::filesystem::path& state_dir,
                              const pi_fartbox::engine::EngineRuntime& engine,
                              const pi_fartbox::session::SessionManager& sessions,
                              const pi_fartbox::platform::LinuxHost& host,
                              const pi_fartbox::control::ControlServer& control,
                              const pi_fartbox::controller::ControllerContext& controller_context) {
  ensure_directory(state_dir);
  std::ofstream state_file(state_dir / "runtime-status.json", std::ios::trunc);
  const auto& probe = host.audio_probe();

  state_file << "{\n";
  state_file << "  \"generated_at\": \"" << json_escape(current_timestamp()) << "\",\n";
  state_file << "  \"runtime\": {\n";
  state_file << "    \"engine\": \"" << json_escape(std::string(engine.subsystem_name())) << "\",\n";
  state_file << "    \"sample_rate_hz\": " << engine.config().sample_rate_hz << ",\n";
  state_file << "    \"slot_count\": " << engine.config().slot_count << ",\n";
  state_file << "    \"session_root\": \"" << json_escape(sessions.paths().session_root) << "\",\n";
  state_file << "    \"content_root\": \"" << json_escape(sessions.paths().content_root) << "\",\n";
  state_file << "    \"install_root\": \"" << json_escape(host.config().install_root) << "\",\n";
  state_file << "    \"control_endpoint\": \"" << json_escape(control.config().bind_host + ":" + std::to_string(control.config().bind_port)) << "\"\n";
  state_file << "  },\n";
  state_file << "  \"audio\": {\n";
  state_file << "    \"backend\": \"" << json_escape(host.config().audio_backend) << "\",\n";
  state_file << "    \"proc_asound_available\": " << (probe.proc_asound_available ? "true" : "false") << ",\n";
  state_file << "    \"cards\": [";
  for (std::size_t index = 0; index < probe.cards.size(); ++index) {
    if (index > 0) {
      state_file << ", ";
    }
    state_file << "\"" << json_escape(probe.cards[index]) << "\"";
  }
  state_file << "],\n";
  state_file << "    \"pcm_devices\": [";
  for (std::size_t index = 0; index < probe.pcm_devices.size(); ++index) {
    if (index > 0) {
      state_file << ", ";
    }
    state_file << "\"" << json_escape(probe.pcm_devices[index]) << "\"";
  }
  state_file << "]\n";
  state_file << "  },\n";
  state_file << "  \"controller\": {\n";
  state_file << "    \"focused_slot\": " << controller_context.focused_slot_id << ",\n";
  state_file << "    \"device_input\": \"" << json_escape(controller_context.port_status.midi_input_name) << "\",\n";
  state_file << "    \"device_output\": \"" << json_escape(controller_context.port_status.midi_output_name) << "\",\n";
  state_file << "    \"sysex_enabled\": " << (controller_context.port_status.sysex_enabled ? "true" : "false") << ",\n";
  state_file << "    \"pages\": [\n";
  for (std::size_t index = 0; index < controller_context.pages.size(); ++index) {
    const auto& page = controller_context.pages[index];
    state_file << "      {\"id\": \"" << json_escape(page.id) << "\", \"title\": \"" << json_escape(page.title)
               << "\", \"binding_count\": " << page.bindings.size() << "}";
    if (index + 1 != controller_context.pages.size()) {
      state_file << ",";
    }
    state_file << "\n";
  }
  state_file << "    ]\n";
  state_file << "  },\n";
  state_file << "  \"slots\": [\n";
  for (std::size_t index = 0; index < engine.slot_runtimes().size(); ++index) {
    const auto& slot = engine.slot_runtimes()[index];
    state_file << "    {\"slot_id\": " << slot.slot_id
               << ", \"midi_channel\": " << static_cast<int>(slot.midi_channel)
               << ", \"focused\": " << (slot.focused ? "true" : "false")
               << ", \"voice_count\": " << slot.voices.size();
    if (slot.compiled_instrument.has_value()) {
      state_file << ", \"instrument\": \"" << json_escape(slot.compiled_instrument->instrument.name) << "\"";
      state_file << ", \"page_count\": " << slot.compiled_instrument->generated_pages.size();
    }
    state_file << "}";
    if (index + 1 != engine.slot_runtimes().size()) {
      state_file << ",";
    }
    state_file << "\n";
  }
  state_file << "  ],\n";
  state_file << "  \"module_palette\": [\n";
  const auto& device_types = engine.registry().device_types();
  for (std::size_t index = 0; index < device_types.size(); ++index) {
    const auto& device = device_types[index];
    state_file << "    {\"id\": \"" << json_escape(device.id)
               << "\", \"label\": \"" << json_escape(device.display.label)
               << "\", \"category\": \"" << json_escape(std::string(pi_fartbox::engine::to_string(device.category)))
               << "\", \"description\": \"" << json_escape(device.description) << "\"}";
    if (index + 1 != device_types.size()) {
      state_file << ",";
    }
    state_file << "\n";
  }
  state_file << "  ]\n";
  state_file << "}\n";
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
  const auto controller_context = controller.controller_core().make_context(
      engine,
      pi_fartbox::controller::PortRoleStatus{
          .midi_input_name = "auto",
          .midi_output_name = "auto",
          .input_connected = false,
          .output_connected = false,
          .sysex_enabled = controller.config().enable_sysex_feedback,
      });
  write_runtime_state_json(state_dir, engine, sessions, host, control, controller_context);

  if (one_shot) {
    return 0;
  }

  install_signal_handlers();
  while (g_should_run.load()) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}
