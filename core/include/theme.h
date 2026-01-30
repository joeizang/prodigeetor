#pragma once

#include <string>
#include <unordered_map>

#include "rendering.h"

namespace prodigeetor {

class SyntaxTheme {
public:
  static SyntaxTheme load_from_file(const std::string &path);

  RenderStyle style_for_capture(const std::string &capture) const;

private:
  RenderStyle m_default_style;
  std::unordered_map<std::string, RenderStyle> m_capture_styles;
};

} // namespace prodigeetor
