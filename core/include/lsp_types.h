#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "text_types.h"

namespace prodigeetor {
namespace lsp {

// LSP Position (0-based line and character)
struct LSPPosition {
  int line;
  int character;
};

// LSP Range
struct LSPRange {
  LSPPosition start;
  LSPPosition end;
};

// LSP Location
struct LSPLocation {
  std::string uri;
  LSPRange range;
};

// Diagnostic severity
enum class DiagnosticSeverity {
  Error = 1,
  Warning = 2,
  Information = 3,
  Hint = 4
};

// LSP Diagnostic
struct Diagnostic {
  LSPRange range;
  DiagnosticSeverity severity;
  std::string code;
  std::string source;
  std::string message;
};

// Completion item kind
enum class CompletionItemKind {
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18,
  Folder = 19,
  EnumMember = 20,
  Constant = 21,
  Struct = 22,
  Event = 23,
  Operator = 24,
  TypeParameter = 25
};

// Completion item
struct CompletionItem {
  std::string label;
  CompletionItemKind kind;
  std::string detail;
  std::string documentation;
  std::string sortText;
  std::string filterText;
  std::string insertText;
};

// Hover result
struct Hover {
  std::string contents;
  std::optional<LSPRange> range;
};

// Symbol kind
enum class SymbolKind {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
  TypeParameter = 26
};

// Document symbol
struct DocumentSymbol {
  std::string name;
  std::string detail;
  SymbolKind kind;
  LSPRange range;
  LSPRange selectionRange;
  std::vector<DocumentSymbol> children;
};

// Text document identifier
struct TextDocumentIdentifier {
  std::string uri;
};

// Versioned text document identifier
struct VersionedTextDocumentIdentifier {
  std::string uri;
  int version;
};

// Text document item
struct TextDocumentItem {
  std::string uri;
  std::string languageId;
  int version;
  std::string text;
};

// Text document content change event
struct TextDocumentContentChangeEvent {
  std::optional<LSPRange> range;
  std::string text;
};

// Server capabilities
struct ServerCapabilities {
  bool completionProvider = false;
  bool hoverProvider = false;
  bool definitionProvider = false;
  bool referencesProvider = false;
  bool documentSymbolProvider = false;
  bool workspaceSymbolProvider = false;
  bool documentFormattingProvider = false;
  bool documentRangeFormattingProvider = false;
  bool renameProvider = false;
  int textDocumentSync = 0; // 0=None, 1=Full, 2=Incremental
};

// Initialize result
struct InitializeResult {
  ServerCapabilities capabilities;
};

} // namespace lsp
} // namespace prodigeetor
