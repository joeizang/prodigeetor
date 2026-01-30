#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string>

#include "split_container.h"

struct AppData {
  GtkWidget *window = nullptr;
  GtkWidget *split_container = nullptr;
};


static gboolean on_key_pressed(GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer user_data) {
  auto *data = static_cast<AppData *>(user_data);
  bool ctrl = (state & GDK_CONTROL_MASK) != 0;
  bool shift = (state & GDK_SHIFT_MASK) != 0;

  // File operations
  if (ctrl && keyval == GDK_KEY_t) {
    prodigeetor_split_container_new_tab(data->split_container);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_o) {
    prodigeetor_split_container_open_file(data->split_container);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_s) {
    prodigeetor_split_container_save_active_file(data->split_container);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_w) {
    prodigeetor_split_container_close_active_tab(data->split_container);
    return TRUE;
  }

  // Split operations
  if (ctrl && keyval == GDK_KEY_backslash && !shift) {
    prodigeetor_split_container_split_vertical(data->split_container);
    return TRUE;
  }
  if (ctrl && shift && keyval == GDK_KEY_backslash) {
    prodigeetor_split_container_split_horizontal(data->split_container);
    return TRUE;
  }

  // Tab navigation
  if (ctrl && keyval == GDK_KEY_bracketright) {
    prodigeetor_split_container_next_tab(data->split_container);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_bracketleft) {
    prodigeetor_split_container_prev_tab(data->split_container);
    return TRUE;
  }

  // Tab selection by number (Ctrl+1 through Ctrl+9)
  if (ctrl && keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
    int index = keyval - GDK_KEY_1;
    prodigeetor_split_container_select_tab(data->split_container, index);
    return TRUE;
  }

  return FALSE;
}

static void on_activate(GApplication *app, gpointer) {
  auto *data = new AppData();

  GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "Prodigeetor");
  gtk_window_set_default_size(GTK_WINDOW(window), 1200, 700);
  data->window = window;

  // Create split container
  GtkWidget *split_container = prodigeetor_split_container_new();
  prodigeetor_split_container_set_window(split_container, window);
  data->split_container = split_container;

  // Add keyboard shortcuts
  GtkEventController *key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), data);
  gtk_widget_add_controller(window, key_controller);

  gtk_window_set_child(GTK_WINDOW(window), split_container);
  g_object_set_data_full(G_OBJECT(window), "app-data", data, [](gpointer ptr) {
    delete static_cast<AppData *>(ptr);
  });
  gtk_window_present(window);
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("com.prodigeetor.editor", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
