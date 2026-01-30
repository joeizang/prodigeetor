#include "core.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

namespace prodigeetor {

// Helper function to find language server executable
static std::string find_language_server(const std::string& command) {
  // Try common locations
  std::vector<std::string> search_paths;

  // 1. Check if command is already absolute path
  if (command[0] == '/' && access(command.c_str(), X_OK) == 0) {
    std::cerr << "[LSP] Found language server: " << command << std::endl;
    return command;
  }

  // 2. Get HOME directory
  const char* home = getenv("HOME");
  if (home) {
    // NVM paths
    search_paths.push_back(std::string(home) + "/.nvm/versions/node/v22.18.0/bin/" + command);
    search_paths.push_back(std::string(home) + "/.nvm/current/bin/" + command);
    // Global npm
    search_paths.push_back(std::string(home) + "/.npm-global/bin/" + command);
    // Homebrew
    search_paths.push_back("/opt/homebrew/bin/" + command);
    search_paths.push_back("/usr/local/bin/" + command);
  }

  // 3. System paths
  search_paths.push_back("/usr/bin/" + command);

  // Try each path
  for (const auto& path : search_paths) {
    if (access(path.c_str(), X_OK) == 0) {
      std::cerr << "[LSP] Found language server: " << path << std::endl;
      return path;
    }
  }

  // Not found, return original command and let it fail with a clear error
  std::cerr << "[LSP] WARNING: Could not find language server: " << command << std::endl;
  std::cerr << "[LSP] Searched paths:" << std::endl;
  for (const auto& path : search_paths) {
    std::cerr << "[LSP]   - " << path << std::endl;
  }
  return command;
}

Core::Core() : m_lsp_manager(std::make_unique<lsp::LSPManager>()) {}

Core::~Core() = default;

void Core::initialize() {
  // Setup default language servers
  // These paths would typically be configured via settings
}

void Core::initialize_lsp(const std::string& root_path) {
  std::cerr << "[LSP] Initializing LSP with root path: " << root_path << std::endl;

  // Register TypeScript/JavaScript language server
  lsp::LanguageServerConfig tsConfig;
  tsConfig.command = find_language_server("typescript-language-server");
  tsConfig.args = {"--stdio"};
  tsConfig.extensions = {".ts", ".tsx", ".js", ".jsx"};
  tsConfig.languageId = "typescript";
  m_lsp_manager->registerLanguageServer("typescript", tsConfig);

  // Register HTML language server
  lsp::LanguageServerConfig htmlConfig;
  htmlConfig.command = find_language_server("vscode-html-language-server");
  htmlConfig.args = {"--stdio"};
  htmlConfig.extensions = {".html", ".htm"};
  htmlConfig.languageId = "html";
  m_lsp_manager->registerLanguageServer("html", htmlConfig);

  // Register CSS language server
  lsp::LanguageServerConfig cssConfig;
  cssConfig.command = find_language_server("vscode-css-language-server");
  cssConfig.args = {"--stdio"};
  cssConfig.extensions = {".css", ".scss", ".less"};
  cssConfig.languageId = "css";
  m_lsp_manager->registerLanguageServer("css", cssConfig);

  // Initialize all servers
  std::cerr << "[LSP] Starting language servers..." << std::endl;
  m_lsp_manager->initializeServers("file://" + root_path);
}

TextBuffer &Core::buffer() {
  return m_buffer;
}

const TextBuffer &Core::buffer() const {
  return m_buffer;
}

UndoStack &Core::undo_stack() {
  return m_undo;
}

const UndoStack &Core::undo_stack() const {
  return m_undo;
}

void Core::insert(size_t offset, std::string_view text) {
  Edit edit = m_buffer.replace(offset, 0, text);
  m_undo.push(edit);
}

void Core::erase(size_t offset, size_t length) {
  Edit edit = m_buffer.replace(offset, length, "");
  m_undo.push(edit);
}

size_t Core::delete_backward(size_t offset) {
  if (offset == 0) {
    return 0;
  }
  Position pos = m_buffer.position_at(offset);
  if (pos.column == 0) {
    if (pos.line == 0) {
      return offset;
    }
    size_t prev_line = pos.line - 1;
    size_t prev_col = line_grapheme_count(prev_line);
    Position prev_pos{static_cast<uint32_t>(prev_line), static_cast<uint32_t>(prev_col)};
    size_t prev_offset = m_buffer.offset_at(prev_pos);
    erase(prev_offset, offset - prev_offset);
    return prev_offset;
  }

  Position prev_pos{pos.line, static_cast<uint32_t>(pos.column - 1)};
  size_t prev_offset = m_buffer.offset_at(prev_pos);
  erase(prev_offset, offset - prev_offset);
  return prev_offset;
}

void Core::set_text(std::string text) {
  m_buffer = TextBuffer(std::move(text));
}

size_t Core::line_count() const {
  return m_buffer.line_count();
}

std::string Core::line_text(size_t line_index) const {
  return m_buffer.line_text(line_index);
}

size_t Core::line_grapheme_count(size_t line_index) const {
  return m_buffer.line_grapheme_count(line_index);
}

Position Core::position_at(size_t offset) const {
  return m_buffer.position_at(offset);
}

size_t Core::offset_at(const Position &pos) const {
  return m_buffer.offset_at(pos);
}

lsp::LSPManager &Core::lsp_manager() {
  return *m_lsp_manager;
}

const lsp::LSPManager &Core::lsp_manager() const {
  return *m_lsp_manager;
}

TreeSitterHighlighter &Core::syntax_highlighter() {
  return m_syntax_highlighter;
}

const TreeSitterHighlighter &Core::syntax_highlighter() const {
  return m_syntax_highlighter;
}

void Core::open_file(const std::string& uri, const std::string& language_id) {
  std::string text = m_buffer.text();
  m_lsp_manager->didOpen(uri, language_id, text);
}

void Core::close_file(const std::string& uri) {
  m_lsp_manager->didClose(uri);
}

void Core::save_file(const std::string& uri) {
  m_lsp_manager->didSave(uri);
}

void Core::tick() {
  m_lsp_manager->processMessages();
}

} // namespace prodigeetor
