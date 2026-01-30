#include "theme.h"

#include <fstream>
#include <regex>
#include <sstream>

namespace prodigeetor {

static uint32_t parse_hex_color(const std::string &hex) {
  std::string value = hex;
  if (!value.empty() && value[0] == '#') {
    value.erase(value.begin());
  }
  uint32_t color = 0xFFFFFFFF;
  if (value.size() == 6) {
    unsigned int rgb = 0;
    std::stringstream ss;
    ss << std::hex << value;
    ss >> rgb;
    color = 0xFF000000 | rgb;
  } else if (value.size() == 8) {
    unsigned int argb = 0;
    std::stringstream ss;
    ss << std::hex << value;
    ss >> argb;
    color = argb;
  }
  return color;
}

SyntaxTheme SyntaxTheme::load_from_file(const std::string &path) {
  SyntaxTheme theme;
  theme.m_default_style.fg_color = 0xFFFFFFFF;

  std::ifstream file(path);
  if (!file.is_open()) {
    return theme;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::regex pair_regex(R"regex("([^"]+)"\s*:\s*"(#?[0-9a-fA-F]{6,8})")regex");
  auto begin = std::sregex_iterator(content.begin(), content.end(), pair_regex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::string key = (*it)[1].str();
    std::string color = (*it)[2].str();
    if (key == "name") {
      continue;
    }
    RenderStyle style;
    style.fg_color = parse_hex_color(color);
    theme.m_capture_styles[key] = style;
  }

  return theme;
}

RenderStyle SyntaxTheme::style_for_capture(const std::string &capture) const {
  auto it = m_capture_styles.find(capture);
  if (it != m_capture_styles.end()) {
    return it->second;
  }
  return m_default_style;
}

} // namespace prodigeetor
