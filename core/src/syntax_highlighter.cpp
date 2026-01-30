#include "syntax_highlighter.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

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

static std::string read_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::string();
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

static std::string query_for_language(TreeSitterHighlighter::LanguageId language) {
  std::vector<std::string> candidates;
  switch (language) {
    case TreeSitterHighlighter::LanguageId::JavaScript:
      candidates = {
        "third_party/tree-sitter-javascript/queries/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::TypeScript:
      candidates = {
        "third_party/tree-sitter-typescript/queries/highlights.scm",
        "third_party/tree-sitter-typescript/queries/ts/highlights.scm"
      };
      break;
    case TreeSitterHighlighter::LanguageId::TSX:
      candidates = {
        "third_party/tree-sitter-typescript/queries/highlights.scm",
        "third_party/tree-sitter-typescript/queries/tsx/highlights.scm"
      };
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

  for (const auto &path : candidates) {
    if (std::filesystem::exists(path)) {
      return read_file(path);
    }
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
    return;
  }

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery *query = ts_query_new(language_for_id(language), query_str.c_str(),
                                static_cast<uint32_t>(query_str.size()),
                                &error_offset, &error_type);
  if (error_type != TSQueryErrorNone) {
    ts_query_delete(query);
    query = nullptr;
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
  if (!parser || !query) {
    return spans;
  }

  TSTree *tree = ts_parser_parse_string(parser, nullptr, text.c_str(), static_cast<uint32_t>(text.size()));
  TSNode root = ts_tree_root_node(tree);

  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root);

  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
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

  ts_query_cursor_delete(cursor);
  ts_tree_delete(tree);
#else
  (void)text;
#endif

  return spans;
}

} // namespace prodigeetor
