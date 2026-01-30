#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

// Tab container widget that manages multiple editor tabs using GtkNotebook
GtkWidget *prodigeetor_tab_container_new(void);

// Tab management
void prodigeetor_tab_container_new_tab(GtkWidget *container);
void prodigeetor_tab_container_open_file(GtkWidget *container, const char *file_path);
void prodigeetor_tab_container_close_active_tab(GtkWidget *container);

// File operations on active tab
void prodigeetor_tab_container_save_active_file(GtkWidget *container);
void prodigeetor_tab_container_save_active_file_as(GtkWidget *container);

// Tab navigation
void prodigeetor_tab_container_next_tab(GtkWidget *container);
void prodigeetor_tab_container_prev_tab(GtkWidget *container);
void prodigeetor_tab_container_select_tab(GtkWidget *container, int index);

// Get active editor
GtkWidget *prodigeetor_tab_container_get_active_editor(GtkWidget *container);

// Set window title callback
void prodigeetor_tab_container_set_title_callback(GtkWidget *container,
                                                   void (*callback)(const char *title, void *user_data),
                                                   void *user_data);

G_END_DECLS
