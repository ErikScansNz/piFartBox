#include "pi_fartbox/engine/device_registry.hpp"

#include <algorithm>
#include <map>

namespace pi_fartbox::engine {

DeviceRegistry::DeviceRegistry() : device_types_(starter_device_palette()) {}

DeviceRegistry::DeviceRegistry(std::vector<DeviceTypeDefinition> device_types)
    : device_types_(std::move(device_types)) {}

auto DeviceRegistry::device_types() const noexcept -> const std::vector<DeviceTypeDefinition>& {
  return device_types_;
}

auto DeviceRegistry::find_device_type(std::string_view type_id) const -> const DeviceTypeDefinition* {
  const auto it = std::find_if(device_types_.begin(), device_types_.end(), [type_id](const DeviceTypeDefinition& device_type) {
    return device_type.id == type_id;
  });
  return (it != device_types_.end()) ? &(*it) : nullptr;
}

auto DeviceRegistry::validate_instrument(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue> {
  return validate_instrument_definition(instrument, device_types_);
}

void DeviceRegistry::replace_device_types(std::vector<DeviceTypeDefinition> device_types) {
  device_types_ = std::move(device_types);
}

void DeviceRegistry::register_device_type(DeviceTypeDefinition device_type) {
  const auto existing = std::find_if(device_types_.begin(), device_types_.end(), [&device_type](const DeviceTypeDefinition& current) {
    return current.id == device_type.id;
  });
  if (existing != device_types_.end()) {
    *existing = std::move(device_type);
    return;
  }
  device_types_.push_back(std::move(device_type));
}

InstrumentCompiler::InstrumentCompiler(const DeviceRegistry& registry) : registry_(registry) {}

auto InstrumentCompiler::validate(const InstrumentDefinition& instrument) const -> std::vector<GraphValidationIssue> {
  return registry_.validate_instrument(instrument);
}

auto InstrumentCompiler::compile(const InstrumentDefinition& instrument) const -> std::optional<CompiledInstrument> {
  if (!validate(instrument).empty()) {
    return std::nullopt;
  }

  RuntimeGraph runtime_graph;
  for (const auto& device : instrument.devices) {
    const auto* device_type = registry_.find_device_type(device.type_id);
    if (!device_type) {
      return std::nullopt;
    }

    const DeviceExecutionNode node{
        .device_id = device.id,
        .type_id = device.type_id,
        .category = device_type->category,
    };

    switch (device_type->category) {
      case DeviceCategory::event:
        runtime_graph.event_nodes.push_back(node);
        break;
      case DeviceCategory::mod:
        runtime_graph.mod_nodes.push_back(node);
        break;
      default:
        runtime_graph.audio_nodes.push_back(node);
        break;
    }
  }

  std::map<std::string, CompiledInstrumentPage> pages_by_id;
  for (const auto& control : instrument.exported_controls) {
    const auto page_id = control.preferred_page.empty() ? std::string("instrument") : control.preferred_page;
    auto& page = pages_by_id[page_id];
    page.id = page_id;
    page.title = page_id;
    page.controls.push_back(control);
  }

  std::vector<CompiledInstrumentPage> generated_pages;
  generated_pages.reserve(pages_by_id.size());
  for (const auto& page_id : instrument.default_pages) {
    const auto page_it = pages_by_id.find(page_id);
    if (page_it == pages_by_id.end()) {
      continue;
    }
    generated_pages.push_back(std::move(page_it->second));
    pages_by_id.erase(page_it);
  }
  for (auto& [page_id, page] : pages_by_id) {
    generated_pages.push_back(std::move(page));
  }

  VoiceDefinition voice_definition{
      .composite_device_id = "voice_strip",
      .polyphony = 8,
      .mod_routes = {
          CuratedModRoute{.source = CuratedModSource::amp_envelope, .target = CuratedModTarget::vca_level, .depth = 1.0},
          CuratedModRoute{.source = CuratedModSource::filter_envelope, .target = CuratedModTarget::filter_cutoff, .depth = 1.0},
          CuratedModRoute{.source = CuratedModSource::lfo, .target = CuratedModTarget::filter_cutoff, .depth = 0.35},
          CuratedModRoute{.source = CuratedModSource::velocity, .target = CuratedModTarget::vca_level, .depth = 0.5},
      },
  };

  return CompiledInstrument{
      .instrument = instrument,
      .runtime_graph = std::move(runtime_graph),
      .voice_definition = std::move(voice_definition),
      .exported_controls = instrument.exported_controls,
      .generated_pages = std::move(generated_pages),
  };
}

}  // namespace pi_fartbox::engine
