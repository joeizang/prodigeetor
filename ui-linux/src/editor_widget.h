#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *prodigeetor_editor_widget_new(void);
void prodigeetor_editor_widget_set_text(GtkWidget *widget, const char *text);

G_END_DECLS
