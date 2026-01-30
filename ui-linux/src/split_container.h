#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

// Split container widget that manages multiple tab containers using GtkPaned
GtkWidget *prodigeetor_split_container_new(void);

// Split management
void prodigeetor_split_container_split_vertical(GtkWidget *container);
void prodigeetor_split_container_split_horizontal(GtkWidget *container);
void prodigeetor_split_container_close_active_split(GtkWidget *container);

// File operations forwarding
void prodigeetor_split_container_new_tab(GtkWidget *container);
void prodigeetor_split_container_open_file(GtkWidget *container);
void prodigeetor_split_container_save_active_file(GtkWidget *container);
void prodigeetor_split_container_close_active_tab(GtkWidget *container);

// Tab navigation forwarding
void prodigeetor_split_container_next_tab(GtkWidget *container);
void prodigeetor_split_container_prev_tab(GtkWidget *container);
void prodigeetor_split_container_select_tab(GtkWidget *container, int index);

// Set window reference
void prodigeetor_split_container_set_window(GtkWidget *container, GtkWidget *window);

// Process LSP messages for all editors
void prodigeetor_split_container_tick_all_editors(GtkWidget *container);

G_END_DECLS
