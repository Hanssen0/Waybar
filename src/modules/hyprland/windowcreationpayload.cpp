#include "modules/hyprland/windowcreationpayload.hpp"

#include <glibmm/ustring.h>
#include <json/value.h>
#include <spdlog/spdlog.h>

#include <string>
#include <utility>
#include <variant>

#include "modules/hyprland/workspaces.hpp"

namespace waybar::modules::hyprland {

// Hyprland cuts socket2 events at 1024 bytes, possibly mid-character, and GTK rejects a label
// whose text is not valid UTF-8.
static std::string makeValidUtf8(std::string text) {
  if (!g_utf8_validate(text.data(), static_cast<gssize>(text.size()), nullptr)) {
    text = Glib::ustring(text).make_valid().raw();
  }
  return text;
}

WindowCreationPayload::WindowCreationPayload(Json::Value const& client_data)
    : m_window(std::make_pair(makeValidUtf8(client_data["class"].asString()),
                              makeValidUtf8(client_data["title"].asString()))),
      m_windowAddress(client_data["address"].asString()) {
  clearAddr();
}

WindowCreationPayload::WindowCreationPayload(std::string workspace_identifier,
                                             WindowAddress window_address, WindowRepr window_repr)
    : m_window(std::move(window_repr)),
      m_windowAddress(std::move(window_address)),
      m_workspaceIdentifier(std::move(workspace_identifier)) {
  clearAddr();
}

WindowCreationPayload::WindowCreationPayload(std::string workspace_identifier,
                                             WindowAddress window_address,
                                             const std::string& window_class,
                                             const std::string& window_title, bool is_active)
    : m_window(std::make_pair(makeValidUtf8(window_class), makeValidUtf8(window_title))),
      m_windowAddress(std::move(window_address)),
      m_workspaceIdentifier(std::move(workspace_identifier)),
      m_workspaceEvent(WorkspaceWindowEvent::Open),
      m_isActive(is_active) {
  clearAddr();
}

void WindowCreationPayload::clearAddr() {
  // substr(2, ...) is necessary because Hyprland's JSON follows this format:
  // 0x{ADDR}
  // While Hyprland's IPC follows this format:
  // {ADDR}
  static const std::string ADDR_PREFIX = "0x";
  static const int ADDR_PREFIX_LEN = ADDR_PREFIX.length();

  if (m_windowAddress.starts_with(ADDR_PREFIX)) {
    m_windowAddress =
        m_windowAddress.substr(ADDR_PREFIX_LEN, m_windowAddress.length() - ADDR_PREFIX_LEN);
  }
}

bool WindowCreationPayload::isEmpty(Workspaces& workspace_manager) {
  if (std::holds_alternative<Repr>(m_window)) {
    return std::get<Repr>(m_window).empty();
  }
  if (std::holds_alternative<ClassAndTitle>(m_window)) {
    auto [window_class, window_title] = std::get<ClassAndTitle>(m_window);
    return (window_class.empty() &&
            (!workspace_manager.windowRewriteConfigUsesTitle() || window_title.empty()));
  }
  // Unreachable
  spdlog::error("WorkspaceWindow::isEmpty: Unreachable");
  throw std::runtime_error("WorkspaceWindow::isEmpty: Unreachable");
}

int WindowCreationPayload::incrementTimeSpentUncreated() { return m_timeSpentUncreated++; }

void WindowCreationPayload::moveToWorkspace(std::string workspace_address) {
  m_workspaceIdentifier = std::move(workspace_address);
  m_workspaceEvent = WorkspaceWindowEvent::Move;
}

void WindowCreationPayload::updateWindow(WindowCreationPayload payload) {
  // Refresh class/title without replacing the queued destination or active state.
  m_window = std::move(payload.m_window);
}

WindowRepr WindowCreationPayload::repr(Workspaces& workspace_manager) {
  if (std::holds_alternative<Repr>(m_window)) {
    return std::get<Repr>(m_window);
  }
  if (std::holds_alternative<ClassAndTitle>(m_window)) {
    auto const& [window_class, window_title] = std::get<ClassAndTitle>(m_window);
    return {m_windowAddress, window_class, window_title,
            workspace_manager.getRewrite(window_class, window_title), m_isActive};
  }
  // Unreachable
  spdlog::error("WorkspaceWindow::repr: Unreachable");
  throw std::runtime_error("WorkspaceWindow::repr: Unreachable");
}

}  // namespace waybar::modules::hyprland
