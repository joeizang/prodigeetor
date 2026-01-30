#include <pango/pangocairo.h>

#include "pango_renderer.h"

namespace prodigeetor {

PangoRenderer::~PangoRenderer() {
  if (m_font_desc) {
    pango_font_description_free(m_font_desc);
    m_font_desc = nullptr;
  }
}

void PangoRenderer::set_context(cairo_t *context) {
  m_context = context;
}

void PangoRenderer::set_font(const std::string &family, float size_points) {
  m_family = family;
  m_size_points = size_points;
  if (m_font_desc) {
    pango_font_description_free(m_font_desc);
  }
  m_font_desc = pango_font_description_new();
  pango_font_description_set_family(m_font_desc, m_family.c_str());
  pango_font_description_set_absolute_size(m_font_desc, m_size_points * PANGO_SCALE);
}

void PangoRenderer::set_ligatures(bool enabled) {
  m_ligatures = enabled;
}

LayoutMetrics PangoRenderer::measure_line(const std::string &text) {
  LayoutMetrics metrics;
  if (!m_context) {
    return metrics;
  }
  if (!m_font_desc) {
    set_font(m_family, m_size_points);
  }
  PangoLayout *layout = pango_cairo_create_layout(m_context);
  pango_layout_set_font_description(layout, m_font_desc);
  pango_layout_set_text(layout, text.c_str(), static_cast<int>(text.size()));

  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
  pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);

  metrics.width = static_cast<float>(logical_rect.width);
  metrics.height = static_cast<float>(logical_rect.height);
  metrics.baseline = static_cast<float>(pango_layout_get_baseline(layout)) / PANGO_SCALE;

  g_object_unref(layout);
  return metrics;
}

LineLayout PangoRenderer::layout_line(const std::string &text, const std::vector<RenderSpan> &spans) {
  LineLayout layout;
  layout.text = text;
  layout.spans = spans;
  layout.metrics = measure_line(text);
  return layout;
}

void PangoRenderer::draw_line(const LineLayout &layout, float x, float y) {
  if (!m_context) {
    return;
  }
  if (!m_font_desc) {
    set_font(m_family, m_size_points);
  }
  PangoLayout *pango_layout = pango_cairo_create_layout(m_context);
  pango_layout_set_font_description(pango_layout, m_font_desc);
  pango_layout_set_text(pango_layout, layout.text.c_str(), static_cast<int>(layout.text.size()));

  if (!layout.spans.empty()) {
    PangoAttrList *attrs = pango_attr_list_new();
    if (m_ligatures) {
      PangoAttribute *liga_attr = pango_attr_font_features_new("liga=1");
      pango_attr_list_insert(attrs, liga_attr);
    }
    for (const auto &span : layout.spans) {
      uint32_t color = span.style.fg_color;
      guint16 r = static_cast<guint16>(((color >> 16) & 0xFF) * 257);
      guint16 g = static_cast<guint16>(((color >> 8) & 0xFF) * 257);
      guint16 b = static_cast<guint16>((color & 0xFF) * 257);
      auto *attr = pango_attr_foreground_new(r, g, b);
      attr->start_index = static_cast<guint>(span.range.start.column);
      attr->end_index = static_cast<guint>(span.range.end.column);
      pango_attr_list_insert(attrs, attr);
    }
    pango_layout_set_attributes(pango_layout, attrs);
    pango_attr_list_unref(attrs);
  } else {
    if (m_ligatures) {
      PangoAttrList *attrs = pango_attr_list_new();
      PangoAttribute *liga_attr = pango_attr_font_features_new("liga=1");
      pango_attr_list_insert(attrs, liga_attr);
      pango_layout_set_attributes(pango_layout, attrs);
      pango_attr_list_unref(attrs);
    }
  }

  cairo_save(m_context);
  cairo_move_to(m_context, x, y);
  pango_cairo_show_layout(m_context, pango_layout);
  cairo_restore(m_context);

  g_object_unref(pango_layout);
}

} // namespace prodigeetor
