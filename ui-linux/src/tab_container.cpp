#include "tab_container.h"
#include "editor_widget.h"
#include <string>
#include <memory>

// Tab data structure
struct TabData {
  std::string file_path;
  std::string display_name;
  bool is_dirty;
  GtkWidget *editor;
  GtkWidget *scroll_window;
  GtkWidget *label;
};

// Container state
struct TabContainerState {
  GtkWidget *notebook;
  std::vector<std::unique_ptr<TabData>> tabs;
  void (*title_callback)(const char *title, void *user_data);
  void *title_callback_data;
};

static void tab_container_state_free(void *data) {
  auto *state = static_cast<TabContainerState *>(data);
  delete state;
}

static void update_window_title(TabContainerState *state) {
  if (!state->title_callback) return;

  int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));
  if (current_page >= 0 && current_page < static_cast<int>(state->tabs.size())) {
    const auto &tab = state->tabs[current_page];
    std::string title = "Prodigeetor - ";
    if (tab->is_dirty) {
      title += "● ";
    }
    title += tab->display_name;
    state->title_callback(title.c_str(), state->title_callback_data);
  } else {
    state->title_callback("Prodigeetor", state->title_callback_data);
  }
}

static void on_tab_close_clicked(GtkButton *button, gpointer user_data) {
  auto *state = static_cast<TabContainerState *>(user_data);
  int page_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "page-num"));

  if (page_num >= 0 && page_num < static_cast<int>(state->tabs.size())) {
    // TODO: Check for unsaved changes and prompt
    gtk_notebook_remove_page(GTK_NOTEBOOK(state->notebook), page_num);
    state->tabs.erase(state->tabs.begin() + page_num);

    // Update page numbers for remaining close buttons
    for (int i = 0; i < static_cast<int>(state->tabs.size()); i++) {
      GtkWidget *tab_label = gtk_notebook_get_tab_label(
        GTK_NOTEBOOK(state->notebook),
        gtk_notebook_get_nth_page(GTK_NOTEBOOK(state->notebook), i)
      );
      if (tab_label) {
        // Find close button and update its page number
        GtkWidget *close_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tab_label), "close-button"));
        if (close_btn) {
          g_object_set_data(G_OBJECT(close_btn), "page-num", GINT_TO_POINTER(i));
        }
      }
    }

    // If no tabs left, create a new one
    if (state->tabs.empty()) {
      prodigeetor_tab_container_new_tab(state->notebook);
    } else {
      update_window_title(state);
    }
  }
}

static GtkWidget *create_tab_label(const char *text, int page_num, TabContainerState *state) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *label = gtk_label_new(text);
  GtkWidget *close_button = gtk_button_new_from_icon_name("window-close-symbolic");

  gtk_widget_set_tooltip_text(close_button, "Close");
  gtk_button_set_has_frame(GTK_BUTTON(close_button), FALSE);

  g_object_set_data(G_OBJECT(close_button), "page-num", GINT_TO_POINTER(page_num));
  g_signal_connect(close_button, "clicked", G_CALLBACK(on_tab_close_clicked), state);

  gtk_box_append(GTK_BOX(box), label);
  gtk_box_append(GTK_BOX(box), close_button);

  // Store references for later updates
  g_object_set_data(G_OBJECT(box), "label", label);
  g_object_set_data(G_OBJECT(box), "close-button", close_button);

  return box;
}

static void on_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
  auto *state = static_cast<TabContainerState *>(user_data);
  update_window_title(state);
}

GtkWidget *prodigeetor_tab_container_new(void) {
  auto *state = new TabContainerState();
  state->notebook = gtk_notebook_new();
  state->title_callback = nullptr;
  state->title_callback_data = nullptr;

  gtk_notebook_set_scrollable(GTK_NOTEBOOK(state->notebook), TRUE);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(state->notebook), FALSE);

  g_object_set_data_full(G_OBJECT(state->notebook), "tab-container-state",
                         state, tab_container_state_free);

  g_signal_connect(state->notebook, "switch-page", G_CALLBACK(on_page_switched), state);

  // Create initial tab
  prodigeetor_tab_container_new_tab(state->notebook);

  return state->notebook;
}

void prodigeetor_tab_container_new_tab(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  // Create tab data
  auto tab = std::make_unique<TabData>();
  tab->display_name = "Untitled";
  tab->is_dirty = false;

  // Create editor widget
  tab->editor = prodigeetor_editor_widget_new();

  // Create scroll window
  tab->scroll_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scroll_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tab->scroll_window), tab->editor);

  // Attach scroll adjustment to editor
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scroll_window));
  prodigeetor_editor_widget_attach_scroll(tab->editor, vadj, tab->scroll_window);

  // Set theme path
  prodigeetor_editor_widget_set_theme_path(tab->editor, "themes/default.json");

  // Create tab label
  int page_num = static_cast<int>(state->tabs.size());
  GtkWidget *label = create_tab_label(tab->display_name.c_str(), page_num, state);
  tab->label = label;

  // Add to notebook
  gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), tab->scroll_window, label);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(state->notebook), tab->scroll_window, TRUE);

  // Switch to new tab
  gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), page_num);

  state->tabs.push_back(std::move(tab));
  update_window_title(state);
}

