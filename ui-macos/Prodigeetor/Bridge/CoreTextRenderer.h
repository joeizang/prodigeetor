#pragma once

#include <CoreGraphics/CoreGraphics.h>

#include "rendering.h"

namespace prodigeetor {

class CoreTextRenderer final : public TextRendererAdapter {
public:
  void set_font(const std::string &family, float size_points) override;
  void set_font_stack(const std::vector<std::string> &families, float size_points);
  void set_ligatures(bool enabled);
  LayoutMetrics measure_line(const std::string &text) override;
  LineLayout layout_line(const std::string &text, const std::vector<RenderSpan> &spans) override;
  void draw_line(const LineLayout &layout, float x, float y) override;

  void set_context(CGContextRef context);

  ~CoreTextRenderer() override;

private:
  CTFontRef m_font = nullptr;
  CGContextRef m_context = nullptr;
  bool m_ligatures = true;
};

} // namespace prodigeetor
