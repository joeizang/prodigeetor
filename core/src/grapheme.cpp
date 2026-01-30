#include "grapheme.h"

#include <vector>

#ifdef PRODIGEETOR_USE_UTF8PROC
#include <utf8proc.h>
#endif

namespace prodigeetor {

static bool is_combining_mark(uint32_t codepoint) {
  return (codepoint >= 0x0300 && codepoint <= 0x036F) ||
         (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||
         (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) ||
         (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||
         (codepoint >= 0xFE20 && codepoint <= 0xFE2F);
}

#ifdef PRODIGEETOR_USE_UTF8PROC
static std::vector<size_t> grapheme_boundaries_utf8proc(std::string_view text) {
  std::vector<size_t> boundaries;
  if (text.empty()) {
    return boundaries;
  }

  size_t i = 0;
  utf8proc_int32_t prev = 0;
  int state = 0;
  bool have_prev = false;

  while (i < text.size()) {
    utf8proc_int32_t codepoint = 0;
    int len = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t *>(text.data() + i),
                               static_cast<ssize_t>(text.size() - i), &codepoint);
    if (len <= 0) {
      ++i;
      continue;
    }

    if (!have_prev) {
      boundaries.push_back(0);
      have_prev = true;
    } else {
      if (utf8proc_grapheme_break_stateful(prev, codepoint, &state)) {
        boundaries.push_back(i);
      }
    }

    prev = codepoint;
    i += static_cast<size_t>(len);
  }

  return boundaries;
}
#endif

static std::vector<size_t> grapheme_boundaries_fallback(std::string_view text) {
  std::vector<size_t> boundaries;
  if (text.empty()) {
    return boundaries;
  }

  size_t i = 0;
  bool in_grapheme = false;
  while (i < text.size()) {
    uint32_t codepoint = 0;
    unsigned char c = static_cast<unsigned char>(text[i]);
    size_t advance = 1;
    if (c < 0x80) {
      codepoint = c;
    } else if ((c >> 5) == 0x6 && i + 1 < text.size()) {
      codepoint = ((c & 0x1F) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3F);
      advance = 2;
    } else if ((c >> 4) == 0xE && i + 2 < text.size()) {
      codepoint = ((c & 0x0F) << 12) |
                  ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6) |
                  (static_cast<unsigned char>(text[i + 2]) & 0x3F);
      advance = 3;
    } else if ((c >> 3) == 0x1E && i + 3 < text.size()) {
      codepoint = ((c & 0x07) << 18) |
                  ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12) |
                  ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6) |
                  (static_cast<unsigned char>(text[i + 3]) & 0x3F);
      advance = 4;
    } else {
      codepoint = c;
    }

    if (!is_combining_mark(codepoint) || !in_grapheme) {
      boundaries.push_back(i);
      in_grapheme = true;
    }

    i += advance;
  }

  return boundaries;
}

static std::vector<size_t> grapheme_boundaries(std::string_view text) {
#ifdef PRODIGEETOR_USE_UTF8PROC
  return grapheme_boundaries_utf8proc(text);
#else
  return grapheme_boundaries_fallback(text);
#endif
}

size_t grapheme_count(std::string_view text) {
  return grapheme_boundaries(text).size();
}

size_t grapheme_byte_offset(std::string_view text, size_t grapheme_index) {
  auto boundaries = grapheme_boundaries(text);
  if (boundaries.empty()) {
    return 0;
  }
  if (grapheme_index >= boundaries.size()) {
    return text.size();
  }
  return boundaries[grapheme_index];
}

} // namespace prodigeetor
