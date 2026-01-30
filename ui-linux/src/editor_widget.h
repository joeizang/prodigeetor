#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *prodigeetor_editor_widget_new(void);
void prodigeetor_editor_widget_set_text(GtkWidget *widget, const char *text);
void prodigeetor_editor_widget_set_file_path(GtkWidget *widget, const char *path);
void prodigeetor_editor_widget_set_theme_path(GtkWidget *widget, const char *path);
void prodigeetor_editor_widget_attach_scroll(GtkWidget *widget, GtkAdjustment *vadj, GtkWidget *viewport);

G_END_DECLS
