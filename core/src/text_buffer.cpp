#include "text_buffer.h"

#include "grapheme.h"

#include <algorithm>
#include <stdexcept>

namespace prodigeetor {

TextBuffer::TextBuffer() = default;

TextBuffer::TextBuffer(std::string initial_text) : m_left(std::move(initial_text)) {}

size_t TextBuffer::size() const {
  return m_left.size() + m_right_reversed.size();
}

bool TextBuffer::empty() const {
  return size() == 0;
}

std::string TextBuffer::text() const {
  std::string out;
  out.reserve(size());
  out.append(m_left);
  for (auto it = m_right_reversed.rbegin(); it != m_right_reversed.rend(); ++it) {
    out.push_back(*it);
  }
  return out;
}

void TextBuffer::move_gap(size_t offset) {
  if (offset > size()) {
    throw std::out_of_range("TextBuffer::move_gap offset out of range");
  }

  while (m_left.size() > offset) {
    m_right_reversed.push_back(m_left.back());
    m_left.pop_back();
  }

  while (m_left.size() < offset) {
    m_left.push_back(m_right_reversed.back());
    m_right_reversed.pop_back();
  }
}

void TextBuffer::insert(size_t offset, std::string_view text) {
  move_gap(offset);
  m_left.append(text);
  m_line_index_dirty = true;
}

void TextBuffer::erase(size_t offset, size_t length) {
  if (length == 0) {
    return;
  }
  move_gap(offset);
  for (size_t i = 0; i < length; ++i) {
    if (m_right_reversed.empty()) {
      break;
    }
    m_right_reversed.pop_back();
  }
  m_line_index_dirty = true;
}

Edit TextBuffer::replace(size_t offset, size_t length, std::string_view text) {
  Edit edit;
  edit.offset = offset;
  edit.removed.reserve(length);

  move_gap(offset);
  for (size_t i = 0; i < length; ++i) {
    if (m_right_reversed.empty()) {
      break;
    }
    edit.removed.push_back(m_right_reversed.back());
    m_right_reversed.pop_back();
  }

  std::reverse(edit.removed.begin(), edit.removed.end());
  edit.inserted.assign(text);
  m_left.append(text);
  m_line_index_dirty = true;
  return edit;
}

char TextBuffer::char_at(size_t offset) const {
  if (offset < m_left.size()) {
    return m_left[offset];
  }
  size_t right_index = offset - m_left.size();
  if (right_index >= m_right_reversed.size()) {
    throw std::out_of_range("TextBuffer::char_at offset out of range");
  }
  return m_right_reversed[m_right_reversed.size() - right_index - 1];
}

size_t TextBuffer::line_count() const {
  ensure_line_index();
  return m_line_starts.size();
}

size_t TextBuffer::line_start(size_t line_index) const {
  ensure_line_index();
  if (line_index >= m_line_starts.size()) {
    return size();
  }
  return m_line_starts[line_index];
}

std::string TextBuffer::line_text(size_t line_index) const {
  size_t start = line_start(line_index);
  if (start >= size()) {
    return std::string();
  }
  size_t end = size();
  ensure_line_index();
  if (line_index + 1 < m_line_starts.size()) {
    end = m_line_starts[line_index + 1];
    if (end > 0 && char_at(end - 1) == '\n') {
      --end;
    }
  }
  return slice(start, end);
}

size_t TextBuffer::line_grapheme_count(size_t line_index) const {
  std::string line = line_text(line_index);
  return grapheme_count(line);
}

Position TextBuffer::position_at(size_t offset) const {
  if (offset > size()) {
    offset = size();
  }
  ensure_line_index();
  auto it = std::upper_bound(m_line_starts.begin(), m_line_starts.end(), offset);
  size_t line_index = it == m_line_starts.begin() ? 0 : static_cast<size_t>(it - m_line_starts.begin() - 1);
  size_t start = m_line_starts[line_index];
  Position pos;
  pos.line = static_cast<uint32_t>(line_index);
  std::string line_slice = slice(start, offset);
  pos.column = static_cast<uint32_t>(grapheme_count(line_slice));
  return pos;
}

size_t TextBuffer::offset_at(const Position &pos) const {
  ensure_line_index();
  if (pos.line >= m_line_starts.size()) {
    return size();
  }
  size_t start = m_line_starts[pos.line];
  size_t end = size();
  if (pos.line + 1 < m_line_starts.size()) {
    end = m_line_starts[pos.line + 1];
  }
  std::string line_slice = slice(start, end);
  size_t byte_offset = grapheme_byte_offset(line_slice, pos.column);
  return start + byte_offset;
}

std::string TextBuffer::slice(size_t start, size_t end) const {
  if (start >= end || start >= size()) {
    return std::string();
  }
  if (end > size()) {
    end = size();
  }
  std::string out;
  out.reserve(end - start);
  for (size_t i = start; i < end; ++i) {
    out.push_back(char_at(i));
  }
  return out;
}

void TextBuffer::ensure_line_index() const {
  if (!m_line_index_dirty) {
    return;
  }
  m_line_starts.clear();
  m_line_starts.push_back(0);
  for (size_t i = 0; i < size(); ++i) {
    if (char_at(i) == '\n') {
      if (i + 1 <= size()) {
        m_line_starts.push_back(i + 1);
      }
    }
  }
  if (m_line_starts.empty()) {
    m_line_starts.push_back(0);
  }
  m_line_index_dirty = false;
}

} // namespace prodigeetor
