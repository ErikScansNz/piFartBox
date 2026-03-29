#include "pi_fartbox/session/session_manager.hpp"

namespace pi_fartbox::session {

SessionManager::SessionManager(SessionPaths paths) : paths_(paths) {}

auto SessionManager::paths() const noexcept -> const SessionPaths& {
  return paths_;
}

auto SessionManager::subsystem_name() const noexcept -> std::string_view {
  return "session-manager";
}

}  // namespace pi_fartbox::session
