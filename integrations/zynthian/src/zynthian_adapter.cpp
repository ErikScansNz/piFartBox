#include "pi_fartbox/zynthian/zynthian_adapter.hpp"

namespace pi_fartbox::zynthian {

ZynthianAdapter::ZynthianAdapter(ZynthianAdapterConfig config) : config_(config) {}

auto ZynthianAdapter::config() const noexcept -> const ZynthianAdapterConfig& {
  return config_;
}

auto ZynthianAdapter::subsystem_name() const noexcept -> std::string_view {
  return "zynthian-adapter";
}

}  // namespace pi_fartbox::zynthian
