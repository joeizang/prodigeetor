#include "syntax_highlighter.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef PRODIGEETOR_USE_TREE_SITTER
#include <tree_sitter/api.h>

extern "C" const TSLanguage *tree_sitter_javascript();
extern "C" const TSLanguage *tree_sitter_typescript();
extern "C" const TSLanguage *tree_sitter_tsx();
extern "C" const TSLanguage *tree_sitter_swift();
extern "C" const TSLanguage *tree_sitter_c_sharp();
extern "C" const TSLanguage *tree_sitter_html();
extern "C" const TSLanguage *tree_sitter_css();
extern "C" const TSLanguage *tree_sitter_sql();
#endif

namespace prodigeetor {

static std::string g_resource_base_path;

static std::string read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::string();
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

static std::string resolve_query_path(const std::string &relative_path) {
  // Try multiple locations in order of preference
  std::vector<std::string> search_paths;

  // 1. User-configured base path
  if (!g_resource_base_path.empty()) {
    search_paths.push_back(g_resource_base_path + "/" + relative_path);
  }

  // 2. Relative to current directory
  search_paths.push_back(relative_path);

  // 3. Common development locations
  search_paths.push_back("/Users/josephizang/Projects/vibes/codeeditor/" + relative_path);

  for (const auto &path : search_paths) {
    if (std::filesystem::exists(path)) {
      std::cerr << "[Highlighter] Found query file: " << path << std::endl;
      return path;
    }
  }

  std::cerr << "[Highlighter] ERROR: Query file not found: " << relative_path << std::endl;
  return std::string();
}

static std::string query_for_language(TreeSitterHighlighter::LanguageId language) {
  std::vector<std::string> candidates;
  std::string base_query;

  switch (language) {
    case TreeSitterHighlighter::LanguageId::JavaScript:
      candidates = {
        "third_party/tree-sitter-javascript/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::TypeScript:
    case TreeSitterHighlighter::LanguageId::TSX:
      // TypeScript queries extend JavaScript, so we need to load both
      {
        std::string js_path = resolve_query_path("third_party/tree-sitter-javascript/queries/highlights.scm");
        if (!js_path.empty()) {
          base_query = read_file(js_path);
        }
        candidates = {
          "third_party/tree-sitter-typescript/queries/highlights.scm"
        };
      }
      break;
    case TreeSitterHighlighter::LanguageId::Swift:
      candidates = {
        "third_party/tree-sitter-swift/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::CSharp:
      candidates = {
        "third_party/tree-sitter-c-sharp/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::HTML:
      candidates = {
        "third_party/tree-sitter-html/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::CSS:
      candidates = {
        "third_party/tree-sitter-css/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::SQL:
      candidates = {
        "third_party/tree-sitter-sql/queries/highlights.scm"
      };
      break;
  }

  std::string extension_query;
  for (const auto &relative_path : candidates) {
    std::string resolved = resolve_query_path(relative_path);
    if (!resolved.empty()) {
      extension_query = read_file(resolved);
      break;
    }
  }

  // Merge base and extension queries
  if (!base_query.empty() && !extension_query.empty()) {
    return base_query + "\n\n" + extension_query;
  } else if (!extension_query.empty()) {
    return extension_query;
  } else if (!base_query.empty()) {
    return base_query;
  }

  return std::string();
}

#ifdef PRODIGEETOR_USE_TREE_SITTER
static const TSLanguage *language_for_id(TreeSitterHighlighter::LanguageId language) {
  switch (language) {
    case TreeSitterHighlighter::LanguageId::JavaScript:
      return tree_sitter_javascript();
    case TreeSitterHighlighter::LanguageId::TypeScript:
      return tree_sitter_typescript();
    case TreeSitterHighlighter::LanguageId::TSX:
      return tree_sitter_tsx();
    case TreeSitterHighlighter::LanguageId::Swift:
      return tree_sitter_swift();
    case TreeSitterHighlighter::LanguageId::CSharp:
      return tree_sitter_c_sharp();
    case TreeSitterHighlighter::LanguageId::HTML:
      return tree_sitter_html();
    case TreeSitterHighlighter::LanguageId::CSS:
      return tree_sitter_css();
    case TreeSitterHighlighter::LanguageId::SQL:
      return tree_sitter_sql();
  }
  return tree_sitter_javascript();
}
#endif

TreeSitterHighlighter::TreeSitterHighlighter() {
#ifdef PRODIGEETOR_USE_TREE_SITTER
  TSParser *parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_javascript());
  m_parser = parser;
  set_language(LanguageId::JavaScript);
#endif
}

TreeSitterHighlighter::~TreeSitterHighlighter() {
#ifdef PRODIGEETOR_USE_TREE_SITTER
  if (m_query) {
    ts_query_delete(static_cast<TSQuery *>(m_query));
    m_query = nullptr;
  }
  if (m_parser) {
    ts_parser_delete(static_cast<TSParser *>(m_parser));
    m_parser = nullptr;
  }
#endif
}

void TreeSitterHighlighter::set_language(LanguageId language) {
  m_language = language;
#ifdef PRODIGEETOR_USE_TREE_SITTER
  TSParser *parser = static_cast<TSParser *>(m_parser);
  if (!parser) {
    return;
  }
  ts_parser_set_language(parser, language_for_id(language));

  if (m_query) {
    ts_query_delete(static_cast<TSQuery *>(m_query));
    m_query = nullptr;
  }

  std::string query_str = query_for_language(language);
  if (query_str.empty()) {
    std::cerr << "[Highlighter] ERROR: Empty query string for language" << std::endl;
    return;
  }

  std::cerr << "[Highlighter] Loading query (" << query_str.size() << " bytes)" << std::endl;

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery *query = ts_query_new(language_for_id(language), query_str.c_str(),
                                static_cast<uint32_t>(query_str.size()),
                                &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    std::cerr << "[Highlighter] ERROR: Query parse failed at offset " << error_offset << ", error type: " << error_type << std::endl;
    // Show some context around the error
    if (error_offset < query_str.size()) {
      size_t context_start = (error_offset > 50) ? error_offset - 50 : 0;
      size_t context_end = std::min(static_cast<size_t>(error_offset) + 50, query_str.size());
      std::cerr << "[Highlighter] Context: " << query_str.substr(context_start, context_end - context_start) << std::endl;
    }
    ts_query_delete(query);
    query = nullptr;
  } else {
    std::cerr << "[Highlighter] Query loaded successfully, " << ts_query_capture_count(query) << " captures" << std::endl;
  }
  m_query = query;
#else
  (void)language;
#endif
}

void TreeSitterHighlighter::set_theme(SyntaxTheme theme) {
  m_theme = std::move(theme);
}

std::vector<RenderSpan> TreeSitterHighlighter::highlight(const std::string &text) {
  std::vector<RenderSpan> spans;

#ifdef PRODIGEETOR_USE_TREE_SITTER
  TSParser *parser = static_cast<TSParser *>(m_parser);
  TSQuery *query = static_cast<TSQuery *>(m_query);
  if (!parser) {
    std::cerr << "[Highlighter] ERROR: Parser is null, cannot highlight" << std::endl;
    return spans;
  }
  if (!query) {
    std::cerr << "[Highlighter] WARNING: Query is null, returning no spans (text will use default color)" << std::endl;
    return spans;
  }

  TSTree *tree = ts_parser_parse_string(parser, nullptr, text.c_str(), static_cast<uint32_t>(text.size()));
  if (!tree) {
    std::cerr << "[Highlighter] ERROR: Failed to parse text" << std::endl;
    return spans;
  }

  TSNode root = ts_tree_root_node(tree);

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root);

  TSQueryMatch match;
  uint32_t match_count = 0;
  while (ts_query_cursor_next_match(cursor, &match)) {
    match_count++;
    for (uint32_t i = 0; i < match.capture_count; ++i) {
      TSQueryCapture capture = match.captures[i];
      TSNode node = capture.node;
      uint32_t start = ts_node_start_byte(node);
      uint32_t end = ts_node_end_byte(node);

      uint32_t length = 0;
      const char *name = ts_query_capture_name_for_id(query, capture.index, &length);
      if (!name || length == 0) {
        continue;
      }
      std::string capture_name(name, length);

      RenderSpan span;
      span.range.start = Position{0, start};
      span.range.end = Position{0, end};
      span.style = m_theme.style_for_capture(capture_name);
      spans.push_back(span);
    }
  }

  std::cerr << "[Highlighter] Generated " << spans.size() << " spans from " << match_count << " matches" << std::endl;

  ts_query_cursor_delete(cursor);
  ts_tree_delete(tree);
#else
  (void)text;
#endif

  return spans;
}

} // namespace prodigeetor
