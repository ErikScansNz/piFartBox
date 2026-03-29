#include "pi_fartbox/engine/device_schema.hpp"

#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <utility>

namespace pi_fartbox::engine {
namespace {

auto make_display(std::string short_label, std::string label, std::string section, std::string help_text) -> DisplayText {
  return DisplayText{
      .short_label = std::move(short_label),
      .label = std::move(label),
      .section = std::move(section),
      .help_text = std::move(help_text),
  };
}

auto make_param(std::string id,
                double minimum,
                double maximum,
                double default_value,
                DisplayFormat format,
                ControlIntent intent,
                DisplayText display,
                std::string units = {}) -> DeviceParameterDefinition {
  return DeviceParameterDefinition{
      .id = std::move(id),
      .value_type = ParameterValueType::floating_point,
      .minimum = minimum,
      .maximum = maximum,
      .default_value = default_value,
      .stepping = ParameterSteppingHint::continuous,
      .display_format = format,
      .control_intent = intent,
      .display = std::move(display),
      .units = std::move(units),
      .modulation_eligible = true,
      .automation_visible = true,
      .options = {},
  };
}

auto find_port(const DeviceTypeDefinition& device_type, const std::string& port_id, PortDirection direction)
    -> const DevicePortDefinition* {
  const auto it = std::find_if(device_type.ports.begin(), device_type.ports.end(), [&port_id, direction](const DevicePortDefinition& port) {
    return port.id == port_id && port.direction == direction;
  });
  return (it != device_type.ports.end()) ? &(*it) : nullptr;
}

auto find_parameter(const DeviceTypeDefinition& device_type, const std::string& parameter_id)
    -> const DeviceParameterDefinition* {
  const auto it = std::find_if(
      device_type.parameters.begin(), device_type.parameters.end(), [&parameter_id](const DeviceParameterDefinition& parameter) {
        return parameter.id == parameter_id;
      });
  return (it != device_type.parameters.end()) ? &(*it) : nullptr;
}

auto is_event_compatible(const DevicePortDefinition& source, const DevicePortDefinition& target) -> bool {
  const auto source_event = source.event_type.value_or(EventSignalType::any);
  const auto target_event = target.event_type.value_or(EventSignalType::any);
  return source_event == EventSignalType::any || target_event == EventSignalType::any || source_event == target_event;
}

void validate_children(const DeviceInstance& instance,
                       const std::unordered_map<std::string, const DeviceTypeDefinition*>& type_by_id,
                       std::vector<GraphValidationIssue>& issues) {
  const auto type_it = type_by_id.find(instance.type_id);
  if (type_it == type_by_id.end()) {
    issues.push_back({
        .code = "unknown-device-type",
        .message = "Device instance references an unknown device type",
        .connection_id = instance.id,
    });
    return;
  }

  const DeviceTypeDefinition& device_type = *type_it->second;
  if (instance.children.empty()) {
    return;
  }

  if (!device_type.child_layout.has_value()) {
    issues.push_back({
        .code = "nested-device-not-allowed",
        .message = "Device type does not allow nested child devices",
        .connection_id = instance.id,
    });
    return;
  }

  const ChildLayoutRule& layout = *device_type.child_layout;
  if (instance.children.size() > layout.max_child_count) {
    issues.push_back({
        .code = "too-many-children",
        .message = "Device instance exceeds the allowed child device count",
        .connection_id = instance.id,
    });
  }

  for (const auto& child : instance.children) {
    const auto child_type_it = type_by_id.find(child.type_id);
    if (child_type_it == type_by_id.end()) {
      issues.push_back({
          .code = "unknown-child-device-type",
          .message = "Child device instance references an unknown device type",
          .connection_id = child.id,
      });
      continue;
    }
    const auto child_category = child_type_it->second->category;
    const bool allowed = std::find(layout.allowed_child_categories.begin(),
                                   layout.allowed_child_categories.end(),
                                   child_category) != layout.allowed_child_categories.end();
    if (!allowed) {
      issues.push_back({
          .code = "child-category-not-allowed",
          .message = "Child device category is not allowed by the parent device type",
          .connection_id = child.id,
      });
    }
    validate_children(child, type_by_id, issues);
  }
}

void validate_exported_controls(const InstrumentDefinition& instrument,
                               const std::unordered_map<std::string, const DeviceInstance*>& instance_by_id,
                               const std::unordered_map<std::string, const DeviceTypeDefinition*>& type_by_id,
                               std::vector<GraphValidationIssue>& issues) {
  for (const auto& exported_control : instrument.exported_controls) {
    const auto instance_it = instance_by_id.find(exported_control.source_device_id);
    if (instance_it == instance_by_id.end()) {
      issues.push_back({
          .code = "unknown-export-device",
          .message = "Exported control references an unknown device instance",
          .connection_id = exported_control.id,
      });
      continue;
    }
    const auto type_it = type_by_id.find(instance_it->second->type_id);
    if (type_it == type_by_id.end()) {
      issues.push_back({
          .code = "unknown-export-device-type",
          .message = "Exported control references a device with an unknown type",
          .connection_id = exported_control.id,
      });
      continue;
    }
    if (!find_parameter(*type_it->second, exported_control.source_parameter_id)) {
      issues.push_back({
          .code = "unknown-export-parameter",
          .message = "Exported control references a missing device parameter",
          .connection_id = exported_control.id,
      });
    }
  }
}

void validate_acyclic_graph(const InstrumentDefinition& instrument,
                            std::vector<GraphValidationIssue>& issues) {
  std::unordered_map<std::string, std::vector<std::string>> adjacency;
  for (const auto& connection : instrument.connections) {
    adjacency[connection.source.device_id].push_back(connection.target.device_id);
  }

  std::unordered_set<std::string> visiting;
  std::unordered_set<std::string> visited;

  const std::function<bool(const std::string&)> dfs = [&](const std::string& node_id) -> bool {
    if (visiting.contains(node_id)) {
      return true;
    }
    if (visited.contains(node_id)) {
      return false;
    }
    visiting.insert(node_id);
    const auto edge_it = adjacency.find(node_id);
    if (edge_it != adjacency.end()) {
      for (const auto& next : edge_it->second) {
        if (dfs(next)) {
          return true;
        }
      }
    }
    visiting.erase(node_id);
    visited.insert(node_id);
    return false;
  };

  for (const auto& device : instrument.devices) {
    if (dfs(device.id)) {
      issues.push_back({
          .code = "cyclic-device-graph",
          .message = "Instrument graph contains a cycle, which is not allowed in v1",
          .connection_id = device.id,
      });
      return;
    }
  }
}

}  // namespace

auto starter_device_palette() -> std::vector<DeviceTypeDefinition> {
  return {
      DeviceTypeDefinition{
          .id = "source.oscillator",
          .category = DeviceCategory::source,
          .description = "Primary subtractive oscillator source.",
          .ports = {
              DevicePortDefinition{.id = "pitch_in", .signal_family = SignalFamily::event, .direction = PortDirection::input, .channel_count = 1, .mono_only = true, .event_type = EventSignalType::note},
              DevicePortDefinition{.id = "audio_out", .signal_family = SignalFamily::audio, .direction = PortDirection::output, .channel_count = 1},
          },
          .parameters = {
              DeviceParameterDefinition{
                  .id = "waveform",
                  .value_type = ParameterValueType::enumeration,
                  .minimum = 0,
                  .maximum = 3,
                  .default_value = 0,
                  .stepping = ParameterSteppingHint::stepped,
                  .display_format = DisplayFormat::enum_label,
                  .control_intent = ControlIntent::knob,
                  .display = make_display("Wave", "Waveform", "Oscillator", "Selects the oscillator waveform."),
                  .units = "",
                  .modulation_eligible = false,
                  .automation_visible = true,
                  .options = {
                      DeviceParameterOptionDefinition{.id = "saw", .label = "Saw", .value = 0},
                      DeviceParameterOptionDefinition{.id = "square", .label = "Square", .value = 1},
                      DeviceParameterOptionDefinition{.id = "triangle", .label = "Triangle", .value = 2},
                      DeviceParameterOptionDefinition{.id = "sine", .label = "Sine", .value = 3},
                  },
              },
              make_param("shape", 0.0, 1.0, 0.5, DisplayFormat::percentage, ControlIntent::knob,
                         make_display("Shape", "Shape", "Oscillator", "Morphs the oscillator shape."), "%"),
          },
          .exported_controls = {
              ExportedControlDefinition{
                  .id = "osc-waveform",
                  .source_device_id = "oscillator",
                  .source_parameter_id = "waveform",
                  .display = make_display("Wave", "Waveform", "Oscillator", "Waveform selection."),
                  .control_intent = ControlIntent::knob,
                  .visible = true,
                  .automation_visible = true,
                  .preferred_page = "osc",
              },
          },
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::light,
          .display = make_display("Osc", "Oscillator", "Source", "Primary tone generator."),
      },
      DeviceTypeDefinition{
          .id = "tone.multimode_filter",
          .category = DeviceCategory::tone,
          .description = "Multimode subtractive filter.",
          .ports = {
              DevicePortDefinition{.id = "audio_in", .signal_family = SignalFamily::audio, .direction = PortDirection::input},
              DevicePortDefinition{.id = "mod_in", .signal_family = SignalFamily::mod, .direction = PortDirection::input},
              DevicePortDefinition{.id = "audio_out", .signal_family = SignalFamily::audio, .direction = PortDirection::output},
          },
          .parameters = {
              make_param("cutoff", 20.0, 18000.0, 8000.0, DisplayFormat::hertz, ControlIntent::knob,
                         make_display("Cutoff", "Cutoff", "Filter", "Filter cutoff frequency."), "Hz"),
              make_param("resonance", 0.0, 1.0, 0.15, DisplayFormat::percentage, ControlIntent::knob,
                         make_display("Reso", "Resonance", "Filter", "Filter resonance amount."), "%"),
          },
          .exported_controls = {
              ExportedControlDefinition{
                  .id = "filter-cutoff",
                  .source_device_id = "filter",
                  .source_parameter_id = "cutoff",
                  .display = make_display("Cut", "Cutoff", "Filter", "Main filter cutoff."),
                  .control_intent = ControlIntent::knob,
                  .visible = true,
                  .automation_visible = true,
                  .preferred_page = "filter",
              },
          },
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::medium,
          .display = make_display("Filt", "Filter", "Tone", "Main subtractive filter."),
      },
      DeviceTypeDefinition{
          .id = "mix.vca",
          .category = DeviceCategory::mix,
          .description = "Voltage-controlled amplifier stage.",
          .ports = {
              DevicePortDefinition{.id = "audio_in", .signal_family = SignalFamily::audio, .direction = PortDirection::input},
              DevicePortDefinition{.id = "mod_in", .signal_family = SignalFamily::mod, .direction = PortDirection::input},
              DevicePortDefinition{.id = "audio_out", .signal_family = SignalFamily::audio, .direction = PortDirection::output},
          },
          .parameters = {
              make_param("level", 0.0, 1.0, 1.0, DisplayFormat::percentage, ControlIntent::fader,
                         make_display("Lvl", "Level", "Amp", "Output level."), "%"),
          },
          .exported_controls = {},
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::light,
          .display = make_display("VCA", "Amplifier", "Mix", "Gain stage."),
      },
      DeviceTypeDefinition{
          .id = "mod.adsr",
          .category = DeviceCategory::mod,
          .description = "Envelope generator.",
          .ports = {
              DevicePortDefinition{.id = "gate_in", .signal_family = SignalFamily::event, .direction = PortDirection::input, .event_type = EventSignalType::gate},
              DevicePortDefinition{.id = "mod_out", .signal_family = SignalFamily::mod, .direction = PortDirection::output},
          },
          .parameters = {
              make_param("attack_ms", 0.0, 5000.0, 20.0, DisplayFormat::milliseconds, ControlIntent::fader,
                         make_display("Atk", "Attack", "Envelope", "Attack time."), "ms"),
              make_param("decay_ms", 0.0, 5000.0, 180.0, DisplayFormat::milliseconds, ControlIntent::fader,
                         make_display("Dec", "Decay", "Envelope", "Decay time."), "ms"),
              make_param("sustain", 0.0, 1.0, 0.75, DisplayFormat::percentage, ControlIntent::fader,
                         make_display("Sus", "Sustain", "Envelope", "Sustain level."), "%"),
              make_param("release_ms", 0.0, 5000.0, 250.0, DisplayFormat::milliseconds, ControlIntent::fader,
                         make_display("Rel", "Release", "Envelope", "Release time."), "ms"),
          },
          .exported_controls = {},
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::light,
          .display = make_display("ADSR", "Envelope", "Mod", "Amplitude or filter envelope."),
      },
      DeviceTypeDefinition{
          .id = "event.clock",
          .category = DeviceCategory::event,
          .description = "Clock and timing source.",
          .ports = {
              DevicePortDefinition{.id = "clock_out", .signal_family = SignalFamily::event, .direction = PortDirection::output, .event_type = EventSignalType::clock},
              DevicePortDefinition{.id = "transport_in", .signal_family = SignalFamily::event, .direction = PortDirection::input, .event_type = EventSignalType::transport},
          },
          .parameters = {
              make_param("bpm", 20.0, 300.0, 120.0, DisplayFormat::plain, ControlIntent::knob,
                         make_display("BPM", "Tempo", "Clock", "Tempo in beats per minute."), "bpm"),
          },
          .exported_controls = {},
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::light,
          .display = make_display("Clock", "Tempo Clock", "Event", "Clock source for sequencers and arps."),
      },
      DeviceTypeDefinition{
          .id = "fx.delay",
          .category = DeviceCategory::fx,
          .description = "Delay effect.",
          .ports = {
              DevicePortDefinition{.id = "audio_in", .signal_family = SignalFamily::audio, .direction = PortDirection::input},
              DevicePortDefinition{.id = "clock_in", .signal_family = SignalFamily::event, .direction = PortDirection::input, .event_type = EventSignalType::clock},
              DevicePortDefinition{.id = "audio_out", .signal_family = SignalFamily::audio, .direction = PortDirection::output},
          },
          .parameters = {
              make_param("mix", 0.0, 1.0, 0.2, DisplayFormat::percentage, ControlIntent::knob,
                         make_display("Mix", "Delay Mix", "Delay", "Delay wet mix."), "%"),
              make_param("time_ms", 1.0, 2000.0, 320.0, DisplayFormat::milliseconds, ControlIntent::knob,
                         make_display("Time", "Delay Time", "Delay", "Delay time."), "ms"),
          },
          .exported_controls = {},
          .child_layout = std::nullopt,
          .cpu_class = CpuClass::medium,
          .display = make_display("Dly", "Delay", "FX", "Delay effect."),
      },
      DeviceTypeDefinition{
          .id = "composite.voice_strip",
          .category = DeviceCategory::composite,
          .description = "Composite voice strip with constrained nested devices.",
          .ports = {
              DevicePortDefinition{.id = "note_in", .signal_family = SignalFamily::event, .direction = PortDirection::input, .event_type = EventSignalType::note},
              DevicePortDefinition{.id = "audio_out", .signal_family = SignalFamily::audio, .direction = PortDirection::output},
          },
          .parameters = {},
          .exported_controls = {},
          .child_layout = ChildLayoutRule{
              .allowed_child_categories = {DeviceCategory::source, DeviceCategory::tone, DeviceCategory::mod, DeviceCategory::mix, DeviceCategory::utility},
              .max_child_count = 8,
              .children_visible = true,
              .bubble_exports = true,
          },
          .cpu_class = CpuClass::heavy,
          .display = make_display("Voice", "Voice Strip", "Composite", "Reusable subtractive voice composite."),
      },
  };
}

auto starter_subtractive_instrument_definition() -> InstrumentDefinition {
  return InstrumentDefinition{
      .id = "instrument.subtractive_test",
      .version = 1,
      .name = "Subtractive Test",
      .devices = {
          DeviceInstance{.id = "voice_strip", .type_id = "composite.voice_strip"},
          DeviceInstance{.id = "oscillator", .type_id = "source.oscillator"},
          DeviceInstance{.id = "filter", .type_id = "tone.multimode_filter"},
          DeviceInstance{.id = "amp_env", .type_id = "mod.adsr"},
          DeviceInstance{.id = "vca", .type_id = "mix.vca"},
          DeviceInstance{.id = "clock", .type_id = "event.clock"},
          DeviceInstance{.id = "delay", .type_id = "fx.delay"},
      },
      .connections = {
          DeviceConnection{.id = "osc-to-filter", .source = {.device_id = "oscillator", .port_id = "audio_out"}, .target = {.device_id = "filter", .port_id = "audio_in"}},
          DeviceConnection{.id = "filter-to-vca", .source = {.device_id = "filter", .port_id = "audio_out"}, .target = {.device_id = "vca", .port_id = "audio_in"}},
          DeviceConnection{.id = "env-to-vca", .source = {.device_id = "amp_env", .port_id = "mod_out"}, .target = {.device_id = "vca", .port_id = "mod_in"}},
          DeviceConnection{.id = "clock-to-delay", .source = {.device_id = "clock", .port_id = "clock_out"}, .target = {.device_id = "delay", .port_id = "clock_in"}},
      },
      .exported_controls = {
          ExportedControlDefinition{
              .id = "osc-waveform",
              .source_device_id = "oscillator",
              .source_parameter_id = "waveform",
              .display = make_display("Wave", "Waveform", "Oscillator", "Oscillator waveform."),
              .control_intent = ControlIntent::knob,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "osc",
          },
          ExportedControlDefinition{
              .id = "osc-shape",
              .source_device_id = "oscillator",
              .source_parameter_id = "shape",
              .display = make_display("Shape", "Shape", "Oscillator", "Oscillator shape."),
              .control_intent = ControlIntent::knob,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "osc",
          },
          ExportedControlDefinition{
              .id = "filter-cutoff",
              .source_device_id = "filter",
              .source_parameter_id = "cutoff",
              .display = make_display("Cut", "Cutoff", "Filter", "Filter cutoff."),
              .control_intent = ControlIntent::knob,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "filter",
          },
          ExportedControlDefinition{
              .id = "filter-resonance",
              .source_device_id = "filter",
              .source_parameter_id = "resonance",
              .display = make_display("Reso", "Resonance", "Filter", "Filter resonance."),
              .control_intent = ControlIntent::knob,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "filter",
          },
          ExportedControlDefinition{
              .id = "amp-attack",
              .source_device_id = "amp_env",
              .source_parameter_id = "attack_ms",
              .display = make_display("Atk", "Attack", "Envelope", "Amp envelope attack."),
              .control_intent = ControlIntent::fader,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "env",
          },
          ExportedControlDefinition{
              .id = "amp-release",
              .source_device_id = "amp_env",
              .source_parameter_id = "release_ms",
              .display = make_display("Rel", "Release", "Envelope", "Amp envelope release."),
              .control_intent = ControlIntent::fader,
              .visible = true,
              .automation_visible = true,
              .preferred_page = "env",
          },
      },
      .default_pages = {"osc", "filter", "env"},
      .validation_notes = {"starter-subtractive-composite"},
  };
}

auto validate_instrument_definition(
    const InstrumentDefinition& instrument,
    const std::vector<DeviceTypeDefinition>& device_types) -> std::vector<GraphValidationIssue> {
  std::vector<GraphValidationIssue> issues;

  std::unordered_map<std::string, const DeviceTypeDefinition*> type_by_id;
  for (const auto& device_type : device_types) {
    type_by_id.emplace(device_type.id, &device_type);
  }

  std::unordered_map<std::string, const DeviceInstance*> instance_by_id;
  for (const auto& instance : instrument.devices) {
    instance_by_id.emplace(instance.id, &instance);
    validate_children(instance, type_by_id, issues);
  }

  for (const auto& connection : instrument.connections) {
    const auto source_instance_it = instance_by_id.find(connection.source.device_id);
    const auto target_instance_it = instance_by_id.find(connection.target.device_id);
    if (source_instance_it == instance_by_id.end() || target_instance_it == instance_by_id.end()) {
      issues.push_back({
          .code = "unknown-device-instance",
          .message = "Connection references an unknown device instance",
          .connection_id = connection.id,
      });
      continue;
    }

    const auto source_type_it = type_by_id.find(source_instance_it->second->type_id);
    const auto target_type_it = type_by_id.find(target_instance_it->second->type_id);
    if (source_type_it == type_by_id.end() || target_type_it == type_by_id.end()) {
      issues.push_back({
          .code = "unknown-device-type",
          .message = "Connection references a device instance with an unknown type",
          .connection_id = connection.id,
      });
      continue;
    }

    const auto* source_port = find_port(*source_type_it->second, connection.source.port_id, PortDirection::output);
    const auto* target_port = find_port(*target_type_it->second, connection.target.port_id, PortDirection::input);
    if (!source_port || !target_port) {
      issues.push_back({
          .code = "unknown-port",
          .message = "Connection references a missing or directionally invalid port",
          .connection_id = connection.id,
      });
      continue;
    }

    if (source_port->signal_family != target_port->signal_family) {
      issues.push_back({
          .code = "signal-family-mismatch",
          .message = "Connection mixes incompatible signal families",
          .connection_id = connection.id,
      });
      continue;
    }

    if (source_port->signal_family == SignalFamily::event && !is_event_compatible(*source_port, *target_port)) {
      issues.push_back({
          .code = "event-type-mismatch",
          .message = "Connection mixes incompatible event signal types",
          .connection_id = connection.id,
      });
    }
  }

  validate_exported_controls(instrument, instance_by_id, type_by_id, issues);
  validate_acyclic_graph(instrument, issues);

  return issues;
}

auto to_string(DeviceCategory value) -> std::string_view {
  switch (value) {
    case DeviceCategory::source:
      return "source";
    case DeviceCategory::tone:
      return "tone";
    case DeviceCategory::mod:
      return "mod";
    case DeviceCategory::event:
      return "event";
    case DeviceCategory::mix:
      return "mix";
    case DeviceCategory::fx:
      return "fx";
    case DeviceCategory::utility:
      return "utility";
    case DeviceCategory::composite:
      return "composite";
  }
  return "unknown";
}

auto to_string(SignalFamily value) -> std::string_view {
  switch (value) {
    case SignalFamily::audio:
      return "audio";
    case SignalFamily::mod:
      return "mod";
    case SignalFamily::event:
      return "event";
  }
  return "unknown";
}

auto to_string(EventSignalType value) -> std::string_view {
  switch (value) {
    case EventSignalType::any:
      return "any";
    case EventSignalType::note:
      return "note";
    case EventSignalType::gate:
      return "gate";
    case EventSignalType::trigger:
      return "trigger";
    case EventSignalType::clock:
      return "clock";
    case EventSignalType::transport:
      return "transport";
    case EventSignalType::sequence:
      return "sequence";
  }
  return "unknown";
}

}  // namespace pi_fartbox::engine
