#pragma once

#include <cstddef>
#include <string_view>
#include <memory>

#include "text_buffer.h"
#include "undo_stack.h"
#include "lsp_manager.h"
#include "syntax_highlighter.h"

namespace prodigeetor {

class Core {
public:
  Core();
  ~Core();
  void initialize();
  void initialize_lsp(const std::string& root_path);

  TextBuffer &buffer();
  const TextBuffer &buffer() const;

  UndoStack &undo_stack();
  const UndoStack &undo_stack() const;

  lsp::LSPManager &lsp_manager();
  const lsp::LSPManager &lsp_manager() const;

  TreeSitterHighlighter &syntax_highlighter();
  const TreeSitterHighlighter &syntax_highlighter() const;

  void insert(size_t offset, std::string_view text);
  void erase(size_t offset, size_t length);
  size_t delete_backward(size_t offset);

  void set_text(std::string text);
  size_t line_count() const;
  std::string line_text(size_t line_index) const;
  size_t line_grapheme_count(size_t line_index) const;
  Position position_at(size_t offset) const;
  size_t offset_at(const Position &pos) const;

  // File management
  void open_file(const std::string& uri, const std::string& language_id);
  void close_file(const std::string& uri);
  void save_file(const std::string& uri);

  // Process LSP messages
  void tick();

private:
  TextBuffer m_buffer;
  UndoStack m_undo;
  std::unique_ptr<lsp::LSPManager> m_lsp_manager;
  TreeSitterHighlighter m_syntax_highlighter;
};

} // namespace prodigeetor
