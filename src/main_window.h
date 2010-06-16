#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

void       update_scene_list (void);
GtkWidget *create_main_window (void);
void       sequence_back_to_sync (void);
void       set_mainwindow_title(const gchar *name);

#endif /* MAIN_WINDOW_H */
