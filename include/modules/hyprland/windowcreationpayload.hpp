#pragma once

#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <json/value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <variant>
#include <vector>

#include "AModule.hpp"
#include "bar.hpp"
#include "modules/hyprland/backend.hpp"
#include "modules/hyprland/workspace_identity.hpp"
#include "util/enum.hpp"
#include "util/regex_collection.hpp"

using WindowAddress = std::string;

namespace waybar::modules::hyprland {

class Workspaces;

struct WindowRepr {
  std::string address;
  std::string window_class;
  std::string window_title;
  std::string repr_rewrite;
  bool isActive = false;

 public:
  bool empty() const { return address.empty(); }
  void setActive(bool value) { isActive = value; }
};

class WindowCreationPayload {
 public:
  WindowCreationPayload(std::string workspace_identifier, WindowAddress window_address,
                        WindowRepr window_repr);
  WindowCreationPayload(std::string workspace_identifier, WindowAddress window_address,
                        const std::string& window_class, const std::string& window_title,
                        bool is_active);
  WindowCreationPayload(Json::Value const& client_data);

  int incrementTimeSpentUncreated();
  bool isEmpty(Workspaces& workspace_manager);
  bool reprIsReady() const { return std::holds_alternative<Repr>(m_window); }
  WindowRepr repr(Workspaces& workspace_manager);
  void setActive(bool value) { m_isActive = value; }

  bool targetsWorkspace(const WorkspaceIdentity& workspace) const {
    return workspace.matchesWindowDestination(m_workspaceIdentifier, m_workspaceEvent);
  }
  WindowAddress getAddress() const { return m_windowAddress; }

  void moveToWorkspace(std::string workspace_address);
  void updateWindow(WindowCreationPayload payload);

 private:
  void clearAddr();

  using Repr = WindowRepr;
  using ClassAndTitle = std::pair<std::string, std::string>;
  std::variant<Repr, ClassAndTitle> m_window;

  WindowAddress m_windowAddress;
  std::string m_workspaceIdentifier;
  WorkspaceWindowEvent m_workspaceEvent = WorkspaceWindowEvent::Move;
  bool m_isActive = false;

  int m_timeSpentUncreated = 0;
};

}  // namespace waybar::modules::hyprland
