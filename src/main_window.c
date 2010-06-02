/* 
 * Copyright (C) 2004 Daniel Mack <daniel@yoobay.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "types.h"
#include "main.h"
#include "project.h"
#include "main_window.h"
#include "scene.h"
#include "scene_editor.h"
#include "step.h"
#include "step_editor.h"
#include "audio.h"
#include "gfx.h"
#include "out.h"

GtkTreeView *scene_view;
GtkTreeView *sequence_view;
GtkTreeView *channel_view;

GtkWidget   *play_button;
GtkWidget   *progress_bar;
Step        *current_step = NULL;
guint        sequence_source_id = 0;


static void save_file (void);

static void
tree_view_edited (GtkCellRendererText *cell,
                  gchar               *path_string,
                  gchar               *new_text,
                  gpointer             data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_STRING, new_text, -1);
  gtk_tree_path_free (path);

  project->dirty = TRUE;
}

static void
check_dirty (void)
{
  GtkWidget *dialog;
  gint result;

  if (!project->dirty)
    return;

  dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   _("Project has been modified.\n"
                                     "Save changes?"));
  
  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  
  if (result == GTK_RESPONSE_YES)
    {
      save_file ();
    }
}

static void
new_project (void)
{
  check_dirty ();

  if (project)
    g_object_unref (project);

  project = project_new (scene_store, sequence_store, channel_store);
}

static gboolean
quit (void)
{
  check_dirty ();

  g_print ("quit!\n");
  audio_stop ();
  gtk_main_quit ();
  return TRUE;
}

static void
open_file (GtkWidget *button, 
           gpointer   user_data) 
{
  const gchar *filename;

  check_dirty ();

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (user_data));

  if (project)
    g_object_unref (project);

  project = project_new_from_file (filename,
                                   scene_store, sequence_store, channel_store);

  if (project == NULL)
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (GTK_WINDOW (user_data),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       _("Error loading file '%s': %s"),
                                       filename, g_strerror (errno));
      
      g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
                                G_CALLBACK (gtk_widget_destroy),
                                GTK_OBJECT (dialog));

      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      project = project_new (scene_store, sequence_store, channel_store);
    }
}

static void
open_file_cb (void)
{
  GtkWidget *file_selector = gtk_file_selection_new (_("Open Project File"));

  g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                    "clicked",
                    G_CALLBACK (open_file),
                    file_selector);
  
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy), 
                            (gpointer) file_selector);
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            (gpointer) file_selector);
  
  gtk_widget_show (file_selector);
}

static void
save_file_as (GtkWidget *button, 
              gpointer   user_data) 
{
  const gchar *filename;
  GtkWidget *dialog;
  gint result;
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (user_data));

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (user_data),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_QUESTION,
                                       GTK_BUTTONS_YES_NO,
                                       _("File '%s' exists - replace?"),
                                       filename);

      result = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      if (result != GTK_RESPONSE_YES)
        return;
    }

  if (!project_write_to_file (project, filename))
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (user_data),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       _("Error writing file '%s': %s"),
                                       filename, g_strerror (errno));

      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      return;
    }

  if (project->filename)
    g_free (project->filename);

  project->filename = g_strdup (filename);
}

static void
save_file_as_cb (void)
{
  GtkWidget *file_selector = gtk_file_selection_new (_("Save Project File as..."));

  g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                    "clicked",
                    G_CALLBACK (save_file_as),
                    file_selector);
  
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy), 
                            (gpointer) file_selector);
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            (gpointer) file_selector);
  
  gtk_widget_show (file_selector);
}

static void
save_file (void)
{
  g_print ("save ==> %s\n", project->filename);
  if (project->filename && *project->filename)
    project_write_to_file (project, project->filename);
  else
    save_file_as_cb ();
}

static GtkWidget *
button_new (gchar    *xpm_filename,
            gchar    *label_text,
            gboolean  toggle)
{
  GtkWidget *button;
  GtkWidget *box, *obox;
  GtkWidget *label;
  GtkWidget *image;
  
  box = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 2);
  
  if (xpm_filename && *xpm_filename)
    {
      image = gtk_image_new_from_pixbuf (pixbuf_new (xpm_filename));
      gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 2);
    }

  if (label_text && *label_text)
    {
      label = gtk_label_new (label_text);
      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 2);
    }
  
  obox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (obox), box, TRUE, FALSE, 0);

  if (toggle)
    button = gtk_toggle_button_new ();
  else
    button = gtk_button_new ();

  gtk_container_add (GTK_CONTAINER (button), obox);

  return button;
}

static void
scene_new_clicked (void)
{
  Scene *scene = scene_new (project);

  project_add_scene (project, scene);

  project_open_scene_editor (project, scene);
}

static void
scene_copy_clicked (void)
{
  Scene *scene, *new_scene;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (scene_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (scene_store), &iter,
                      COLUMN_DATA, &scene,
                      -1);

  new_scene = scene_duplicate (scene);

  project_add_scene (project, new_scene);
  g_object_unref (new_scene);

  project_open_scene_editor (project, new_scene);
}

static void
scene_edit_clicked (void)
{
  Scene *scene;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (scene_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (scene_store), &iter,
                      COLUMN_DATA, &scene,
                      -1);

  project_open_scene_editor (project, scene);
}

static void
scene_remove_clicked (void)
{
  Scene *scene;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (scene_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;
  
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (scene_store), &iter);

  gtk_tree_model_get (GTK_TREE_MODEL (scene_store), &iter,
                      COLUMN_DATA, &scene,
                      -1);

  gtk_list_store_remove (GTK_LIST_STORE (scene_store), &iter);
  g_object_unref (scene);

  if (scene == last_scene)
    last_scene = NULL;
}

static void
step_new_clicked (void)
{
  GtkTreeIter       iter, new_iter;
  GtkTreeSelection *selection;
  Step             *step = step_new (project);

  selection = gtk_tree_view_get_selection (sequence_view);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    project_add_step (project, &iter, step, &new_iter);
  else
    project_add_step (project, NULL, step, &new_iter);

  gtk_tree_selection_select_iter (selection, &new_iter);

  project_open_step_editor (project, step);
}

static void
step_remove_clicked (void)
{
  Step *step;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (sequence_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;
  
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (sequence_store), &iter);

  gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
                      COLUMN_DATA, &step,
                      -1);

  gtk_list_store_remove (GTK_LIST_STORE (sequence_store), &iter);
}

static void
step_edit_clicked (void)
{
  Step *step;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (sequence_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;
  
  gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
                      COLUMN_DATA, &step,
                      -1);

  project_open_step_editor (project, step);
}

static gboolean
sequence_first (void)
{
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (sequence_store), &iter))
    return FALSE;

  selection = gtk_tree_view_get_selection (sequence_view);

  gtk_tree_selection_select_iter (selection, &iter);

  return TRUE;
}


static void
sequence_prev (void)
{
  GtkTreeIter       iter;
  GtkTreePath      *path;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (sequence_view);

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (sequence_store), &iter);

  if (!gtk_tree_path_prev (path))
    return;

  if (sequence_source_id > 0)
    {
      current_step->progress = 0;
      gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
                          COLUMN_DATA, &current_step,
                          -1);
      current_step->progress = 0;
    }
  
  gtk_tree_selection_select_path (selection, path);
}

static gboolean
sequence_next (void)
{
  GtkTreeIter       iter;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (sequence_view);

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return FALSE;
  if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (sequence_store), &iter))
    return FALSE;

  gtk_tree_selection_select_iter (selection, &iter);

  return TRUE;
}

static void
sequence_next_clicked (void)
{
  if (sequence_source_id != 0)
    {
      sync_received = TRUE;
      return;
    }      
  
  sequence_next ();
}

static gboolean
sequence_tick (gpointer *foo)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeIter iter;
  gdouble perc;

  if (!current_step)
    {
      if (!sequence_next ())
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (play_button), FALSE);
          return FALSE;
        }

      selection = gtk_tree_view_get_selection (sequence_view);
      if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
          return FALSE;
        }
      
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (sequence_store), &iter);
  
      gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
                          COLUMN_DATA, &current_step,
                          -1);
      current_step->progress = 0;
      g_print ("new current step. time = %d\n", current_step->time);
    }

  if ((current_step->type == STEP_TYPE_DIMM ||
       current_step->type == STEP_TYPE_SLEEP) &&
       current_step->time > 0)
    {
      perc = ((gdouble) (current_step->progress) / (gdouble) current_step->time) * 0.95;

      gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_bar), perc);
    }

  if (step_tick (current_step) == FALSE)
    {
      if (current_step->type == STEP_TYPE_DIMM)
        last_scene = current_step->scene;

      current_step = NULL;
      return TRUE;
    }

  return TRUE;
}

static gboolean
audio_stop_timeout (gpointer foo)
{
  audio_stop ();
  return FALSE;
}

static void
sequence_play_clicked (void)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeIter iter;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (play_button)) == FALSE)
    {
      g_print ("stopping.\n");
      if (sequence_source_id > 0)
        g_source_remove (sequence_source_id);
      gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_bar), 0);
      sequence_source_id = 0;
      current_step = NULL;
      audio_stop ();
      g_timeout_add (50, audio_stop_timeout, NULL);
      return;
    }

  selection = gtk_tree_view_get_selection (sequence_view);
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      sequence_first ();
      selection = gtk_tree_view_get_selection (sequence_view);
      if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
        return;
    }
  
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (sequence_store), &iter);
  
  gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
                      COLUMN_DATA, &current_step,
                      -1);

  current_step->progress = 0;
  last_scene = NULL;
  g_print ("starting...\n");
  sequence_source_id = g_timeout_add (10, (GSourceFunc) sequence_tick, NULL);
}

void
sequence_back_to_sync (void)
{
  GtkTreeIter       iter;
  GtkTreePath      *path;
  GtkTreeSelection *selection;
  Step             *last_sync = NULL, *last_dimm = NULL;
  
  if (!sequence_source_id)
    return;

  selection = gtk_tree_view_get_selection (sequence_view);

  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (sequence_store), &iter);

  do
    {
      gtk_tree_selection_select_path (selection, path);
      if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
        return;

      gtk_tree_model_get (GTK_TREE_MODEL (sequence_store), &iter,
			  COLUMN_DATA, &current_step,
			  -1);
      
      if (last_sync == NULL && current_step->type == STEP_TYPE_SYNC)
	last_sync = current_step;

      if (last_sync && current_step->type == STEP_TYPE_DIMM)
        {
          last_dimm = current_step;
          break;
        }
    }
  while (gtk_tree_path_prev (path));

  if (last_sync)
    {
      if (last_dimm && last_dimm->scene)
        {
          g_print ("back to sync: setting values from >%s<\n", last_dimm->scene->name);
          scene_send_all_values (last_dimm->scene);
        } 
      else 
        {
          gint chn[N_CHANNELS];
          gint val[N_CHANNELS];
          gint n;
          
          g_print ("back to sync: setting zero values.\n");
          
          for (n=0; n<N_CHANNELS; n++)
            {
              chn[n] = n;
              val[n] = 0;
            }
          
          output_channel_set (chn, val, N_CHANNELS);
        }

      current_step = last_sync;
      sequence_next ();
    }

  if (current_step)
    {
      current_step->progress = 0;
      last_scene = NULL;
    }

  gtk_progress_bar_update (GTK_PROGRESS_BAR (progress_bar), 0);
}

static GtkTreeView *
setup_list_view (GtkListStore *store)
{
   GtkWidget *tree;
   GtkTreeViewColumn *column;
   GtkCellRenderer *text_renderer, *icon_renderer, *id_renderer;

   tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

   text_renderer = gtk_cell_renderer_text_new ();
   id_renderer   = gtk_cell_renderer_text_new ();
   icon_renderer = gtk_cell_renderer_pixbuf_new ();

   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

   column = gtk_tree_view_column_new_with_attributes ("icon", icon_renderer,
                                                      "pixbuf", COLUMN_ICON,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* igittigittigitt */
   if (store == channel_store)
     {
       column = gtk_tree_view_column_new_with_attributes ("id", id_renderer,
                                                          "text", COLUMN_ID,
                                                          NULL);
       gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
     }

   column = gtk_tree_view_column_new_with_attributes ("text", text_renderer,
                                                      "text", COLUMN_STRING,
                                                      "editable", COLUMN_IS_EDITABLE,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   
   g_signal_connect (G_OBJECT (text_renderer), "edited",
                     G_CALLBACK (tree_view_edited), store);

   return GTK_TREE_VIEW (tree);
}

