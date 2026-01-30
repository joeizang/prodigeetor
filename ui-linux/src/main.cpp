#include <gtk/gtk.h>

#include "editor_widget.h"

static void on_activate(GApplication *app, gpointer) {
  GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "Prodigeetor");
  gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

  GtkWidget *editor = prodigeetor_editor_widget_new();
  prodigeetor_editor_widget_set_text(editor,
                                     "// Prodigeetor\n"
                                     "let greeting = \"Hello, world!\";\n"
                                     "print(greeting);\n");

  gtk_window_set_child(GTK_WINDOW(window), editor);
  gtk_widget_show(window);
}

int main(int argc, char **argv) {
  GtkApplication *app = gtk_application_new("com.prodigeetor.editor", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
