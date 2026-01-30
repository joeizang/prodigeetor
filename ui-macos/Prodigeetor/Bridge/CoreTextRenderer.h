#pragma once

#include <CoreGraphics/CoreGraphics.h>

#include "rendering.h"

namespace prodigeetor {

class CoreTextRenderer final : public TextRendererAdapter {
public:
  void set_font(const std::string &family, float size_points) override;
  LayoutMetrics measure_line(const std::string &text) override;
  LineLayout layout_line(const std::string &text, const std::vector<RenderSpan> &spans) override;
  void draw_line(const LineLayout &layout, float x, float y) override;

  void set_context(CGContextRef context);

  ~CoreTextRenderer() override;

private:
  CTFontRef m_font = nullptr;
  CGContextRef m_context = nullptr;
};

} // namespace prodigeetor
