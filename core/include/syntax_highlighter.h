#pragma once

#include <string>
#include <vector>

#include "rendering.h"
#include "theme.h"

namespace prodigeetor {

class SyntaxHighlighter {
public:
  virtual ~SyntaxHighlighter() = default;
  virtual std::vector<RenderSpan> highlight(const std::string &text) = 0;
};

class TreeSitterHighlighter final : public SyntaxHighlighter {
public:
  enum class LanguageId {
    JavaScript,
    TypeScript,
    TSX,
    Swift,
    CSharp,
    HTML,
    CSS,
    SQL
  };

  TreeSitterHighlighter();
  ~TreeSitterHighlighter() override;
  void set_language(LanguageId language);
  void set_theme(SyntaxTheme theme);
  std::vector<RenderSpan> highlight(const std::string &text) override;

private:
  LanguageId m_language = LanguageId::JavaScript;
  SyntaxTheme m_theme;
  void *m_parser = nullptr;
  void *m_query = nullptr;
};

} // namespace prodigeetor
