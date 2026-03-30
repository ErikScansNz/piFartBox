#include "pi_fartbox/controller/controller_core.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace pi_fartbox::controller {
namespace {

constexpr std::uint8_t kScreenUpCc = 81;
constexpr std::uint8_t kScreenDownCc = 82;
constexpr std::uint8_t kOptionsCc = 90;
constexpr std::uint8_t kSysexHeader[] = {0xF0, 0x00, 0x20, 0x29, 0x02, 0x0A, 0x01};
constexpr std::uint8_t kCmdSetLayout = 0x01;
constexpr std::uint8_t kCmdSetProperties = 0x02;
constexpr std::uint8_t kCmdSetLed = 0x03;
constexpr std::uint8_t kCmdSetNotification = 0x04;
constexpr std::uint8_t kPropText = 0x01;
constexpr std::uint8_t kPropValue = 0x03;
constexpr std::uint8_t kTextFieldRow1 = 0x00;
constexpr std::uint8_t kTextFieldRow3 = 0x02;
constexpr std::uint8_t kValueFieldKnob = 0x00;
constexpr std::size_t kTextMaxChars = 9;

auto clamp_int(int value, int minimum, int maximum) -> std::uint8_t {
  return static_cast<std::uint8_t>(std::max(minimum, std::min(maximum, value)));
}

auto compact_text(std::string text, std::size_t max_chars) -> std::string {
  text.erase(std::unique(text.begin(), text.end(), [](char left, char right) {
    return std::isspace(static_cast<unsigned char>(left)) && std::isspace(static_cast<unsigned char>(right));
  }), text.end());
  if (text.size() <= max_chars) {
    return text;
  }
  return text.substr(0, max_chars);
}

auto ascii_bytes(std::string text, std::size_t max_chars) -> std::vector<std::uint8_t> {
  text = compact_text(std::move(text), max_chars);
  std::vector<std::uint8_t> bytes;
  bytes.reserve(max_chars);
  for (char ch : text) {
    const auto code = static_cast<unsigned char>(ch);
    bytes.push_back(code >= 32 && code <= 126 ? code : 32);
  }
  return bytes;
}

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

auto format_parameter_value(const engine::DeviceParameterDefinition& parameter, double value) -> std::string {
  std::ostringstream out;
  switch (parameter.display_format) {
    case engine::DisplayFormat::percentage:
      out << static_cast<int>(std::round(value * 100.0)) << "%";
      break;
    case engine::DisplayFormat::hertz:
      out << static_cast<int>(std::round(value)) << "Hz";
      break;
    case engine::DisplayFormat::milliseconds:
      out << static_cast<int>(std::round(value)) << "ms";
      break;
    case engine::DisplayFormat::enum_label: {
      const auto option_it = std::find_if(parameter.options.begin(), parameter.options.end(), [value](const engine::DeviceParameterOptionDefinition& option) {
        return option.value == static_cast<int>(std::round(value));
      });
      if (option_it != parameter.options.end()) {
        return option_it->label;
      }
      out << static_cast<int>(std::round(value));
      break;
    }
    case engine::DisplayFormat::plain:
    case engine::DisplayFormat::semitones:
    case engine::DisplayFormat::decibels:
    default:
      out << std::fixed << std::setprecision(value < 10.0 ? 2 : 1) << value;
      break;
  }
  return out.str();
}

auto parameter_to_midi_value(const engine::DeviceParameterDefinition& parameter, double value) -> std::uint8_t {
  if (parameter.maximum <= parameter.minimum) {
    return 0;
  }
  const auto normalized = (value - parameter.minimum) / (parameter.maximum - parameter.minimum);
  return clamp_int(static_cast<int>(std::round(normalized * 127.0)), 0, 127);
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
  const auto slots = runtime.slot_runtimes_snapshot();
  const auto focused_slot_it = std::find_if(slots.begin(), slots.end(), [](const engine::SlotRuntime& slot) { return slot.focused; });
  const auto focused_slot_id = focused_slot_it != slots.end() ? focused_slot_it->slot_id : 1;

  ControllerContext context{
      .focused_slot_id = focused_slot_id,
      .active_page_index = 0,
      .pages = build_workstation_pages(slots),
      .port_status = std::move(port_status),
  };

  if (focused_slot_it != slots.end()) {
    auto instrument_pages = build_instrument_pages(*focused_slot_it);
    const auto instrument_page_start = context.pages.size();
    context.pages.insert(context.pages.end(), instrument_pages.begin(), instrument_pages.end());
    if (!instrument_pages.empty()) {
      context.active_page_index = instrument_page_start;
      context.active_display_page = build_active_display_page(runtime, *focused_slot_it, context.pages, context.active_page_index);
    }
  }

  context.pages.push_back(build_system_page(context.port_status));
  if (!context.active_display_page.has_value() && !slots.empty()) {
    context.active_page_index = 0;
    context.active_display_page = build_active_display_page(runtime, slots.front(), context.pages, context.active_page_index);
  }
  return context;
}

auto SlmkiiiControllerCore::decode_relative_delta(std::uint8_t value) const -> std::int32_t {
  return value < 64 ? static_cast<std::int32_t>(value) : static_cast<std::int32_t>(value) - 128;
}

auto SlmkiiiControllerCore::build_set_layout_sysex(std::uint8_t layout_index) const -> std::vector<std::uint8_t> {
  return {kSysexHeader[0], kSysexHeader[1], kSysexHeader[2], kSysexHeader[3], kSysexHeader[4], kSysexHeader[5], kSysexHeader[6],
          kCmdSetLayout, layout_index, 0xF7};
}

auto SlmkiiiControllerCore::build_set_properties_sysex(const InControlDisplayPage& page) const -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> payload = {kSysexHeader[0], kSysexHeader[1], kSysexHeader[2], kSysexHeader[3], kSysexHeader[4], kSysexHeader[5], kSysexHeader[6], kCmdSetProperties};
  for (const auto& column : page.columns) {
    const auto row1 = ascii_bytes(column.row1, kTextMaxChars);
    const auto row3 = ascii_bytes(column.row3, kTextMaxChars);
    payload.push_back(column.column);
    payload.push_back(kPropText);
    payload.push_back(kTextFieldRow1);
    payload.insert(payload.end(), row1.begin(), row1.end());
    payload.push_back(0x00);

    payload.push_back(column.column);
    payload.push_back(kPropText);
    payload.push_back(kTextFieldRow3);
    payload.insert(payload.end(), row3.begin(), row3.end());
    payload.push_back(0x00);

    payload.push_back(column.column);
    payload.push_back(kPropValue);
    payload.push_back(kValueFieldKnob);
    payload.push_back(column.value);
  }
  payload.push_back(0xF7);
  return payload;
}

