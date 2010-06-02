#ifndef _MAIN_H
#define _MAIN_H

#define N_CHANNELS 20

GtkListStore *scene_store;
GtkListStore *sequence_store;
GtkListStore *channel_store;
Project      *project;
Scene        *last_scene;
gboolean      sync_received;

#endif /* _MAIN_H */

