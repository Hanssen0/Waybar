#include "util/ssid.hpp"

#include <fmt/args.h>
#include <fmt/format.h>
#include <pango/pango.h>

#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
const std::string REPLACEMENT = "\xef\xbf\xbd";

const std::vector<std::pair<std::string, std::string>> SSIDS = {
    {"", ""},
    {"Home Wi-Fi", "Home Wi-Fi"},
    {"SZ-WLAN（深i网)", "SZ-WLAN（深i网)"},
    {"Wi-Fi 📶😀", "Wi-Fi 📶😀"},
    {"SZ-WLAN\xa3\xa8\xc9\xee\x69\xcd\xf8)", "SZ-WLAN" + REPLACEMENT + REPLACEMENT + REPLACEMENT +
                                                 REPLACEMENT + "i" + REPLACEMENT + REPLACEMENT +
                                                 ")"},
    {"\xff\x80\xfe", REPLACEMENT + REPLACEMENT + REPLACEMENT},
    {"\xc0\xaf", REPLACEMENT + REPLACEMENT},                    // Overlong encoding.
    {"\xed\xa0\x80", REPLACEMENT + REPLACEMENT + REPLACEMENT},  // Surrogate.
    {"\xf4\x90\x80\x80", REPLACEMENT + REPLACEMENT + REPLACEMENT + REPLACEMENT},
    {"end\xc2", "end" + REPLACEMENT},
    {"end\xe4\xb8", "end" + REPLACEMENT + REPLACEMENT},
    {"end\xf0\x9f\x93", "end" + REPLACEMENT + REPLACEMENT + REPLACEMENT},
    {std::string("a\0b\0", 4), "a" + REPLACEMENT + "b" + REPLACEMENT},
    {"<Wi-Fi> & \"friends\" 'guest'", "<Wi-Fi> & \"friends\" 'guest'"},
    {"<\xff>&amp;", "<" + REPLACEMENT + ">&amp;"},
    {std::string(32, 'a'), std::string(32, 'a')},
    {std::string(32, '\xff'),
     [] {
       std::string text;
       for (int i = 0; i < 32; ++i) text += REPLACEMENT;
       return text;
     }()},
    {"📶📶📶📶📶📶📶📶", "📶📶📶📶📶📶📶📶"},
};

std::string parse_markup(const std::string& markup) {
  gchar* text = nullptr;
  GError* error = nullptr;
  const auto parsed =
      pango_parse_markup(markup.data(), markup.size(), 0, nullptr, &text, nullptr, &error);
  const std::unique_ptr<gchar, decltype(&g_free)> owned_text(text, g_free);
  const std::unique_ptr<GError, decltype(&g_error_free)> owned_error(error, g_error_free);
  INFO((error != nullptr ? error->message : ""));
  REQUIRE(parsed);
  return text;
}
}  // namespace

TEST_CASE("SSID display repairs UTF-8 before escaping markup", "[network][ssid]") {
  for (const auto& [raw, expected] : SSIDS) {
    INFO("SSID length: " << raw.size());
    const auto original = raw;
    const auto markup = waybar::util::ssid_to_markup(raw);
    REQUIRE(g_utf8_validate(markup.data(), markup.size(), nullptr));
    REQUIRE(markup.find('\0') == std::string::npos);
    REQUIRE(markup == Glib::Markup::escape_text(expected).raw());
    REQUIRE(parse_markup(markup) == expected);
    REQUIRE(raw == original);
  }
}

TEST_CASE("SSID markup survives network formatting and tooltip collation", "[network][ssid]") {
  for (const auto& [raw, expected] : SSIDS) {
    INFO("SSID length: " << raw.size());
    const auto essid = waybar::util::ssid_to_markup(raw);
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    store.push_back(fmt::arg("essid", essid));
    const auto label = fmt::vformat("<b>{essid}</b>", store);
    const auto tooltip = fmt::vformat("Connected to {essid}", store);
    REQUIRE(g_utf8_validate(label.data(), label.size(), nullptr));
    REQUIRE(g_utf8_validate(tooltip.data(), tooltip.size(), nullptr));
    REQUIRE(parse_markup(label) == expected);
    const Glib::ustring previous_tooltip = parse_markup(tooltip);
    REQUIRE(previous_tooltip.validate());
    // Exercise the g_utf8_collate() overload used by Waybar 0.15.0.
    REQUIRE_FALSE(previous_tooltip != std::string("Connected to " + expected));
    REQUIRE(previous_tooltip != std::string("Disconnected"));
    REQUIRE(Glib::ustring("Disconnected") != tooltip);
  }
}
