#pragma once

#include <vector>

#include "text_buffer.h"

namespace prodigeetor {

class UndoStack {
public:
  void push(const Edit &edit);
  bool can_undo() const;
  bool can_redo() const;
  Edit undo();
  Edit redo();
  void clear();

private:
  std::vector<Edit> m_undo;
  std::vector<Edit> m_redo;
};

} // namespace prodigeetor
