#import <CoreText/CoreText.h>

#include "CoreTextRenderer.h"

namespace prodigeetor {

static CFIndex utf16_index_for_utf8_offset(CFStringRef cf_text, size_t utf8_offset) {
  if (!cf_text) {
    return 0;
  }
  CFIndex length = CFStringGetLength(cf_text);
  if (length == 0 || utf8_offset == 0) {
    return 0;
  }

  CFIndex low = 0;
  CFIndex high = length;
  while (low < high) {
    CFIndex mid = low + (high - low) / 2;
    CFIndex bytes_used = 0;
    CFStringGetBytes(cf_text, CFRangeMake(0, mid), kCFStringEncodingUTF8, 0, false, nullptr, 0, &bytes_used);
    if (static_cast<size_t>(bytes_used) < utf8_offset) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return low;
}

CoreTextRenderer::~CoreTextRenderer() {
  if (m_font) {
    CFRelease(m_font);
    m_font = nullptr;
  }
}

void CoreTextRenderer::set_context(CGContextRef context) {
  m_context = context;
}

void CoreTextRenderer::set_font(const std::string &family, float size_points) {
  if (m_font) {
    CFRelease(m_font);
    m_font = nullptr;
  }
  CFStringRef cf_family = CFStringCreateWithCString(kCFAllocatorDefault, family.c_str(), kCFStringEncodingUTF8);
  CTFontRef font = CTFontCreateWithName(cf_family, size_points, nullptr);
  if (!font) {
    font = CTFontCreateWithName(CFSTR("Menlo"), size_points, nullptr);
  }
  m_font = font;
  CFRelease(cf_family);
}

void CoreTextRenderer::set_font_stack(const std::vector<std::string> &families, float size_points) {
  if (families.empty()) {
    set_font("Menlo", size_points);
    return;
  }
  for (const auto &family : families) {
    if (m_font) {
      CFRelease(m_font);
      m_font = nullptr;
    }
    CFStringRef cf_family = CFStringCreateWithCString(kCFAllocatorDefault, family.c_str(), kCFStringEncodingUTF8);
    CTFontRef font = CTFontCreateWithName(cf_family, size_points, nullptr);
    CFRelease(cf_family);
    if (font) {
      m_font = font;
      return;
    }
  }
  set_font("Menlo", size_points);
}

void CoreTextRenderer::set_ligatures(bool enabled) {
  m_ligatures = enabled;
}

LayoutMetrics CoreTextRenderer::measure_line(const std::string &text) {
  if (!m_font) {
    set_font("Menlo", 14.0f);
  }
  CFStringRef cf_text = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                reinterpret_cast<const UInt8 *>(text.data()),
                                                static_cast<CFIndex>(text.size()),
                                                kCFStringEncodingUTF8,
                                                false);
  const void *keys[] = {kCTFontAttributeName, kCTLigatureAttributeName};
  int ligature = m_ligatures ? 1 : 0;
  const void *values[] = {m_font, &ligature};
  CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault,
                                             keys,
                                             values,
                                             2,
                                             &kCFTypeDictionaryKeyCallBacks,
                                             &kCFTypeDictionaryValueCallBacks);
  CFAttributedStringRef attr = CFAttributedStringCreate(kCFAllocatorDefault, cf_text, attrs);
  CTLineRef line = CTLineCreateWithAttributedString(attr);
  CGFloat ascent = 0;
  CGFloat descent = 0;
  CGFloat leading = 0;
  double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);

  LayoutMetrics metrics;
  metrics.width = static_cast<float>(width);
  metrics.height = static_cast<float>(ascent + descent + leading);
  metrics.baseline = static_cast<float>(ascent);

  CFRelease(line);
  CFRelease(attr);
  CFRelease(attrs);
  CFRelease(cf_text);
  return metrics;
}

LineLayout CoreTextRenderer::layout_line(const std::string &text, const std::vector<RenderSpan> &spans) {
  LineLayout layout;
  layout.text = text;
  layout.spans = spans;
  layout.metrics = measure_line(text);
  return layout;
}

void CoreTextRenderer::draw_line(const LineLayout &layout, float x, float y) {
  if (!m_context) {
    return;
  }
  if (!m_font) {
    set_font("Menlo", 14.0f);
  }
  CFStringRef cf_text = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                reinterpret_cast<const UInt8 *>(layout.text.data()),
                                                static_cast<CFIndex>(layout.text.size()),
                                                kCFStringEncodingUTF8,
                                                false);
  CFMutableAttributedStringRef attr = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
  CFAttributedStringReplaceString(attr, CFRangeMake(0, 0), cf_text);

  const void *base_keys[] = {kCTFontAttributeName, kCTLigatureAttributeName};
  int base_ligature = m_ligatures ? 1 : 0;
  const void *base_values[] = {m_font, &base_ligature};
  CFDictionaryRef base_attrs = CFDictionaryCreate(kCFAllocatorDefault,
                                                  base_keys,
                                                  base_values,
                                                  2,
                                                  &kCFTypeDictionaryKeyCallBacks,
                                                  &kCFTypeDictionaryValueCallBacks);
  CFAttributedStringSetAttributes(attr, CFRangeMake(0, CFStringGetLength(cf_text)), base_attrs, false);

  for (const auto &span : layout.spans) {
    uint32_t color = span.style.fg_color;
    CGFloat a = ((color >> 24) & 0xFF) / 255.0f;
    CGFloat r = ((color >> 16) & 0xFF) / 255.0f;
    CGFloat g = ((color >> 8) & 0xFF) / 255.0f;
    CGFloat b = (color & 0xFF) / 255.0f;
    CGFloat components[4] = {r, g, b, a};
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGColorRef cg_color = CGColorCreate(space, components);

    CFIndex start = utf16_index_for_utf8_offset(cf_text, span.range.start.column);
    CFIndex end = utf16_index_for_utf8_offset(cf_text, span.range.end.column);
    CFIndex length = end - start;
    if (length > 0) {
      CFAttributedStringSetAttribute(attr, CFRangeMake(start, length), kCTForegroundColorAttributeName, cg_color);
    }
    CGColorRelease(cg_color);
    CGColorSpaceRelease(space);
  }

  CTLineRef line = CTLineCreateWithAttributedString(attr);

  CGContextSaveGState(m_context);
  CGContextSetTextPosition(m_context, x, y + layout.metrics.baseline);
  CTLineDraw(line, m_context);
  CGContextRestoreGState(m_context);

  CFRelease(line);
  CFRelease(attr);
  CFRelease(base_attrs);
  CFRelease(cf_text);
}

} // namespace prodigeetor
