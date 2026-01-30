#include "syntax_highlighter.h"

#include <cstring>

#ifdef PRODIGEETOR_USE_TREE_SITTER
#include <tree_sitter/api.h>

extern "C" const TSLanguage *tree_sitter_javascript();
#endif

namespace prodigeetor {

static RenderStyle style_for_capture(const std::string &capture) {
  RenderStyle style;
  if (capture == "string") {
    style.fg_color = 0xFFB86CFF;
  } else if (capture == "comment") {
    style.fg_color = 0xFF6272A4;
  } else if (capture == "keyword") {
    style.fg_color = 0xFFFF79C6;
  } else if (capture == "number") {
    style.fg_color = 0xFFBD93F9;
  } else if (capture == "function") {
    style.fg_color = 0xFF50FA7B;
  } else if (capture == "property") {
    style.fg_color = 0xFF8BE9FD;
  } else if (capture == "constant") {
    style.fg_color = 0xFFF1FA8C;
  } else {
    style.fg_color = 0xFFFFFFFF;
  }
  return style;
}

TreeSitterHighlighter::TreeSitterHighlighter() {
#ifdef PRODIGEETOR_USE_TREE_SITTER
  TSParser *parser = ts_parser_new();
  ts_parser_set_language(parser, tree_sitter_javascript());

  const char *query_str =
      "(string) @string\n"
      "(template_string) @string\n"
      "(comment) @comment\n"
      "(number) @number\n"
      "((true) @constant)\n"
      "((false) @constant)\n"
      "((null) @constant)\n"
      "(identifier) @variable\n"
      "(property_identifier) @property\n"
      "(function_declaration name: (identifier) @function)\n"
      "(call_expression function: (identifier) @function)\n";

  uint32_t error_offset = 0;
  TSQueryError error_type = TSQueryErrorNone;
  TSQuery *query = ts_query_new(tree_sitter_javascript(), query_str,
                                static_cast<uint32_t>(std::strlen(query_str)),
                                &error_offset, &error_type);

  if (error_type != TSQueryErrorNone) {
    ts_query_delete(query);
    query = nullptr;
  }

  m_parser = parser;
  m_query = query;
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
      span.style = style_for_capture(capture_name);
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
