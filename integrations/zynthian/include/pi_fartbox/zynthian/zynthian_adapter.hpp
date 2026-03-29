#pragma once

#include <string>
#include <string_view>

namespace pi_fartbox::zynthian {

struct ZynthianAdapterConfig {
  std::string engine_id = "piFartBox";
  std::string preset_root = "/zynthian/zynthian-my-data/presets/piFartBox";
};

class ZynthianAdapter {
 public:
  explicit ZynthianAdapter(ZynthianAdapterConfig config = {});

  [[nodiscard]] auto config() const noexcept -> const ZynthianAdapterConfig&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

 private:
  ZynthianAdapterConfig config_;
};

}  // namespace pi_fartbox::zynthian
