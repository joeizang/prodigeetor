#include "editor_widget.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string_view>

#include <gio/gio.h>

#include "grapheme.h"
#include "core.h"
#include "pango_renderer.h"
#include "syntax_highlighter.h"
#include "text_buffer.h"
#include "theme.h"
#include "settings.h"
#include "lsp_manager.h"
#include "lsp_types.h"

struct EditorState {
  prodigeetor::TextBuffer buffer;
  std::unique_ptr<prodigeetor::Core> core;  // For LSP support
  prodigeetor::PangoRenderer renderer;
  prodigeetor::TreeSitterHighlighter highlighter;
  size_t cursor_offset = 0;
  size_t selection_anchor = 0;
  float line_height = 18.0f;
  float scroll_offset_y = 0.0f;
  float view_height = 0.0f;
  GtkWidget *widget = nullptr;
  GtkAdjustment *v_adjustment = nullptr;
  GtkWidget *viewport = nullptr;
  std::string theme_path = "themes/default.json";
  std::string file_path;
  bool lsp_initialized = false;
  GFileMonitor *theme_monitor = nullptr;
  prodigeetor::EditorSettings settings;
  std::string font_stack;
};

static void editor_state_destroy(gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (state && state->theme_monitor) {
    g_object_unref(state->theme_monitor);
    state->theme_monitor = nullptr;
  }
  delete static_cast<EditorState *>(data);
}

static void notify_lsp_text_changed(EditorState *state) {
  if (!state || !state->lsp_initialized || !state->core || state->file_path.empty()) {
    return;
  }
  std::string uri = "file://" + state->file_path;
  std::string text = state->buffer.text();
  state->core->lsp_manager().didChange(uri, text);
}

static void request_completion(EditorState *state) {
  if (!state || !state->lsp_initialized || !state->core || state->file_path.empty()) {
    std::cerr << "[Editor] Cannot request completion - LSP not initialized" << std::endl;
    return;
  }

  std::string uri = "file://" + state->file_path;
  prodigeetor::Position pos = state->buffer.position_at(state->cursor_offset);

  std::cerr << "[Editor] Requesting completion at line " << pos.line << ", column " << pos.column << std::endl;

  state->core->lsp_manager().completion(
    uri,
    static_cast<int>(pos.line),
    static_cast<int>(pos.column),
    [](const std::vector<prodigeetor::lsp::CompletionItem>& items) {
      std::cerr << "[Editor] Received " << items.size() << " completion items:" << std::endl;
      for (size_t i = 0; i < std::min(items.size(), size_t(10)); ++i) {
        std::cerr << "  - " << items[i].label;
        if (!items[i].detail.empty()) {
          std::cerr << " (" << items[i].detail << ")";
        }
        std::cerr << std::endl;
      }
      if (items.size() > 10) {
        std::cerr << "  ... and " << (items.size() - 10) << " more" << std::endl;
      }
    }
  );
}

static void editor_draw(GtkDrawingArea *area, cairo_t *cr, int, int, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return;
  }

  state->renderer.set_context(cr);
  if (!state->font_stack.empty()) {
    state->renderer.set_font(state->font_stack, state->settings.font_size);
    state->renderer.set_ligatures(state->settings.font_ligatures);
  } else {
    state->renderer.set_font("Monoid", 14.0f);
  }
  prodigeetor::LayoutMetrics metrics = state->renderer.measure_line("M");
  state->line_height = metrics.height > 0 ? metrics.height : state->line_height;
  if (state->viewport) {
    state->view_height = static_cast<float>(gtk_widget_get_height(state->viewport));
  } else {
    state->view_height = static_cast<float>(gtk_widget_get_height(state->widget));
  }
  if (state->v_adjustment) {
    state->scroll_offset_y = static_cast<float>(gtk_adjustment_get_value(state->v_adjustment));
  }

  size_t lines = state->buffer.line_count();
  float content_height = static_cast<float>(lines) * state->line_height + 16.0f;
  gtk_widget_set_size_request(state->widget, -1, static_cast<int>(content_height));
  size_t start_line = static_cast<size_t>(state->scroll_offset_y / state->line_height);
  float offset = state->scroll_offset_y - (start_line * state->line_height);
  float y = 8.0f - offset;
  for (size_t i = start_line; i < lines && y < state->view_height; ++i) {
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

    y += state->line_height;
  }
}

static gboolean editor_key_pressed(GtkEventControllerKey *, guint keyval, guint, GdkModifierType state_mask, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return FALSE;
  }

  // Handle Ctrl+Space for completion
  bool ctrl = (state_mask & GDK_CONTROL_MASK) != 0;
  if (ctrl && keyval == GDK_KEY_space) {
    request_completion(state);
    return TRUE;
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
    notify_lsp_text_changed(state);
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
    notify_lsp_text_changed(state);
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
      notify_lsp_text_changed(state);
      gtk_widget_queue_draw(state->widget);
      return TRUE;
    }
  }
  return FALSE;
}

static float max_scroll_offset(const EditorState *state) {
  if (!state) {
    return 0.0f;
  }
  float content_height = static_cast<float>(state->buffer.line_count()) * state->line_height + 16.0f;
  if (content_height <= state->view_height) {
    return 0.0f;
  }
  return content_height - state->view_height;
}

static void editor_reload_theme(EditorState *state) {
  if (!state) {
    return;
  }
  prodigeetor::SyntaxTheme theme = prodigeetor::SyntaxTheme::load_from_file(state->theme_path);
  state->highlighter.set_theme(std::move(theme));
}