GtkWidget*
create_main_window (void)
{
  GtkWidget *window;
  GtkWidget *vbox, *hbox;
  GtkWidget *vbox_scenes, *vbox_sequence, *vbox_channels;
  GtkWidget *button;
  GtkWidget *menubar;
  GtkWidget *notebook;
  GtkWidget *status_bar;
  GtkWidget *label;
  GtkWidget *widget;
  
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkItemFactoryEntry menu_entries[] = {
    { "/_File",             NULL,      NULL,            0, "<Branch>" },
    //    { "/File/tear1",        NULL,      NULL,         0, "<Tearoff>" },
    { "/File/_New",         NULL,      new_project,     1, "<StockItem>", GTK_STOCK_NEW },
    { "/File/_Open...",     NULL,      open_file_cb,    1, "<StockItem>", GTK_STOCK_OPEN },
    { "/File/_Save",        NULL,      save_file,       1, "<StockItem>", GTK_STOCK_SAVE },
    { "/File/Save as...",   NULL,      save_file_as_cb, 1, "<StockItem>", GTK_STOCK_SAVE_AS },
    { "/File/sep1",         NULL,      NULL,            0, "<Separator>" },
    { "/File/_Quit",        NULL,      (GtkItemFactoryCallback) quit,
                                                        0, "<StockItem>", GTK_STOCK_QUIT} 
  };

  /*** SETUP GTK MAIN WINDOW ***/
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "main_window", window);
//  gtk_widget_set_usize (window, 800, 600);
  gtk_window_set_title (GTK_WINDOW (window), "GlowControl");
  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (quit), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);


  /* The menu */

  accel_group = gtk_accel_group_new ();
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items (item_factory, 
                                 sizeof (menu_entries) / sizeof (menu_entries[0]),
                                 menu_entries, 
                                 NULL);
  menubar = gtk_item_factory_get_widget (item_factory, "<main>");
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  status_bar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE, FALSE, 0);
 
  progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (status_bar), progress_bar, TRUE, TRUE, 0);


  /*** PREPARE NOTEBOOK PAGES ***/

  /* NOTEBOOK PAGE: SCENES */

  vbox_scenes = gtk_vbox_new (FALSE, 0);
  widget = gtk_scrolled_window_new (NULL, NULL);
  scene_view = setup_list_view (scene_store);

  g_signal_connect (G_OBJECT (scene_view), "row-activated",
                    G_CALLBACK (scene_edit_clicked), NULL);

  gtk_tree_view_set_reorderable (scene_view, TRUE);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (scene_view));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);

  gtk_box_pack_start (GTK_BOX (vbox_scenes), widget, TRUE, TRUE, 0);


  /* button bar */
  hbox = gtk_hbox_new (TRUE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (scene_new_clicked), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_COPY);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (scene_copy_clicked), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (scene_remove_clicked), NULL);
  gtk_box_pack_start (GTK_BOX (vbox_scenes), hbox, FALSE, FALSE, 0);

  /* NOTEBOOK: SEQUENCE */

  vbox_sequence = gtk_vbox_new (FALSE, 0);
  widget = gtk_scrolled_window_new (NULL, NULL);
  sequence_view = setup_list_view (sequence_store);

  g_signal_connect (G_OBJECT (sequence_view), "row-activated",
                    G_CALLBACK (step_edit_clicked), NULL);

  gtk_tree_view_set_reorderable (sequence_view, TRUE);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (sequence_view));
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);

  gtk_box_pack_start (GTK_BOX (vbox_sequence), widget, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (TRUE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (step_new_clicked), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (step_remove_clicked), NULL);

  gtk_box_pack_start (GTK_BOX (vbox_sequence), hbox, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (TRUE, 0);

  button = button_new ("../pixmaps/first.xpm", _("First"), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (sequence_first), NULL);

  button = button_new ("../pixmaps/prev.xpm", _("Previous"), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (sequence_prev), NULL);

  play_button = button_new ("../pixmaps/play.xpm", _("Play"), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), play_button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (play_button), "clicked",
                    G_CALLBACK (sequence_play_clicked), NULL);

  button = button_new ("../pixmaps/next.xpm", _("Next"), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (sequence_next_clicked), NULL);

  button = button_new ("../pixmaps/sync.xpm", _("Back to sync"), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (sequence_back_to_sync), NULL);


  gtk_box_pack_start (GTK_BOX (vbox_sequence), hbox, FALSE, FALSE, 0);
                

  /* NOTEBOOK: CHANNELS */

  vbox_channels = gtk_vbox_new (FALSE, 2);
  widget = gtk_scrolled_window_new (NULL, NULL);

  gtk_box_pack_start (GTK_BOX (vbox_channels), widget, TRUE, TRUE, 0);

  channel_view = setup_list_view (channel_store);
  gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (channel_view));

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);
  

  /*** GATHER STUFF FOR NOTEBOOK ***/
  label = gtk_label_new (_("Channels"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_channels, label);

  label = gtk_label_new (_("Scenes"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_scenes, label);

  label = gtk_label_new (_("Sequence"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox_sequence, label);

  gtk_widget_show_all (window);
  
  return window;
}
