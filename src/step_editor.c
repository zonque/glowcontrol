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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "types.h"
#include "main.h"
#include "main_window.h"
#include "step.h"
#include "step_editor.h"
#include "main_window.h"
#include "project.h"
#include "scene.h"
#include "gfx.h"


static void step_editor_class_init (StepEditorClass *class);
static void step_editor_init            (StepEditor      *step_editor);
static void step_editor_finalize   (GObject         *object);

static GObjectClass *parent_class = NULL;

GType
step_editor_get_type (void)
{
  static GType step_editor_type = 0;

  if (!step_editor_type)
    {
      static const GTypeInfo step_editor_info =
      {
        sizeof (StepEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) step_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (StepEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) step_editor_init,
      };
      
      step_editor_type = g_type_register_static (G_TYPE_OBJECT,
                                                 "StepEditor", &step_editor_info, 0);
    }
  
  return step_editor_type;
}


static void
step_editor_class_init (StepEditorClass *class)
{
  GObjectClass *object_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = step_editor_finalize;
}

static void
step_editor_init (StepEditor *editor)
{
  editor->step = NULL;
}

static void
step_editor_finalize (GObject *object)
{
  StepEditor *editor;

  editor = STEP_EDITOR (object);

  editor->step->project->step_editor = NULL;

  editor->step = NULL;

  if (editor->widget)
    {
      gtk_widget_destroy (editor->widget);
      editor->widget = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
set_audio_file (GtkWidget  *button, 
                StepEditor *editor)
{
  const gchar *filename;
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (editor->file_selector));

  if (editor->step->filename)
    g_free (editor->step->filename);

  editor->step->filename = g_strdup (filename);
  
  gtk_entry_set_text (GTK_ENTRY (editor->audio_file_entry), filename);
  project_update_step (editor->step->project, editor->step);
}

static void
select_audio_file_cb (GtkButton  *button,
                      StepEditor *editor)
{
  editor->file_selector = gtk_file_selection_new (_("Select Audio File"));

  g_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (editor->file_selector)->ok_button),
                    "clicked",
                    G_CALLBACK (set_audio_file),
                    editor);
  
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (editor->file_selector)->ok_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy), 
                            (gpointer) editor->file_selector);
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (editor->file_selector)->cancel_button),
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            (gpointer) editor->file_selector);
  
  gtk_widget_show (editor->file_selector);
}


static gboolean    
step_editor_close_request (GtkWidget *widget,
                           GdkEvent  *event,
                           gpointer   user_data)
{
  StepEditor *editor = STEP_EDITOR (user_data);

  g_object_unref (editor);

  return TRUE;
}

void
step_editor_set_title (StepEditor  *editor,
                       const gchar *name)
{
  gchar *str = g_strdup_printf ("StepEditor: %s", name);
  gtk_window_set_title (GTK_WINDOW (editor->widget), str);
  g_free (str);
}

static void
step_scene_selected (GtkOptionMenu *menu,
                     StepEditor    *editor)
{
  GtkTreeIter iter;
  gint n;

  n = gtk_option_menu_get_history (menu);

  if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (editor->step->project->scene_store),
                                      &iter, NULL, n))
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (editor->step->project->scene_store), 
                      &iter,
                      COLUMN_DATA, &editor->step->scene,
                      -1);
  project_update_step (editor->step->project, editor->step);
}

static void
time_value_notify (GtkEntry   *entry,
                   GParamSpec *spec,
                   StepEditor *editor)
{
  const gchar *s;

  s = gtk_entry_get_text (entry);
  editor->step->time = strtol (s, NULL, 0);
  
  project_update_step (editor->step->project, editor->step);
}