auto SlmkiiiControllerCore::build_notification_sysex(std::string line1, std::string line2) const -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> message = {kSysexHeader[0], kSysexHeader[1], kSysexHeader[2], kSysexHeader[3], kSysexHeader[4], kSysexHeader[5], kSysexHeader[6], kCmdSetNotification};
  const auto l1 = ascii_bytes(std::move(line1), 18);
  const auto l2 = ascii_bytes(std::move(line2), 18);
  message.insert(message.end(), l1.begin(), l1.end());
  message.push_back(0x00);
  message.insert(message.end(), l2.begin(), l2.end());
  message.push_back(0x00);
  message.push_back(0xF7);
  return message;
}

auto SlmkiiiControllerCore::build_set_led_rgb_sysex(std::uint8_t led_index, std::uint8_t red, std::uint8_t green, std::uint8_t blue) const
    -> std::vector<std::uint8_t> {
  return {kSysexHeader[0], kSysexHeader[1], kSysexHeader[2], kSysexHeader[3], kSysexHeader[4], kSysexHeader[5], kSysexHeader[6],
          kCmdSetLed, led_index, 0x01, red, green, blue, 0xF7};
}

auto SlmkiiiControllerCore::build_workstation_pages(const std::vector<engine::SlotRuntime>& slots) const
    -> std::vector<engine::ControllerPageDefinition> {
  engine::ControllerPageDefinition page{
      .id = "workstation",
      .title = "Workstation",
      .bindings = {},
  };

  for (const auto& slot : slots) {
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

auto SlmkiiiControllerCore::build_active_display_page(const engine::EngineRuntime& runtime,
                                                      const engine::SlotRuntime& focused_slot,
                                                      const std::vector<engine::ControllerPageDefinition>& pages,
                                                      std::size_t page_index) const -> InControlDisplayPage {
  if (pages.empty()) {
    return {};
  }

  const auto& page = pages[std::min(page_index, pages.size() - 1)];
  InControlDisplayPage display_page{
      .id = page.id,
      .title = page.title,
      .columns = {},
      .sysex_messages = {},
  };

  for (std::size_t index = 0; index < page.bindings.size() && index < 8; ++index) {
    const auto& binding = page.bindings[index];
    std::string row1 = binding.display.short_label.empty() ? binding.display.label : binding.display.short_label;
    std::string row3 = "--";
    std::uint8_t value = 0;

    if (page.id.rfind("instrument.", 0) == 0) {
      const auto separator = binding.target_id.find('.');
      if (separator != std::string::npos && focused_slot.compiled_instrument.has_value()) {
        const auto device_id = binding.target_id.substr(0, separator);
        const auto parameter_id = binding.target_id.substr(separator + 1);
        const auto parameter_value = runtime.parameter_value(focused_slot.slot_id, device_id, parameter_id);
        if (parameter_value.has_value()) {
          const auto device_it = std::find_if(
              focused_slot.compiled_instrument->instrument.devices.begin(),
              focused_slot.compiled_instrument->instrument.devices.end(),
              [&device_id](const engine::DeviceInstance& instance) { return instance.id == device_id; });
          if (device_it != focused_slot.compiled_instrument->instrument.devices.end()) {
            if (const auto* device_type = runtime.registry().find_device_type(device_it->type_id)) {
              const auto parameter_it = std::find_if(
                  device_type->parameters.begin(), device_type->parameters.end(),
                  [&parameter_id](const engine::DeviceParameterDefinition& parameter) { return parameter.id == parameter_id; });
              if (parameter_it != device_type->parameters.end()) {
                row3 = format_parameter_value(*parameter_it, *parameter_value);
                value = parameter_to_midi_value(*parameter_it, *parameter_value);
              }
            }
          }
        }
      }
    } else {
      row3 = binding.display.section;
    }

    display_page.columns.push_back(InControlDisplayColumn{
        .column = static_cast<std::uint8_t>(index),
        .row1 = row1,
        .row3 = row3,
        .value = value,
    });
  }

  display_page.sysex_messages.push_back(build_set_layout_sysex());
  display_page.sysex_messages.push_back(build_set_properties_sysex(display_page));
  return display_page;
}

}  // namespace pi_fartbox::controller
