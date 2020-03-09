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
#include "out.h"
#include "scene.h"
#include "main_window.h"
#include "project.h"
#include "xml_writer.h"
#include "xml_parser.h"

static void scene_class_init (SceneClass *class);
static void scene_init       (Scene      *scene);
static void scene_finalize   (GObject    *object);

static GObjectClass *parent_class = NULL;

GType
scene_get_type (void)
{
  static GType scene_type = 0;

  if (!scene_type)
    {
      static const GTypeInfo scene_info =
      {
        sizeof (SceneClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) scene_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (Scene),
        0,              /* n_preallocs */
        (GInstanceInitFunc) scene_init,
        NULL,
      };
      
      scene_type = g_type_register_static (G_TYPE_OBJECT,
                                           "Scene", &scene_info, 0);
    }
  
  return scene_type;
}


static void
scene_class_init (SceneClass *class)
{
  GObjectClass *object_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = scene_finalize;
}

static void
scene_init (Scene *scene)
{
  scene->name = g_strdup (_("<untitled Scene>"));
  scene->project = NULL;
  scene->editor = NULL;
  memset (scene->channel, 0, sizeof(scene->channel));
}

static void
scene_finalize (GObject *object)
{
  Scene *scene;

  scene = SCENE (object);

  if (scene->name)
    {
      g_free (scene->name);
      scene->name = NULL;
    }
  if (scene->editor)
    {
      g_object_unref (scene->editor);
      scene->editor = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

Scene *
scene_new (Project *project)
{
  Scene *scene;

  scene = SCENE (g_object_new (TYPE_SCENE, NULL));
  scene->project = project;
  scene->id = project->last_scene_id++;

  return scene;
}

Scene *
scene_duplicate (Scene *scene)
{
  Scene *new_scene = scene_new (scene->project);

  if (new_scene->name)
    g_free (new_scene->name);

  new_scene->name = g_strdup (scene->name);

  memcpy (new_scene->channel, scene->channel, N_CHANNELS);
  
  return new_scene;
}

void
scene_set_value (Scene *scene,
                 gint n, gint val)
{
  g_return_if_fail (n >= 0 && n < N_CHANNELS);

  scene->channel[n] = val;
  output_channel_set (&n, &val, 1);
  scene->project->dirty = TRUE;
}

void
scene_send_all_values (Scene *scene)
{
  gint i, n[N_CHANNELS];

  for (i = 0; i < N_CHANNELS; i++)
    n[i] = i;

  output_channel_set (n, scene->channel, N_CHANNELS);
}

void
scene_dump (Scene *scene,
            XMLWriter *writer)
{
  gint n;
  gchar *id, *v;

  id = g_strdup_printf ("%d", scene->id);
  xml_write_open_tag (writer, "Scene", "id", id, NULL);
  g_free (id);

  xml_write_element (writer, "Name", scene->name, NULL);

  for (n=0; n<N_CHANNELS; n++)
    {
      v = g_strdup_printf ("%d", scene->channel[n]);
      id = g_strdup_printf ("%d", n);

      xml_write_element (writer, 
                         "Value", v, 
                         "id", id, 
                         NULL);
      g_free (v);
      g_free (id);
    }

  xml_write_close_tag (writer, "Scene");
}
