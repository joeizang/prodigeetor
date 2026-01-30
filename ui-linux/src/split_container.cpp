#include "split_container.h"
#include "tab_container.h"
#include <vector>
#include <memory>

// Split container state
struct SplitContainerState {
  GtkWidget *root_widget;  // Either a GtkBox or GtkPaned
  GtkWidget *window;       // Reference to main window
  std::vector<GtkWidget *> tab_containers;
  int active_split_index;
};

static void split_container_state_free(void *data) {
  auto *state = static_cast<SplitContainerState *>(data);
  delete state;
}

static void update_window_title(const char *title, void *user_data) {
  auto *state = static_cast<SplitContainerState *>(user_data);
  if (state->window) {
    gtk_window_set_title(GTK_WINDOW(state->window), title);
  }
}

static GtkWidget *get_active_tab_container(SplitContainerState *state) {
  if (state->active_split_index >= 0 &&
      state->active_split_index < static_cast<int>(state->tab_containers.size())) {
    return state->tab_containers[state->active_split_index];
  }
  return nullptr;
}

GtkWidget *prodigeetor_split_container_new(void) {
  auto *state = new SplitContainerState();

  // Create initial tab container
  GtkWidget *tab_container = prodigeetor_tab_container_new();
  prodigeetor_tab_container_set_title_callback(tab_container, update_window_title, state);

  state->tab_containers.push_back(tab_container);
  state->active_split_index = 0;
  state->window = nullptr;

  // Start with a simple box containing one tab container
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(box), tab_container);
  gtk_widget_set_vexpand(tab_container, TRUE);
  gtk_widget_set_hexpand(tab_container, TRUE);

  state->root_widget = box;

  g_object_set_data_full(G_OBJECT(box), "split-container-state",
                         state, split_container_state_free);

  return box;
}

void prodigeetor_split_container_split_vertical(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  // Create new tab container
  GtkWidget *new_tab_container = prodigeetor_tab_container_new();
  prodigeetor_tab_container_set_title_callback(new_tab_container, update_window_title, state);

  if (state->tab_containers.size() == 1) {
    // First split: replace box with paned
    GtkWidget *old_container = state->tab_containers[0];
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    // Remove old container from box
    gtk_box_remove(GTK_BOX(state->root_widget), old_container);

    // Add both containers to paned
    gtk_paned_set_start_child(GTK_PANED(paned), old_container);
    gtk_paned_set_end_child(GTK_PANED(paned), new_tab_container);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

    // Add paned to box
    gtk_box_append(GTK_BOX(state->root_widget), paned);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_widget_set_hexpand(paned, TRUE);
  } else {
    // Subsequent splits: find the current paned and add to it
    // For simplicity, we'll just append to the rightmost position
    GtkWidget *current = state->root_widget;
    GtkWidget *paned = gtk_widget_get_first_child(current);

    if (GTK_IS_PANED(paned)) {
      // Create a new paned to replace the end child
      GtkWidget *end_child = gtk_paned_get_end_child(GTK_PANED(paned));
      GtkWidget *new_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

      g_object_ref(end_child);
      gtk_paned_set_end_child(GTK_PANED(paned), nullptr);

      gtk_paned_set_start_child(GTK_PANED(new_paned), end_child);
      g_object_unref(end_child);

      gtk_paned_set_end_child(GTK_PANED(new_paned), new_tab_container);
      gtk_paned_set_resize_start_child(GTK_PANED(new_paned), TRUE);
      gtk_paned_set_resize_end_child(GTK_PANED(new_paned), TRUE);
      gtk_paned_set_shrink_start_child(GTK_PANED(new_paned), FALSE);
      gtk_paned_set_shrink_end_child(GTK_PANED(new_paned), FALSE);

      gtk_paned_set_end_child(GTK_PANED(paned), new_paned);
    }
  }

  state->tab_containers.push_back(new_tab_container);
  state->active_split_index = static_cast<int>(state->tab_containers.size()) - 1;
}

