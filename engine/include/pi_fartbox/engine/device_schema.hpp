#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pi_fartbox::engine {

enum class DeviceCategory {
  source,
  tone,
  mod,
  event,
  mix,
  fx,
  utility,
  composite
};

enum class CpuClass {
  light,
  medium,
  heavy
};

enum class SignalFamily {
  audio,
  mod,
  event
};

enum class PortDirection {
  input,
  output
};

enum class EventSignalType {
  any,
  note,
  gate,
  trigger,
  clock,
  transport,
  sequence
};

enum class ParameterValueType {
  floating_point,
  integer,
  boolean,
  enumeration
};

enum class ParameterSteppingHint {
  continuous,
  stepped
};

enum class DisplayFormat {
  plain,
  percentage,
  hertz,
  milliseconds,
  semitones,
  decibels,
  enum_label
};

enum class ControlIntent {
  knob,
  fader,
  button,
  switch_control
};

struct DisplayText {
  std::string short_label;
  std::string label;
  std::string section;
  std::string help_text;
};

struct DevicePortDefinition {
  std::string id;
  SignalFamily signal_family = SignalFamily::audio;
  PortDirection direction = PortDirection::input;
  std::uint32_t channel_count = 1;
  bool mono_only = false;
  std::optional<EventSignalType> event_type;
};

struct DeviceParameterOptionDefinition {
  std::string id;
  std::string label;
  std::int32_t value = 0;
};

struct DeviceParameterDefinition {
  std::string id;
  ParameterValueType value_type = ParameterValueType::floating_point;
  double minimum = 0.0;
  double maximum = 1.0;
  double default_value = 0.0;
  ParameterSteppingHint stepping = ParameterSteppingHint::continuous;
  DisplayFormat display_format = DisplayFormat::plain;
  ControlIntent control_intent = ControlIntent::knob;
  DisplayText display;
  std::string units;
  bool modulation_eligible = true;
  bool automation_visible = true;
  std::vector<DeviceParameterOptionDefinition> options;
};

struct ExportedControlDefinition {
  std::string id;
  std::string source_device_id;
  std::string source_parameter_id;
  DisplayText display;
  ControlIntent control_intent = ControlIntent::knob;
  bool visible = true;
  bool automation_visible = true;
  std::string preferred_page;
};

struct ChildLayoutRule {
  std::vector<DeviceCategory> allowed_child_categories;
  std::uint32_t max_child_count = 0;
  bool children_visible = true;
  bool bubble_exports = false;
};

struct DeviceTypeDefinition {
  std::string id;
  DeviceCategory category = DeviceCategory::utility;
  std::string description;
  std::vector<DevicePortDefinition> ports;
  std::vector<DeviceParameterDefinition> parameters;
  std::vector<ExportedControlDefinition> exported_controls;
  std::optional<ChildLayoutRule> child_layout;
  CpuClass cpu_class = CpuClass::light;
  DisplayText display;
};

struct DeviceInstance {
  std::string id;
  std::string type_id;
  std::unordered_map<std::string, double> parameter_values;
  std::unordered_map<std::string, std::string> exported_control_overrides;
  std::vector<DeviceInstance> children;
};

struct DeviceConnectionEndpoint {
  std::string device_id;
  std::string port_id;
};

struct DeviceConnection {
  std::string id;
  DeviceConnectionEndpoint source;
  DeviceConnectionEndpoint target;
};

struct InstrumentDefinition {
  std::string id;
  std::uint32_t version = 1;
  std::string name;
  std::vector<DeviceInstance> devices;
  std::vector<DeviceConnection> connections;
  std::vector<ExportedControlDefinition> exported_controls;
  std::vector<std::string> default_pages;
  std::vector<std::string> validation_notes;
};

struct SlotState {
  std::uint32_t slot_id = 0;
  std::string instrument_id;
  bool enabled = true;
  bool muted = false;
  bool focused = false;
  std::uint32_t midi_channel = 1;
  double mix_level = 1.0;
  double pan = 0.0;
  double send_level = 0.0;
};

struct ControllerBindingDefinition {
  std::string id;
  std::string target_id;
  ControlIntent control_intent = ControlIntent::knob;
  DisplayText display;
};

struct ControllerPageDefinition {
  std::string id;
  std::string title;
  std::vector<ControllerBindingDefinition> bindings;
};

struct ControllerProfile {
  std::string id;
  std::string device_name;
  std::vector<ControllerPageDefinition> pages;
};

struct SessionDefinition {
  std::string id;
  std::uint32_t version = 1;
  std::vector<SlotState> slots;
  std::string focused_slot_id;
  std::vector<std::string> global_state_tags;
  std::vector<ControllerProfile> controller_profiles;
};

struct GraphValidationIssue {
  std::string code;
  std::string message;
  std::string connection_id;
};

[[nodiscard]] auto starter_device_palette() -> std::vector<DeviceTypeDefinition>;
[[nodiscard]] auto validate_instrument_definition(
  const InstrumentDefinition& instrument,
  const std::vector<DeviceTypeDefinition>& device_types) -> std::vector<GraphValidationIssue>;

[[nodiscard]] auto to_string(DeviceCategory value) -> std::string_view;
[[nodiscard]] auto to_string(SignalFamily value) -> std::string_view;
[[nodiscard]] auto to_string(EventSignalType value) -> std::string_view;

}  // namespace pi_fartbox::engine
