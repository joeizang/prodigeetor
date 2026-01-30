#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "text_types.h"

namespace prodigeetor {

struct Edit {
  size_t offset = 0;
  std::string inserted;
  std::string removed;
};

class TextBuffer {
public:
  TextBuffer();
  explicit TextBuffer(std::string initial_text);

  size_t size() const;
  bool empty() const;
  std::string text() const;

  void insert(size_t offset, std::string_view text);
  void erase(size_t offset, size_t length);
  Edit replace(size_t offset, size_t length, std::string_view text);

  size_t line_count() const;
  size_t line_start(size_t line_index) const;
  std::string line_text(size_t line_index) const;
  size_t line_grapheme_count(size_t line_index) const;

  Position position_at(size_t offset) const;
  size_t offset_at(const Position &pos) const;

private:
  std::string m_left;
  std::string m_right_reversed;
  mutable std::vector<size_t> m_line_starts;
  mutable bool m_line_index_dirty = true;

  void move_gap(size_t offset);
  char char_at(size_t offset) const;
  std::string slice(size_t start, size_t end) const;
  void ensure_line_index() const;
};

} // namespace prodigeetor
