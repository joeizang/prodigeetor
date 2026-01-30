#pragma once

#include <cstddef>
#include <string_view>

namespace prodigeetor {

size_t grapheme_count(std::string_view text);
size_t grapheme_byte_offset(std::string_view text, size_t grapheme_index);

} // namespace prodigeetor