static void editor_theme_changed(GFileMonitor *, GFile *, GFile *, GFileMonitorEvent event_type, gpointer data) {
  auto *state = static_cast<EditorState *>(data);
  if (!state) {
    return;
  }
  if (event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT || event_type == G_FILE_MONITOR_EVENT_CREATED) {
    editor_reload_theme(state);
    gtk_widget_queue_draw(state->widget);
  }
}

static prodigeetor::TreeSitterHighlighter::LanguageId language_for_path(const std::string &path) {
  std::string lower = path;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (lower.size() >= 4 && lower.ends_with(".tsx")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::TSX;
  }
  if (lower.size() >= 3 && lower.ends_with(".ts")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::TypeScript;
  }
  if (lower.size() >= 3 && (lower.ends_with(".js") || lower.ends_with(".jsx"))) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript;
  }
  if (lower.size() >= 6 && lower.ends_with(".swift")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::Swift;
  }
  if (lower.size() >= 3 && lower.ends_with(".cs")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::CSharp;
  }
  if (lower.size() >= 5 && (lower.ends_with(".html") || lower.ends_with(".htm"))) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::HTML;
  }
  if (lower.size() >= 4 && lower.ends_with(".css")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::CSS;
  }
  if (lower.size() >= 4 && lower.ends_with(".sql")) {
    return prodigeetor::TreeSitterHighlighter::LanguageId::SQL;
  }
  return prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript;
}

static std::string detect_language_id(const std::string &path) {
  std::string lower = path;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (lower.ends_with(".ts")) return "typescript";
  if (lower.ends_with(".tsx")) return "typescriptreact";
  if (lower.ends_with(".js")) return "javascript";
  if (lower.ends_with(".jsx")) return "javascriptreact";
  if (lower.ends_with(".html") || lower.ends_with(".htm")) return "html";
  if (lower.ends_with(".css")) return "css";
  if (lower.ends_with(".scss")) return "scss";
  if (lower.ends_with(".less")) return "less";
  if (lower.ends_with(".swift")) return "swift";
  if (lower.ends_with(".cs")) return "csharp";
  if (lower.ends_with(".sql")) return "sql";
  return "plaintext";
}

static void editor_set_cursor_from_point(EditorState *state, double x, double y, bool extend) {
  if (!state) {
    return;
  }
  if (!extend) {
    state->selection_anchor = state->cursor_offset;
  }
  double content_y = y + state->scroll_offset_y;
  size_t line = static_cast<size_t>((content_y - 8.0) / state->line_height);
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
  state->core = std::make_unique<prodigeetor::Core>();
  state->core->initialize();
  state->settings = prodigeetor::SettingsLoader::load_from_file("settings/default.json");
  state->font_stack = state->settings.font_family;
  for (const auto &fallback : state->settings.font_fallbacks) {
    state->font_stack.append(", ");
    state->font_stack.append(fallback);
  }
  editor_reload_theme(state);
  state->highlighter.set_language(prodigeetor::TreeSitterHighlighter::LanguageId::JavaScript);
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
    if (y < 0.0) {
      state->scroll_offset_y = std::max(0.0f, state->scroll_offset_y - state->line_height);
      y = 0.0;
    } else if (y > state->view_height) {
      state->scroll_offset_y = std::min(max_scroll_offset(state), state->scroll_offset_y + state->line_height);
      y = state->view_height;
    }
    if (state->v_adjustment) {
      gtk_adjustment_set_value(state->v_adjustment, state->scroll_offset_y);
    }
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

char *prodigeetor_editor_widget_get_text(GtkWidget *widget) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state) {
    return g_strdup("");
  }
  std::string text = state->buffer.text();
  return g_strdup(text.c_str());
}

void prodigeetor_editor_widget_set_file_path(GtkWidget *widget, const char *path) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state || !path) {
    return;
  }
  state->file_path = path;
  state->highlighter.set_language(language_for_path(path));

  // Initialize LSP if not already initialized
  if (!state->lsp_initialized && state->core) {
    // Extract directory from file path
    std::string workspace_path = path;
    size_t last_slash = workspace_path.find_last_of('/');
    if (last_slash != std::string::npos) {
      workspace_path = workspace_path.substr(0, last_slash);
    }
    state->core->initialize_lsp(workspace_path);
    state->lsp_initialized = true;

    // Notify LSP about opened file
    std::string uri = "file://" + std::string(path);
    std::string language_id = detect_language_id(path);
    std::string text = state->buffer.text();
    state->core->open_file(uri, language_id);
  }

  gtk_widget_queue_draw(widget);
}

void prodigeetor_editor_widget_set_theme_path(GtkWidget *widget, const char *path) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state || !path) {
    return;
  }
  state->theme_path = path;
  editor_reload_theme(state);
  if (state->theme_monitor) {
    g_object_unref(state->theme_monitor);
    state->theme_monitor = nullptr;
  }
  GFile *file = g_file_new_for_path(path);
  state->theme_monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  if (state->theme_monitor) {
    g_signal_connect(state->theme_monitor, "changed", G_CALLBACK(editor_theme_changed), state);
  }
  g_object_unref(file);
  gtk_widget_queue_draw(widget);
}

void prodigeetor_editor_widget_attach_scroll(GtkWidget *widget, GtkAdjustment *vadj, GtkWidget *viewport) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state) {
    return;
  }
  state->v_adjustment = vadj;
  state->viewport = viewport;
}

void prodigeetor_editor_widget_tick(GtkWidget *widget) {
  auto *state = static_cast<EditorState *>(g_object_get_data(G_OBJECT(widget), "editor-state"));
  if (!state || !state->core) {
    return;
  }
  state->core->tick();
}
