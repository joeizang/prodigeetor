#include "editor_widget.h"

#include <algorithm>
#include <memory>
#include <string_view>

#include "grapheme.h"
#include "core.h"
#include "pango_renderer.h"
#include "syntax_highlighter.h"
#include "text_buffer.h"

struct EditorState {
  prodigeetor::TextBuffer buffer;
  prodigeetor::PangoRenderer renderer;
  prodigeetor::TreeSitterHighlighter highlighter;
  size_t cursor_offset = 0;
  size_t selection_anchor = 0;
  float line_height = 18.0f;
  GtkWidget *widget = nullptr;
};

static void editor_state_destroy(gpointer data) {
  delete static_cast<EditorState *>(data);
}

static void editor_draw(GtkDrawingArea *area, cairo_t *cr, int, int, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return;
  }

  state->renderer.set_context(cr);
  state->renderer.set_font("Monospace", 14.0f);
  prodigeetor::LayoutMetrics metrics = state->renderer.measure_line("M");
  state->line_height = metrics.height > 0 ? metrics.height : state->line_height;

  size_t lines = state->buffer.line_count();
  float y = 8.0f;
  for (size_t i = 0; i < lines; ++i) {
    std::string line = state->buffer.line_text(i);
    std::vector<prodigeetor::RenderSpan> spans = state->highlighter.highlight(line);

    // Selection rendering
    size_t selection_start = std::min(state->cursor_offset, state->selection_anchor);
    size_t selection_end = std::max(state->cursor_offset, state->selection_anchor);
    prodigeetor::Position sel_start_pos = state->buffer.position_at(selection_start);
    prodigeetor::Position sel_end_pos = state->buffer.position_at(selection_end);
    if (selection_start != selection_end && i >= sel_start_pos.line && i <= sel_end_pos.line) {
      size_t line_columns = state->buffer.line_grapheme_count(i);
      size_t start_col = (i == sel_start_pos.line) ? sel_start_pos.column : 0;
      size_t end_col = (i == sel_end_pos.line) ? sel_end_pos.column : line_columns;

      std::string start_prefix = line.substr(0, prodigeetor::grapheme_byte_offset(line, start_col));
      std::string end_prefix = line.substr(0, prodigeetor::grapheme_byte_offset(line, end_col));
      float x_start = 8.0f + state->renderer.measure_line(start_prefix).width;
      float x_end = 8.0f + state->renderer.measure_line(end_prefix).width;
      if (x_end < x_start) {
        std::swap(x_end, x_start);
      }

      cairo_save(cr);
      cairo_set_source_rgba(cr, 0.2, 0.4, 0.8, 0.35);
      cairo_rectangle(cr, x_start, y, x_end - x_start, state->line_height);
      cairo_fill(cr);
      cairo_restore(cr);
    }

    prodigeetor::LineLayout layout = state->renderer.layout_line(line, spans);
    state->renderer.draw_line(layout, 8.0f, y);

    // Caret rendering
    prodigeetor::Position caret_pos = state->buffer.position_at(state->cursor_offset);
    if (caret_pos.line == i) {
      std::string caret_prefix = line.substr(0, prodigeetor::grapheme_byte_offset(line, caret_pos.column));
      float x = 8.0f + state->renderer.measure_line(caret_prefix).width;
      cairo_save(cr);
      cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
      cairo_rectangle(cr, x, y, 1.0, state->line_height);
      cairo_fill(cr);
      cairo_restore(cr);
    }

    y += layout.metrics.height > 0 ? layout.metrics.height : 18.0f;
  }
}

