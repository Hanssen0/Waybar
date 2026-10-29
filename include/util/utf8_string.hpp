#pragma once
#include <string>

namespace waybar::util {
std::string utf8_make_valid(const std::string& str);
size_t utf8_width(const std::string& str);
void utf8_truncate(std::string& s, const std::string& ellipsis, size_t max_len);
}  // namespace waybar::util
