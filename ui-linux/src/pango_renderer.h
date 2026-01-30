#pragma once

#include "rendering.h"

namespace prodigeetor {

class PangoRenderer final : public TextRendererAdapter {
public:
  void set_font(const std::string &family, float size_points) override;
  LayoutMetrics measure_line(const std::string &text) override;
  LineLayout layout_line(const std::string &text, const std::vector<RenderSpan> &spans) override;
  void draw_line(const LineLayout &layout, float x, float y) override;

  void set_context(cairo_t *context);
  ~PangoRenderer() override;

private:
  cairo_t *m_context = nullptr;
  PangoFontDescription *m_font_desc = nullptr;
  std::string m_family = "Monospace";
  float m_size_points = 14.0f;
};

} // namespace prodigeetor
