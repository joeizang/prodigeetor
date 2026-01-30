#pragma once

#include <cstdint>
#include <string>

namespace prodigeetor {

struct Position {
  uint32_t line = 0;
  uint32_t column = 0;

  bool operator==(const Position &other) const {
    return line == other.line && column == other.column;
  }
};

struct Range {
  Position start;
  Position end;
};

struct Selection {
  Position anchor;
  Position active;
};

struct Diagnostic {
  Range range;
  std::string message;
  std::string source;
  int severity = 0;
};

} // namespace prodigeetor