static GtkWidget *
create_edit_widget (Project    *project,
                    StepEditor *editor,
                    StepType    type)
{
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  gchar     *s;

  table = gtk_table_new (2, 2, FALSE);

  switch (type)
    {
    case STEP_TYPE_DIMM:
      {
        GtkItemFactoryEntry menu_entry;
        GtkTreeIter iter;
        GtkWidget *menu = NULL;
        GtkItemFactory *item_factory;
        Scene *scene;
        gint n = 0, selected_scene = 0;

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->scene_store), &iter))
          {
            item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<scenes>", NULL);

            do
              {
                gtk_tree_model_get (GTK_TREE_MODEL (project->scene_store), &iter,
                                    COLUMN_DATA, &scene,
                                    -1);

                if (editor->step->scene == scene)
                  selected_scene = n;
                
                menu_entry.path = g_strdup_printf ("/%s", scene->name);
                menu_entry.accelerator = NULL;
                menu_entry.callback = NULL;
                menu_entry.callback_action = 0;
                menu_entry.item_type = "<Item>";
                menu_entry.extra_data = NULL;
                n++;

                gtk_item_factory_create_item (item_factory, &menu_entry, NULL, 1);
              }
            while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->scene_store), &iter));

            menu = gtk_item_factory_get_widget (item_factory, "<scenes>");
          }
        else
          {
            label = gtk_label_new (_("No scenes in project"));
            gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 2);
            return table;
          }

        label = gtk_label_new (_("Destination scene"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 5, 2);
        
        editor->scene_menu = gtk_option_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU (editor->scene_menu), menu);
        gtk_table_attach (GTK_TABLE (table), editor->scene_menu, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 2);
        g_signal_connect (G_OBJECT (editor->scene_menu), "changed",
                          G_CALLBACK (step_scene_selected), editor);
        gtk_option_menu_set_history (GTK_OPTION_MENU (editor->scene_menu), selected_scene);
        
        label = gtk_label_new (_("Time (in ms)"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, 0, 0, 5, 2);

        entry = gtk_entry_new ();
        s = g_strdup_printf ("%d", editor->step->time);
        gtk_entry_set_text (GTK_ENTRY (entry), s);
        g_free (s);
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, 
                          GTK_EXPAND | GTK_FILL, 0, 2, 2);
        g_signal_connect (G_OBJECT (entry), "notify::text",
                          G_CALLBACK (time_value_notify), editor);
      }
      break;
    case STEP_TYPE_SLEEP:
      {
        label = gtk_label_new (_("Time (in ms)"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 5, 2);
        entry = gtk_entry_new ();
        s = g_strdup_printf ("%d", editor->step->time);
        gtk_entry_set_text (GTK_ENTRY (entry), s);
        g_free (s);
        gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, 0, 0, 2, 2);
            g_signal_connect (G_OBJECT (entry), "notify::text",
                          G_CALLBACK (time_value_notify), editor);
      }
      break;
    case STEP_TYPE_SYNC:
      {
        label = gtk_label_new (_("This command has no options"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 2);
      }
      break;
    case STEP_TYPE_AUDIO:
      {
        GtkWidget *button;

        label = gtk_label_new (_("Filename"));
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 5, 2);

        editor->audio_file_entry = gtk_entry_new ();
        gtk_table_attach (GTK_TABLE (table), editor->audio_file_entry, 
                          1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 5, 2);
        gtk_entry_set_text (GTK_ENTRY (editor->audio_file_entry),
                            editor->step->filename);

        button = gtk_button_new_with_label (_("Select..."));
        gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1, 0, 0, 5, 2);
        g_signal_connect (G_OBJECT (button), "clicked",
                          G_CALLBACK (select_audio_file_cb), editor);
      }
      break;
    default:
      g_warning ("Ooops. unknown step type %d!?", type);
    }

  return table;
}

static void
type_changed (GtkOptionMenu *optionmenu,
              StepEditor    *editor)
{
  StepType type;

  type = gtk_option_menu_get_history (optionmenu);

  gtk_container_remove (GTK_CONTAINER (editor->container), editor->edit_widget);
  editor->edit_widget = create_edit_widget (editor->step->project, editor, type);
  gtk_box_pack_start (GTK_BOX (editor->container), editor->edit_widget, TRUE, TRUE, 10);
  gtk_widget_show_all (editor->edit_widget);

  editor->step->type = type;

  project_update_step (editor->step->project, editor->step);
}

StepEditor *
step_editor_new (Step *step)
{
  GtkWidget   *label, *hbox;
  GtkWidget   *option_menu;
  StepEditor  *editor;
  GtkWidget   *menu;
  GtkItemFactory    *item_factory = NULL;
  GtkItemFactoryEntry menu_entries[] = {
    { _("/Dimm to a scene"),    NULL,   NULL,   1, "<Item>" },
    { _("/Sleep for awhile"),   NULL,   NULL,   1, "<Item>" },
    { _("/Wait for sync"),      NULL,   NULL,   1, "<Item>" },
    { _("/Play audio file"),    NULL,   NULL,   1, "<Item>" }
  };

  editor = STEP_EDITOR (g_object_new (TYPE_STEP_EDITOR, NULL));

  editor->step = step;

  editor->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (editor->widget), "editor->widget", editor->widget);
  gtk_widget_set_size_request (editor->widget, 400, 150);

  gtk_window_set_title (GTK_WINDOW (editor->widget), "StepEditor");

  editor->container = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (editor->widget), editor->container);

  hbox = gtk_hbox_new (FALSE, 2);
  
  label = gtk_label_new (_("Type"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  option_menu = gtk_option_menu_new ();
  
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<steptypes>", NULL);
  gtk_item_factory_create_items (item_factory,
                                 sizeof (menu_entries) / sizeof (menu_entries[0]),
                                 menu_entries, 
                                 NULL);

  menu = gtk_item_factory_get_widget (item_factory, "<steptypes>");

  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), step->type);

  g_signal_connect (G_OBJECT (option_menu), "changed",
                    G_CALLBACK (type_changed), editor);

  gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (editor->container), hbox, FALSE, FALSE, 0);


  gtk_box_pack_start (GTK_BOX (editor->container), gtk_hseparator_new (), FALSE, FALSE, 0);

  editor->edit_widget = create_edit_widget (step->project, editor, step->type);
  gtk_box_pack_start (GTK_BOX (editor->container), editor->edit_widget, TRUE, TRUE, 10);
  

  gtk_widget_show_all (editor->widget);
  
  g_signal_connect (G_OBJECT (editor->widget), "delete-event",
                    G_CALLBACK (step_editor_close_request), editor);

  return editor;
}

