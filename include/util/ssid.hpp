#pragma once

#include <glib.h>
#include <glibmm/markup.h>

#include <memory>
#include <string>

namespace waybar::util {

// SSIDs may contain non-UTF-8 bytes.
inline std::string ssid_to_markup(const std::string& ssid) {
  const std::unique_ptr<gchar, decltype(&g_free)> valid(g_utf8_make_valid(ssid.data(), ssid.size()),
                                                        g_free);
  return Glib::Markup::escape_text(valid.get()).raw();
}

}  // namespace waybar::util
