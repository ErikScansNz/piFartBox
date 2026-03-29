#pragma once

#include <string>
#include <string_view>

namespace pi_fartbox::session {

struct SessionPaths {
  std::string session_root = "/var/lib/piFartBox/sessions";
  std::string content_root = "/opt/piFartBox/content";
  std::string controller_profile_root = "/opt/piFartBox/content/controller-profiles";
};

class SessionManager {
 public:
  explicit SessionManager(SessionPaths paths = {});

  [[nodiscard]] auto paths() const noexcept -> const SessionPaths&;
  [[nodiscard]] auto subsystem_name() const noexcept -> std::string_view;

 private:
  SessionPaths paths_;
};

}  // namespace pi_fartbox::session
