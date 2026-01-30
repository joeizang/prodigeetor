#pragma once

#include <string>
#include <vector>

#include "text_types.h"

namespace prodigeetor {

struct RenderLine {
  size_t line_index = 0;
  std::string text;
};

struct LayoutMetrics {
  float width = 0.0f;
  float height = 0.0f;
  float baseline = 0.0f;
};

struct RenderStyle {
  uint32_t fg_color = 0xFFFFFFFF;
  uint32_t bg_color = 0x00000000;
  bool bold = false;
  bool italic = false;
};

struct RenderSpan {
  Range range;
  RenderStyle style;
};
// RenderSpan range columns are UTF-8 byte offsets within the line.

struct Glyph {
  uint32_t codepoint = 0;
  float x = 0.0f;
  float advance = 0.0f;
};

struct GlyphRun {
  RenderStyle style;
  std::vector<Glyph> glyphs;
};

struct LineLayout {
  LayoutMetrics metrics;
  std::vector<GlyphRun> runs;
  std::string text;
  std::vector<RenderSpan> spans;
};

class TextRendererAdapter {
public:
  virtual ~TextRendererAdapter() = default;

  virtual void set_font(const std::string &family, float size_points) = 0;
  virtual LayoutMetrics measure_line(const std::string &text) = 0;
  virtual LineLayout layout_line(const std::string &text, const std::vector<RenderSpan> &spans) = 0;

  virtual void draw_line(const LineLayout &layout, float x, float y) = 0;
};

class TextLayoutEngine {
public:
  virtual ~TextLayoutEngine() = default;
  virtual std::vector<RenderSpan> style_spans(const std::string &text) = 0;
};

} // namespace prodigeetor
