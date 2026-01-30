#include "undo_stack.h"

namespace prodigeetor {

void UndoStack::push(const Edit &edit) {
  m_undo.push_back(edit);
  m_redo.clear();
}

bool UndoStack::can_undo() const {
  return !m_undo.empty();
}

bool UndoStack::can_redo() const {
  return !m_redo.empty();
}

Edit UndoStack::undo() {
  if (m_undo.empty()) {
    return Edit{};
  }
  Edit edit = m_undo.back();
  m_undo.pop_back();
  m_redo.push_back(edit);
  return edit;
}

Edit UndoStack::redo() {
  if (m_redo.empty()) {
    return Edit{};
  }
  Edit edit = m_redo.back();
  m_redo.pop_back();
  m_undo.push_back(edit);
  return edit;
}

void UndoStack::clear() {
  m_undo.clear();
  m_redo.clear();
}

} // namespace prodigeetor
