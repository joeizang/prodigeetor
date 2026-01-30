#pragma once

#include <cstddef>
#include <string_view>

#include "text_buffer.h"
#include "undo_stack.h"

namespace prodigeetor {

class Core {
public:
  Core();
  void initialize();

  TextBuffer &buffer();
  const TextBuffer &buffer() const;

  UndoStack &undo_stack();
  const UndoStack &undo_stack() const;

  void insert(size_t offset, std::string_view text);
  void erase(size_t offset, size_t length);
  size_t delete_backward(size_t offset);

  void set_text(std::string text);
  size_t line_count() const;
  std::string line_text(size_t line_index) const;
  size_t line_grapheme_count(size_t line_index) const;
  Position position_at(size_t offset) const;
  size_t offset_at(const Position &pos) const;

private:
  TextBuffer m_buffer;
  UndoStack m_undo;
};

} // namespace prodigeetor
