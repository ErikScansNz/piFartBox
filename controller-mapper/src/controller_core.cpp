#include "pi_fartbox/controller/controller_core.hpp"

#include <algorithm>

namespace pi_fartbox::controller {
namespace {

constexpr std::uint8_t kScreenUpCc = 81;
constexpr std::uint8_t kScreenDownCc = 82;
constexpr std::uint8_t kOptionsCc = 90;

auto make_binding(std::string id,
                  std::string target_id,
                  engine::ControlIntent intent,
                  engine::DisplayText display) -> engine::ControllerBindingDefinition {
  return engine::ControllerBindingDefinition{
      .id = std::move(id),
      .target_id = std::move(target_id),
      .control_intent = intent,
      .display = std::move(display),
  };
}

}  // namespace

auto MidiRouter::route_note_message(const MidiMessage& message) const -> std::optional<engine::NoteEvent> {
  if (message.type != MidiMessageType::note_on && message.type != MidiMessageType::note_off) {
    return std::nullopt;
  }

  return engine::NoteEvent{
      .type = message.type == MidiMessageType::note_on ? engine::NoteEventType::note_on : engine::NoteEventType::note_off,
      .midi_channel = static_cast<std::uint8_t>(message.channel),
      .midi_note = message.data1,
      .velocity = static_cast<double>(message.data2) / 127.0,
  };
}

auto MidiRouter::route_controller_action(const MidiMessage& message) const -> std::optional<ControllerAction> {
  if (message.type != MidiMessageType::control_change || message.data2 == 0) {
    return std::nullopt;
  }

  if (message.data1 >= 51 && message.data1 <= 54) {
    return ControllerAction{
        .type = ControllerActionType::focus_slot,
        .slot_id = static_cast<std::uint32_t>(message.data1 - 50),
    };
  }

  if (message.data1 == kScreenUpCc) {
    return ControllerAction{.type = ControllerActionType::previous_page};
  }
  if (message.data1 == kScreenDownCc) {
    return ControllerAction{.type = ControllerActionType::next_page};
  }
  if (message.data1 == kOptionsCc) {
    return ControllerAction{.type = ControllerActionType::show_system_page};
  }

  return std::nullopt;
}

auto MidiRouter::slot_channel_for_slot(std::uint32_t slot_id) -> std::uint8_t {
  return static_cast<std::uint8_t>(slot_id);
}

SlmkiiiControllerCore::SlmkiiiControllerCore(SlmkiiiMapperConfig config) : config_(std::move(config)) {}

auto SlmkiiiControllerCore::config() const noexcept -> const SlmkiiiMapperConfig& {
  return config_;
}

auto SlmkiiiControllerCore::make_context(const engine::EngineRuntime& runtime, PortRoleStatus port_status) const -> ControllerContext {
  ControllerContext context{
      .focused_slot_id = runtime.focused_slot() ? runtime.focused_slot()->slot_id : 1,
      .pages = build_workstation_pages(runtime),
      .port_status = std::move(port_status),
  };

  if (const auto* focused_slot = runtime.focused_slot()) {
    auto instrument_pages = build_instrument_pages(*focused_slot);
    context.pages.insert(context.pages.end(), instrument_pages.begin(), instrument_pages.end());
  }

  context.pages.push_back(build_system_page(context.port_status));
  return context;
}

auto SlmkiiiControllerCore::decode_relative_delta(std::uint8_t value) const -> std::int32_t {
  return value < 64 ? static_cast<std::int32_t>(value) : static_cast<std::int32_t>(value) - 128;
}

auto SlmkiiiControllerCore::build_notification_sysex(std::string line1, std::string line2) const -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> message = {0xF0, 0x00, 0x20, 0x29, 0x02, 0x0A, 0x01, 0x04};
  for (const auto ch : line1) {
    message.push_back(static_cast<std::uint8_t>(ch));
  }
  message.push_back(0x00);
  for (const auto ch : line2) {
    message.push_back(static_cast<std::uint8_t>(ch));
  }
  message.push_back(0x00);
  message.push_back(0xF7);
  return message;
}

auto SlmkiiiControllerCore::build_set_led_rgb_sysex(std::uint8_t led_index, std::uint8_t red, std::uint8_t green, std::uint8_t blue) const
    -> std::vector<std::uint8_t> {
  return {0xF0, 0x00, 0x20, 0x29, 0x02, 0x0A, 0x01, 0x03, led_index, 0x01, red, green, blue, 0xF7};
}

auto SlmkiiiControllerCore::build_workstation_pages(const engine::EngineRuntime& runtime) const
    -> std::vector<engine::ControllerPageDefinition> {
  engine::ControllerPageDefinition page{
      .id = "workstation",
      .title = "Workstation",
      .bindings = {},
  };

  for (const auto& slot : runtime.slot_runtimes()) {
    page.bindings.push_back(make_binding(
        "slot-" + std::to_string(slot.slot_id),
        "slot.focus." + std::to_string(slot.slot_id),
        engine::ControlIntent::button,
        engine::DisplayText{
            .short_label = "S" + std::to_string(slot.slot_id),
            .label = "Slot " + std::to_string(slot.slot_id),
            .section = "Slots",
            .help_text = "Focus slot " + std::to_string(slot.slot_id),
        }));
  }

  return {page};
}

auto SlmkiiiControllerCore::build_instrument_pages(const engine::SlotRuntime& slot) const
    -> std::vector<engine::ControllerPageDefinition> {
  if (!slot.compiled_instrument.has_value()) {
    return {};
  }

  std::vector<engine::ControllerPageDefinition> pages;
  for (const auto& page : slot.compiled_instrument->generated_pages) {
    engine::ControllerPageDefinition page_definition{
        .id = "instrument." + page.id,
        .title = page.title,
        .bindings = {},
    };

    for (const auto& control : page.controls) {
      page_definition.bindings.push_back(make_binding(
          control.id,
          control.source_device_id + "." + control.source_parameter_id,
          control.control_intent,
          control.display));
    }
    pages.push_back(std::move(page_definition));
  }
  return pages;
}

auto SlmkiiiControllerCore::build_system_page(PortRoleStatus port_status) const -> engine::ControllerPageDefinition {
  return engine::ControllerPageDefinition{
      .id = "system",
      .title = "System",
      .bindings = {
          make_binding(
              "system-midi-in",
              port_status.midi_input_name,
              engine::ControlIntent::switch_control,
              engine::DisplayText{
                  .short_label = "In",
                  .label = "MIDI Input",
                  .section = "System",
                  .help_text = port_status.midi_input_name,
              }),
          make_binding(
              "system-midi-out",
              port_status.midi_output_name,
              engine::ControlIntent::switch_control,
              engine::DisplayText{
                  .short_label = "Out",
                  .label = "MIDI Output",
                  .section = "System",
                  .help_text = port_status.midi_output_name,
              }),
      },
  };
}

}  // namespace pi_fartbox::controller