void prodigeetor_split_container_split_horizontal(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  // Similar to vertical split but with vertical orientation
  GtkWidget *new_tab_container = prodigeetor_tab_container_new();
  prodigeetor_tab_container_set_title_callback(new_tab_container, update_window_title, state);

  if (state->tab_containers.size() == 1) {
    // First split: replace box with paned
    GtkWidget *old_container = state->tab_containers[0];
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    // Remove old container from box
    gtk_box_remove(GTK_BOX(state->root_widget), old_container);

    // Add both containers to paned
    gtk_paned_set_start_child(GTK_PANED(paned), old_container);
    gtk_paned_set_end_child(GTK_PANED(paned), new_tab_container);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

    // Add paned to box
    gtk_box_append(GTK_BOX(state->root_widget), paned);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_widget_set_hexpand(paned, TRUE);
  } else {
    // Subsequent splits: find the current paned and add to it
    GtkWidget *current = state->root_widget;
    GtkWidget *paned = gtk_widget_get_first_child(current);

    if (GTK_IS_PANED(paned)) {
      // Create a new paned to replace the end child
      GtkWidget *end_child = gtk_paned_get_end_child(GTK_PANED(paned));
      GtkWidget *new_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

      g_object_ref(end_child);
      gtk_paned_set_end_child(GTK_PANED(paned), nullptr);

      gtk_paned_set_start_child(GTK_PANED(new_paned), end_child);
      g_object_unref(end_child);

      gtk_paned_set_end_child(GTK_PANED(new_paned), new_tab_container);
      gtk_paned_set_resize_start_child(GTK_PANED(new_paned), TRUE);
      gtk_paned_set_resize_end_child(GTK_PANED(new_paned), TRUE);
      gtk_paned_set_shrink_start_child(GTK_PANED(new_paned), FALSE);
      gtk_paned_set_shrink_end_child(GTK_PANED(new_paned), FALSE);

      gtk_paned_set_end_child(GTK_PANED(paned), new_paned);
    }
  }

  state->tab_containers.push_back(new_tab_container);
  state->active_split_index = static_cast<int>(state->tab_containers.size()) - 1;
}

void prodigeetor_split_container_close_active_split(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state || state->tab_containers.size() <= 1) return;

  // TODO: Implement split removal logic
  // For now, do nothing to avoid complexity
}

void prodigeetor_split_container_new_tab(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_new_tab(tab_container);
  }
}

static void on_open_file_response(GObject *source, GAsyncResult *result, gpointer user_data) {
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  auto *state = static_cast<SplitContainerState *>(user_data);

  GError *error = nullptr;
  GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);

  if (file) {
    char *path = g_file_get_path(file);
    if (path) {
      GtkWidget *tab_container = get_active_tab_container(state);
      if (tab_container) {
        prodigeetor_tab_container_open_file(tab_container, path);
      }
      g_free(path);
    }
    g_object_unref(file);
  } else if (error) {
    if (error->code != GTK_DIALOG_ERROR_DISMISSED) {
      g_warning("Failed to open file: %s", error->message);
    }
    g_error_free(error);
  }
}

void prodigeetor_split_container_open_file(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state || !state->window) return;

  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Open File");
  gtk_file_dialog_open(dialog, GTK_WINDOW(state->window), nullptr,
                       on_open_file_response, state);
}

void prodigeetor_split_container_save_active_file(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_save_active_file(tab_container);
  }
}

void prodigeetor_split_container_close_active_tab(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_close_active_tab(tab_container);
  }
}

void prodigeetor_split_container_next_tab(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_next_tab(tab_container);
  }
}

void prodigeetor_split_container_prev_tab(GtkWidget *container) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_prev_tab(tab_container);
  }
}

void prodigeetor_split_container_select_tab(GtkWidget *container, int index) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (!state) return;

  GtkWidget *tab_container = get_active_tab_container(state);
  if (tab_container) {
    prodigeetor_tab_container_select_tab(tab_container, index);
  }
}

void prodigeetor_split_container_set_window(GtkWidget *container, GtkWidget *window) {
  auto *state = static_cast<SplitContainerState *>(
    g_object_get_data(G_OBJECT(container), "split-container-state"));

  if (state) {
    state->window = window;
  }
}
