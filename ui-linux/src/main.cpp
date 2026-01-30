#include <gtk/gtk.h>
#include <string>

#include "editor_widget.h"

struct AppData {
  GtkWidget *window = nullptr;
  GtkWidget *editor = nullptr;
  std::string current_file_path;
};

static void save_to_file(AppData *data, const char *path) {
  if (!data || !path) {
    return;
  }
  char *text = prodigeetor_editor_widget_get_text(data->editor);
  GError *error = nullptr;
  if (!g_file_set_contents(path, text, -1, &error)) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(data->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "Failed to save file: %s", error ? error->message : "Unknown error");
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
    if (error) {
      g_error_free(error);
    }
  }
  g_free(text);
}

static void on_open_response(GObject *source, GAsyncResult *result, gpointer user_data) {
  auto *data = static_cast<AppData *>(user_data);
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  GFile *file = gtk_file_dialog_open_finish(dialog, result, nullptr);
  if (!file) {
    return;
  }

  char *path = g_file_get_path(file);
  char *contents = nullptr;
  gsize length = 0;
  GError *error = nullptr;

  if (g_file_get_contents(path, &contents, &length, &error)) {
    prodigeetor_editor_widget_set_text(data->editor, contents);
    prodigeetor_editor_widget_set_file_path(data->editor, path);
    data->current_file_path = path;

    char *basename = g_file_get_basename(file);
    std::string title = std::string("Prodigeetor - ") + basename;
    gtk_window_set_title(GTK_WINDOW(data->window), title.c_str());
    g_free(basename);
    g_free(contents);
  } else {
    GtkWidget *error_dialog = gtk_message_dialog_new(
        GTK_WINDOW(data->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "Failed to open file: %s", error ? error->message : "Unknown error");
    gtk_window_present(GTK_WINDOW(error_dialog));
    g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
    if (error) {
      g_error_free(error);
    }
  }

  g_free(path);
  g_object_unref(file);
}

static void on_save_response(GObject *source, GAsyncResult *result, gpointer user_data) {
  auto *data = static_cast<AppData *>(user_data);
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  GFile *file = gtk_file_dialog_save_finish(dialog, result, nullptr);
  if (!file) {
    return;
  }

  char *path = g_file_get_path(file);
  data->current_file_path = path;
  save_to_file(data, path);

  char *basename = g_file_get_basename(file);
  std::string title = std::string("Prodigeetor - ") + basename;
  gtk_window_set_title(GTK_WINDOW(data->window), title.c_str());
  g_free(basename);
  g_free(path);
  g_object_unref(file);
}

static void open_file(AppData *data) {
  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Open File");
  gtk_file_dialog_open(dialog, GTK_WINDOW(data->window), nullptr, on_open_response, data);
}

static void save_file(AppData *data) {
  if (!data->current_file_path.empty()) {
    save_to_file(data, data->current_file_path.c_str());
  } else {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Save File");
    gtk_file_dialog_save(dialog, GTK_WINDOW(data->window), nullptr, on_save_response, data);
  }
}

static gboolean on_key_pressed(GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer user_data) {
  auto *data = static_cast<AppData *>(user_data);
  bool ctrl = (state & GDK_CONTROL_MASK) != 0;

  if (ctrl && keyval == GDK_KEY_o) {
    open_file(data);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_s) {
    save_file(data);
    return TRUE;
  }
  return FALSE;
}

static void on_activate(GApplication *app, gpointer) {
  auto *data = new AppData();

  GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "Prodigeetor");
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
  data->window = window;

  GtkWidget *editor = prodigeetor_editor_widget_new();
  prodigeetor_editor_widget_set_text(editor,
                                     "// Prodigeetor\n"
                                     "// Use Ctrl+O to open a file, Ctrl+S to save\n"
                                     "let greeting = \"Hello, world!\";\n"
                                     "print(greeting);\n");
  prodigeetor_editor_widget_set_file_path(editor, "Sample.ts");
  prodigeetor_editor_widget_set_theme_path(editor, "themes/default.json");
  data->editor = editor;

  GtkWidget *scroller = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), editor);
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroller));
  prodigeetor_editor_widget_attach_scroll(editor, vadj, scroller);

  GtkEventController *key_controller = gtk_event_controller_key_new();
  g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_pressed), data);
  gtk_widget_add_controller(window, key_controller);

  gtk_window_set_child(GTK_WINDOW(window), scroller);
  g_object_set_data_full(G_OBJECT(window), "app-data", data, [](gpointer ptr) {
    delete static_cast<AppData *>(ptr);
  });
  gtk_widget_show(window);
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("com.prodigeetor.editor", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