static gboolean editor_key_pressed(GtkEventControllerKey *, guint keyval, guint, GdkModifierType state_mask, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return FALSE;
  }
  bool extend = (state_mask & GDK_SHIFT_MASK) != 0;
  if (!extend) {
    state->selection_anchor = state->cursor_offset;
  }
  if (keyval == GDK_KEY_BackSpace) {
    prodigeetor::Core core;
    core.set_text(state->buffer.text());
    state->cursor_offset = core.delete_backward(state->cursor_offset);
    state->buffer = core.buffer();
    gtk_widget_queue_draw(state->widget);
    return TRUE;
  }
  if (keyval == GDK_KEY_Left || keyval == GDK_KEY_Right) {
    prodigeetor::Position pos = state->buffer.position_at(state->cursor_offset);
    if (keyval == GDK_KEY_Left) {
      if (pos.column > 0) {
        pos.column -= 1;
      } else if (pos.line > 0) {
        pos.line -= 1;
        pos.column = static_cast<uint32_t>(state->buffer.line_grapheme_count(pos.line));
      }
    } else {
      size_t line_cols = state->buffer.line_grapheme_count(pos.line);
      if (pos.column < line_cols) {
        pos.column += 1;
      } else if (pos.line + 1 < state->buffer.line_count()) {
        pos.line += 1;
        pos.column = 0;
      }
    }
    state->cursor_offset = state->buffer.offset_at(pos);
    gtk_widget_queue_draw(state->widget);
    return TRUE;
  }
  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    std::string insert = "\n";
    state->buffer.insert(state->cursor_offset, insert);
    state->cursor_offset += insert.size();
    gtk_widget_queue_draw(state->widget);
    return TRUE;
  }

  gunichar unicode = gdk_keyval_to_unicode(keyval);
  if (unicode != 0 && g_unichar_isprint(unicode)) {
    char utf8[8] = {0};
    int len = g_unichar_to_utf8(unicode, utf8);
    if (len > 0) {
      state->buffer.insert(state->cursor_offset, std::string_view(utf8, static_cast<size_t>(len)));
      state->cursor_offset += static_cast<size_t>(len);
      gtk_widget_queue_draw(state->widget);
      return TRUE;
    }
  }
  return FALSE;
}

static void editor_set_cursor_from_point(EditorState *state, double x, double y, bool extend) {
  if (!state) {
    return;
  }
  if (!extend) {
    state->selection_anchor = state->cursor_offset;
  }
  size_t line = static_cast<size_t>((y - 8.0) / state->line_height);
  if (line >= state->buffer.line_count()) {
    line = state->buffer.line_count() > 0 ? state->buffer.line_count() - 1 : 0;
  }
  std::string line_text = state->buffer.line_text(line);
  size_t column = 0;
  float target = static_cast<float>(x - 8.0);
  for (size_t i = 0; i <= line_text.size(); ++i) {
    std::string prefix = line_text.substr(0, i);
    float width = state->renderer.measure_line(prefix).width;
    if (width >= target) {
      column = prodigeetor::grapheme_count(prefix);
      break;
    }
    column = prodigeetor::grapheme_count(line_text);
  }
  prodigeetor::Position pos{static_cast<uint32_t>(line), static_cast<uint32_t>(column)};
  state->cursor_offset = state->buffer.offset_at(pos);
  gtk_widget_queue_draw(state->widget);
}

static void editor_click_pressed(GtkGestureClick *gesture, int, double x, double y, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return;
  }
  bool extend = false;
  if (gesture) {
    GdkModifierType mask = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
    extend = (mask & GDK_SHIFT_MASK) != 0;
  }
  editor_set_cursor_from_point(state, x, y, extend);
}

GtkWidget *prodigeetor_editor_widget_new(void) {
  GtkWidget *area = gtk_drawing_area_new();
  auto *state = new EditorState();
  state->widget = area;
  g_object_set_data_full(G_OBJECT(area), "editor-state", state, editor_state_destroy);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), editor_draw, state, nullptr);
  gtk_widget_set_focusable(area, TRUE);

  GtkEventController *key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(editor_key_pressed), state);
  gtk_widget_add_controller(area, key_controller);

  GtkGestureClick *click = gtk_gesture_click_new();
  g_signal_connect(click, "pressed", G_CALLBACK(editor_click_pressed), state);
  gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));

  GtkGestureDrag *drag = gtk_gesture_drag_new();
  g_signal_connect(drag, "drag-begin", G_CALLBACK(+[](GtkGestureDrag *gesture, double start_x, double start_y, gpointer data) {
    auto *state = static_cast<EditorState *>(data);
    if (!state) {
      return;
    }
    GdkModifierType state_mask = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
    bool extend = (state_mask & GDK_SHIFT_MASK) != 0;
    if (!extend) {
      state->selection_anchor = state->cursor_offset;
    }
    editor_set_cursor_from_point(state, start_x, start_y, extend);
  }), state);
  g_signal_connect(drag, "drag-update", G_CALLBACK(+[](GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer data) {
    auto *state = static_cast<EditorState *>(data);
    if (!state) {
      return;
    }
    double start_x = 0.0;
    double start_y = 0.0;
    gtk_gesture_drag_get_start_point(gesture, &start_x, &start_y);
    double x = start_x + offset_x;
    double y = start_y + offset_y;
    editor_set_cursor_from_point(state, x, y, true);
  }), state);
  gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(drag));

  return area;
}

void prodigeetor_editor_widget_set_text(GtkWidget *widget, const char *text) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state) {
    return;
  }
  state->buffer = prodigeetor::TextBuffer(text ? text : "");
  gtk_widget_queue_draw(widget);
}
