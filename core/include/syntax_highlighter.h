#pragma once

#include <string>
#include <vector>

#include "rendering.h"

namespace prodigeetor {

class SyntaxHighlighter {
public:
  virtual ~SyntaxHighlighter() = default;
  virtual std::vector<RenderSpan> highlight(const std::string &text) = 0;
};

class TreeSitterHighlighter final : public SyntaxHighlighter {
public:
  TreeSitterHighlighter();
  ~TreeSitterHighlighter() override;
  std::vector<RenderSpan> highlight(const std::string &text) override;

private:
  void *m_parser = nullptr;
  void *m_query = nullptr;
};

} // namespace prodigeetor
