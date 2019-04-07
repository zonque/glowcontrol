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
#include "scene.h"
#include "scene_editor.h"
#include "main_window.h"
#include "project.h"

static gint widget_width = 600;
static gint widget_height = 400;

static void scene_editor_class_init (SceneEditorClass *class);
static void scene_editor_init             (SceneEditor      *scene_editor);
static void scene_editor_finalize   (GObject          *object);

static GObjectClass *parent_class = NULL;

typedef struct _Control Control;
struct _Control
{
  GtkWidget *scale;
  GtkWidget *entry;
  gint       id;
  Scene     *scene;
};

GType
scene_editor_get_type (void)
{
  static GType scene_editor_type = 0;

  if (!scene_editor_type)
    {
      static const GTypeInfo scene_editor_info =
      {
        sizeof (SceneEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) scene_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (SceneEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) scene_editor_init,
      };
      
      scene_editor_type = g_type_register_static (G_TYPE_OBJECT,
                                                  "SceneEditor", &scene_editor_info, 0);
    }
  
  return scene_editor_type;
}


static void
scene_editor_class_init (SceneEditorClass *class)
{
  GObjectClass *object_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = scene_editor_finalize;
}

static void
scene_editor_init (SceneEditor *editor)
{
  editor->scene      = NULL;
  editor->control    = 0;
  editor->name_entry = NULL;
}

static void
scene_editor_finalize (GObject *object)
{
  SceneEditor *editor;

  editor = SCENE_EDITOR (object);

  if (editor->scene)
    editor->scene->editor = NULL;

  editor->scene = NULL;

  if (editor->widget)
    {
      gtk_widget_destroy (editor->widget);
      editor->widget = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
set_entry (GtkRange *range,
           Control  *c)
{
  gint v;
  gchar *s;

  v = gtk_range_get_value (range);

  scene_set_value (c->scene, c->id, v);

  s = g_strdup_printf ("%d", v);
  gtk_entry_set_text (GTK_ENTRY (c->entry), s);
  g_free (s);
}
                           
static void 
set_scale (GtkEntry *entry,
           Control  *c)
{
  gint v;
  const gchar *s;
  
  s = gtk_entry_get_text (entry);
  v = strtol (s, NULL, 0);

  gtk_range_set_value (GTK_RANGE (c->scale), v);
}

static void
set_name (GtkEntry *entry,
          Scene    *scene)
{
  project_set_scene_name (scene->project,
                          scene,
                          (gchar *) gtk_entry_get_text (entry));
}

static void
name_entry_activate_cb (GtkEntry *entry,
                        gpointer user_data)
{
  set_name (entry, (Scene *) user_data);
}

static void
name_entry_text_notify (GObject *object,
                        GParamSpec *spec,
                        gpointer user_data)
{
  set_name (GTK_ENTRY (object), SCENE (user_data));
}


static gboolean
update_scene_editor (GtkWidget *widget,
                     GtkDirectionType arg1,
                     SceneEditor *editor)
{
  GList *item;
  
  for (item = editor->control; item; item = item->next)
    {
      Control *control = item->data;
      if (!control)
        continue;
      
      set_entry (GTK_RANGE (control->scale), control);
    }
  
  return TRUE;
}

static gboolean    
scene_editor_close_request (GtkWidget *widget,
                            GdkEvent  *event,
                            gpointer   user_data)
{
  SceneEditor *editor = SCENE_EDITOR (user_data);

  g_object_unref (editor);

  return TRUE;
}

void
scene_editor_set_title (SceneEditor *editor,
                        const gchar *name)
{
  gchar *str = g_strdup_printf ("SceneEditor: %s", name);
  gtk_window_set_title (GTK_WINDOW (editor->widget), str);
  g_free (str);
}

void
scene_editor_size_allocate (GtkWidget *widget,
			    GtkAllocation *alloc,
			    gpointer foo)
{
  widget_width = alloc->width;
  widget_height = alloc->height;
}


SceneEditor *
scene_editor_new (Scene *scene)
{
  GtkWidget   *vbox;
  GtkWidget   *scrolledwindow;
  GtkWidget   *table;
  GtkWidget   *hseparator;
  GtkWidget   *label;
  GtkWidget   *name_entry;
  gint         i;
  gchar       *s;
  Control     *c;
  SceneEditor *editor;

  editor = SCENE_EDITOR (g_object_new (TYPE_SCENE_EDITOR, NULL));

  editor->scene = scene;
  scene->editor = editor;

  editor->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (editor->widget), "editor->widget", editor->widget);
  gtk_widget_set_size_request (editor->widget, widget_width, widget_height);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (editor->widget), vbox);

  name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (name_entry), scene->name);
  gtk_box_pack_start (GTK_BOX (vbox), name_entry, FALSE, TRUE, 0);
  
  g_signal_connect (G_OBJECT (name_entry), "activate",
                    G_CALLBACK (name_entry_activate_cb), scene);
  g_signal_connect (G_OBJECT (name_entry), "notify::text",
                    G_CALLBACK (name_entry_text_notify), scene);

  hseparator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, TRUE, 0);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow);
  gtk_object_set_data_full (GTK_OBJECT (editor->widget), "scrolledwindow", scrolledwindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

 
  table = gtk_table_new (N_CHANNELS, 3, FALSE);

  for (i = 0; i < N_CHANNELS; i++)
    {
      c = g_new0 (Control, 1);

      c->id = i;
      c->scene = scene;

      s = g_strdup_printf ("%s (%02d)", 
                           project_get_nth_channel_name (scene->project, i), i+1);
      label = gtk_label_new (s);
      g_free (s);

      gtk_table_attach (GTK_TABLE (table), label, 
                        0, 1, i, i+1,
                        GTK_SHRINK, 0, 3, 2);

      c->scale = gtk_hscale_new_with_range (0, 100, 1);
      gtk_scale_set_draw_value (GTK_SCALE (c->scale), FALSE);
      gtk_table_attach (GTK_TABLE (table), c->scale, 
                        1, 2, i, i+1,
                        GTK_EXPAND | GTK_FILL, 0, 1, 2);

      c->entry = gtk_entry_new_with_max_length (3);
      gtk_entry_set_width_chars (GTK_ENTRY (c->entry), 3);
      gtk_table_attach (GTK_TABLE (table), c->entry, 
                        2, 3, i, i+1,
                        GTK_SHRINK, 0, 1, 2);

      g_signal_connect (G_OBJECT (c->scale), "value-changed",
                        G_CALLBACK (set_entry), c);
      g_signal_connect (G_OBJECT (c->entry), "activate",
                        G_CALLBACK (set_scale), c);

      gtk_range_set_value (GTK_RANGE (c->scale), (gdouble) scene->channel[i]);

      editor->control = g_list_append (editor->control, c);
    }

  update_scene_editor (NULL, 0, editor);


  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow), table);

  gtk_widget_show_all (editor->widget);

  scene_editor_set_title (editor, scene->name);

  g_signal_connect (G_OBJECT (editor->widget), "focus-in-event",
                    G_CALLBACK (update_scene_editor), editor);
  g_signal_connect (G_OBJECT (editor->widget), "delete-event",
                    G_CALLBACK (scene_editor_close_request), editor);
  g_signal_connect (G_OBJECT (editor->widget), "size-allocate",
		    G_CALLBACK (scene_editor_size_allocate), editor);
  return editor;
}