void prodigeetor_tab_container_open_file(GtkWidget *container, const char *file_path) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state || !file_path) return;

  // Create new tab
  auto tab = std::make_unique<TabData>();
  tab->file_path = file_path;

  // Extract filename from path
  const char *filename = strrchr(file_path, '/');
  tab->display_name = filename ? (filename + 1) : file_path;
  tab->is_dirty = false;

  // Create editor widget
  tab->editor = prodigeetor_editor_widget_new();

  // Load file content
  GError *error = nullptr;
  char *contents = nullptr;
  gsize length = 0;

  if (g_file_get_contents(file_path, &contents, &length, &error)) {
    prodigeetor_editor_widget_set_text(tab->editor, contents);
    prodigeetor_editor_widget_set_file_path(tab->editor, file_path);
    g_free(contents);
  } else {
    g_warning("Failed to load file: %s", error ? error->message : "unknown error");
    if (error) g_error_free(error);
  }

  // Create scroll window
  tab->scroll_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab->scroll_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tab->scroll_window), tab->editor);

  // Attach scroll adjustment
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab->scroll_window));
  prodigeetor_editor_widget_attach_scroll(tab->editor, vadj, tab->scroll_window);

  // Set theme path
  prodigeetor_editor_widget_set_theme_path(tab->editor, "themes/default.json");

  // Create tab label
  int page_num = static_cast<int>(state->tabs.size());
  GtkWidget *label = create_tab_label(tab->display_name.c_str(), page_num, state);
  tab->label = label;

  // Add to notebook
  gtk_notebook_append_page(GTK_NOTEBOOK(state->notebook), tab->scroll_window, label);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(state->notebook), tab->scroll_window, TRUE);

  // Switch to new tab
  gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), page_num);

  state->tabs.push_back(std::move(tab));
  update_window_title(state);
}

void prodigeetor_tab_container_close_active_tab(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));
  if (current_page >= 0) {
    // Simulate close button click
    GtkWidget *tab_label = gtk_notebook_get_tab_label(
      GTK_NOTEBOOK(state->notebook),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(state->notebook), current_page)
    );
    if (tab_label) {
      GtkWidget *close_btn = GTK_WIDGET(g_object_get_data(G_OBJECT(tab_label), "close-button"));
      if (close_btn) {
        g_signal_emit_by_name(close_btn, "clicked");
      }
    }
  }
}

void prodigeetor_tab_container_save_active_file(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));
  if (current_page < 0 || current_page >= static_cast<int>(state->tabs.size())) return;

  auto &tab = state->tabs[current_page];

  if (tab->file_path.empty()) {
    // No file path, use save as
    prodigeetor_tab_container_save_active_file_as(container);
    return;
  }

  // Get text from editor
  char *text = prodigeetor_editor_widget_get_text(tab->editor);
  if (!text) return;

  // Save to file
  GError *error = nullptr;
  if (g_file_set_contents(tab->file_path.c_str(), text, -1, &error)) {
    tab->is_dirty = false;
    update_window_title(state);
  } else {
    g_warning("Failed to save file: %s", error ? error->message : "unknown error");
    if (error) g_error_free(error);
  }

  g_free(text);
}

void prodigeetor_tab_container_save_active_file_as(GtkWidget *container) {
  // TODO: Implement save as dialog
  // For now, just call save_active_file
  prodigeetor_tab_container_save_active_file(container);
}

void prodigeetor_tab_container_next_tab(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));
  int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(state->notebook));

  if (current < n_pages - 1) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), current + 1);
  }
}

void prodigeetor_tab_container_prev_tab(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));

  if (current > 0) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), current - 1);
  }
}

void prodigeetor_tab_container_select_tab(GtkWidget *container, int index) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(state->notebook));
  if (index >= 0 && index < n_pages) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(state->notebook), index);
  }
}

GtkWidget *prodigeetor_tab_container_get_active_editor(GtkWidget *container) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return nullptr;

  int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(state->notebook));
  if (current_page >= 0 && current_page < static_cast<int>(state->tabs.size())) {
    return state->tabs[current_page]->editor;
  }

  return nullptr;
}

void prodigeetor_tab_container_set_title_callback(GtkWidget *container,
                                                   void (*callback)(const char *title, void *user_data),
                                                   void *user_data) {
  auto *state = static_cast<TabContainerState *>(
    g_object_get_data(G_OBJECT(container), "tab-container-state"));

  if (!state) return;

  state->title_callback = callback;
  state->title_callback_data = user_data;
}
