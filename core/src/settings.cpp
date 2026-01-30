#include "settings.h"

#include <fstream>
#include <regex>
#include <sstream>

namespace prodigeetor {

static std::string read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::string();
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

EditorSettings SettingsLoader::load_from_file(const std::string &path) {
  EditorSettings settings;
  std::string content = read_file(path);
  if (content.empty()) {
    return settings;
  }

  std::regex font_regex(R"regex("fontFamily"\s*:\s*"([^"]+)")regex");
  std::regex size_regex(R"regex("fontSize"\s*:\s*([0-9]+(\.[0-9]+)?)")regex");
  std::regex liga_regex(R"regex("fontLigatures"\s*:\s*(true|false))regex");
  std::regex fallback_regex(R"regex("fontFallbacks"\s*:\s*\[([^\]]*)\])regex");

  std::smatch match;
  if (std::regex_search(content, match, font_regex)) {
    settings.font_family = match[1].str();
  }
  if (std::regex_search(content, match, size_regex)) {
    settings.font_size = std::stof(match[1].str());
  }
  if (std::regex_search(content, match, liga_regex)) {
    settings.font_ligatures = (match[1].str() == "true");
  }
  if (std::regex_search(content, match, fallback_regex)) {
    settings.font_fallbacks.clear();
    std::string list = match[1].str();
    std::regex item_regex(R"regex("([^"]+)")regex");
    auto begin = std::sregex_iterator(list.begin(), list.end(), item_regex);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
      settings.font_fallbacks.push_back((*it)[1].str());
    }
  }

  return settings;
}

} // namespace prodigeetor
