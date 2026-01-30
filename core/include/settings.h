#pragma once

#include <string>
#include <vector>

namespace prodigeetor {

struct EditorSettings {
  std::string font_family = "Monoid";
  std::vector<std::string> font_fallbacks = {"Menlo", "Fira Code", "monospace"};
  bool font_ligatures = true;
  float font_size = 14.0f;
};

class SettingsLoader {
public:
  static EditorSettings load_from_file(const std::string &path);
};

} // namespace prodigeetor
