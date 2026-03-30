#pragma once

#include "pi_fartbox/engine/device_schema.hpp"
#include "pi_fartbox/engine/engine_runtime.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pi_fartbox::controller {

struct SlmkiiiMapperConfig {
  std::string device_name = "Novation SL MkIII";
  std::uint32_t slot_count = 4;
  bool enable_sysex_feedback = true;
};

enum class MidiMessageType {
  note_on,
  note_off,
  control_change,
  sysex
};

struct MidiMessage {
  MidiMessageType type = MidiMessageType::control_change;
  std::uint8_t channel = 1;
  std::uint8_t data1 = 0;
  std::uint8_t data2 = 0;
  std::vector<std::uint8_t> sysex;
};

enum class ControllerActionType {
  none,
  focus_slot,
  next_page,
  previous_page,
  show_system_page
};

struct ControllerAction {
  ControllerActionType type = ControllerActionType::none;
  std::uint32_t slot_id = 0;
};

struct PortRoleStatus {
  std::string midi_input_name;
  std::string midi_output_name;
  bool input_connected = false;
  bool output_connected = false;
  bool sysex_enabled = false;
};

struct InControlDisplayColumn {
  std::uint8_t column = 0;
  std::string row1;
  std::string row3;
  std::uint8_t value = 0;
};

struct InControlDisplayPage {
  std::string id;
  std::string title;
  std::vector<InControlDisplayColumn> columns;
  std::vector<std::vector<std::uint8_t>> sysex_messages;
};

struct ControllerContext {
  std::uint32_t focused_slot_id = 1;
  std::size_t active_page_index = 0;
  std::vector<engine::ControllerPageDefinition> pages;
  std::optional<InControlDisplayPage> active_display_page;
  PortRoleStatus port_status;
};

class MidiRouter {
 public:
  [[nodiscard]] auto route_note_message(const MidiMessage& message) const -> std::optional<engine::NoteEvent>;
  [[nodiscard]] auto route_controller_action(const MidiMessage& message) const -> std::optional<ControllerAction>;

  static auto slot_channel_for_slot(std::uint32_t slot_id) -> std::uint8_t;
};

class SlmkiiiControllerCore {
 public:
  explicit SlmkiiiControllerCore(SlmkiiiMapperConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const SlmkiiiMapperConfig&;
  [[nodiscard]] auto make_context(const engine::EngineRuntime& runtime, PortRoleStatus port_status = {}) const -> ControllerContext;
  [[nodiscard]] auto decode_relative_delta(std::uint8_t value) const -> std::int32_t;
  [[nodiscard]] auto build_set_layout_sysex(std::uint8_t layout_index = 0x01) const -> std::vector<std::uint8_t>;
  [[nodiscard]] auto build_set_properties_sysex(const InControlDisplayPage& page) const -> std::vector<std::uint8_t>;
  [[nodiscard]] auto build_notification_sysex(std::string line1, std::string line2) const -> std::vector<std::uint8_t>;
  [[nodiscard]] auto build_set_led_rgb_sysex(std::uint8_t led_index, std::uint8_t red, std::uint8_t green, std::uint8_t blue) const
      -> std::vector<std::uint8_t>;

 private:
  [[nodiscard]] auto build_workstation_pages(const std::vector<engine::SlotRuntime>& slots) const -> std::vector<engine::ControllerPageDefinition>;
  [[nodiscard]] auto build_instrument_pages(const engine::SlotRuntime& slot) const -> std::vector<engine::ControllerPageDefinition>;
  [[nodiscard]] auto build_system_page(PortRoleStatus port_status) const -> engine::ControllerPageDefinition;
  [[nodiscard]] auto build_active_display_page(const engine::EngineRuntime& runtime,
                                               const engine::SlotRuntime& focused_slot,
                                               const std::vector<engine::ControllerPageDefinition>& pages,
                                               std::size_t page_index) const -> InControlDisplayPage;

  SlmkiiiMapperConfig config_;
};

}  // namespace pi_fartbox::controller
