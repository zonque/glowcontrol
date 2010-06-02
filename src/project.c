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
#include <stdlib.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "types.h"
#include "main.h"
#include "main_window.h"
#include "out.h"
#include "scene.h"
#include "scene_editor.h"
#include "step.h"
#include "step_editor.h"
#include "project.h"
#include "main_window.h"
#include "xml_writer.h"
#include "xml_parser.h"
#include "gfx.h"


static void project_class_init (ProjectClass *class);
static void project_init       (Project      *receiver);
static void project_finalize   (GObject      *object);

static GObjectClass *parent_class = NULL;

GType
project_get_type (void)
{
  static GType project_type = 0;

  if (!project_type)
    {
      static const GTypeInfo project_info =
      {
        sizeof (ProjectClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) project_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (Project),
        0,              /* n_preallocs */
        (GInstanceInitFunc) project_init,
      };
      
      project_type = g_type_register_static (G_TYPE_OBJECT,
                                             "Project", &project_info, 0);
    }
  
  return project_type;
}


static void
project_class_init (ProjectClass *class)
{
  GObjectClass *object_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = project_finalize;
}

static void
project_init (Project *project)
{
  project->dirty = FALSE;

  project->last_scene_id = 0;
  project->filename = NULL;

  project->scene_store = NULL;
  project->sequence_store = NULL;
  project->channel_store = NULL;

  project->step_editor = NULL;
}

static void
project_finalize (GObject *object)
{
  gchar *s;
  GObject *obj;
  Project *project = PROJECT (object);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->channel_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->channel_store), &iter,
                              COLUMN_DATA, &s,
                              -1);
          if (s)
            g_free (s);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->channel_store), &iter));
    }
  gtk_list_store_clear (project->channel_store);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->scene_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->scene_store), &iter,
                              COLUMN_DATA, &obj,
                              -1);
          if (obj)
            g_object_unref (obj);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->scene_store), &iter));
    }
  gtk_list_store_clear (project->scene_store);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->sequence_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->sequence_store), &iter,
                              COLUMN_DATA, &obj,
                              -1);
          if (obj)
            g_object_unref (obj);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->sequence_store), &iter));
    }
  gtk_list_store_clear (project->sequence_store);

  if (project->step_editor)
    g_object_unref (project->step_editor);

  if (project->filename)
    g_free (project->filename);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
load_default_channel_names (GtkListStore *store)
{
  gint i;
  GtkTreeIter iter;
  gchar *name, *id;
  
  for (i=0; i<N_CHANNELS; i++)
    {
      name = g_strdup_printf ("Channel %02d", i+1);
      id = g_strdup_printf ("%02d", i+1);

      gtk_list_store_append (store, &iter);
     
      gtk_list_store_set (store, &iter,
                          COLUMN_ID, id,
                          COLUMN_ICON, pixbuf_channel,
                          COLUMN_STRING, name,
                          COLUMN_IS_EDITABLE, GINT_TO_POINTER (TRUE),
                          -1);

      g_free (name);
      g_free (id);
    }
}


Project *
project_new (GtkListStore *scene_store,
             GtkListStore *sequence_store,
             GtkListStore *channel_store)
{
  Project *project;

  project = PROJECT (g_object_new (TYPE_PROJECT, NULL));

  project->scene_store    = scene_store;
  project->sequence_store = sequence_store;
  project->channel_store  = channel_store;

  gtk_list_store_clear (scene_store);
  gtk_list_store_clear (sequence_store);
  gtk_list_store_clear (channel_store);

  load_default_channel_names (channel_store);

  return project;
}


/* XML foo starts here. */
static guint  xml_scene_channel_count = 0;
static Scene *xml_scene = NULL;
static Step  *xml_step = NULL;
static GtkTreeIter xml_iter;

enum
{
  STATE_PROJECT = XML_PARSER_STATE_USER,
  STATE_SCENES,
  STATE_SCENE,
  STATE_SCENE_NAME,
  STATE_SCENE_ID,
  STATE_SCENE_VALUE,
  STATE_SEQUENCE,
  STATE_STEP,
  STATE_CHANNELS,
  STATE_CHANNEL,
  STATE_FINISHED
};

static XMLParserState 
project_start_element (XMLParserState   state,
                       const gchar     *element_name,
                       const gchar    **attribute_names,
                       const gchar    **attribute_values,
                       gpointer         user_data,
                       GError         **error)
{
  Project *project = PROJECT (user_data);

  switch (state)
    {
    case XML_PARSER_STATE_TOPLEVEL:
      if (strcmp (element_name, "Project") == 0)
        return STATE_PROJECT;
      break;
    case STATE_PROJECT:
      if (strcmp (element_name, "Scenes") == 0)
        return STATE_SCENES;
      if (strcmp (element_name, "Sequence") == 0)
        return STATE_SEQUENCE;
      if (strcmp (element_name, "Channels") == 0)
        {
          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->channel_store), 
                                         &xml_iter);
          return STATE_CHANNELS;
        }
      break;
    case STATE_SCENES:
      if (strcmp (element_name, "Scene") == 0)
        {
          xml_scene_channel_count = 0;
          xml_scene = scene_new (project);
          return STATE_SCENE;
        }
      break;
    case STATE_SCENE:
      if (strcmp (element_name, "Name") == 0)
        return STATE_SCENE_NAME;
      if (strcmp (element_name, "id") == 0)
        return STATE_SCENE_ID;
      if (strcmp (element_name, "Value") == 0)
        {
          const gchar **name, **val;

          name = attribute_names;
          val = attribute_values;
          
          while (*name && *val)
            {
              if (strcmp (*name, "id") == 0)
                {
                  xml_scene_channel_count = strtol (*val, NULL, 0);
                }
              else 
                return XML_PARSER_STATE_UNKNOWN;

              name++;
              val++;
            }
        }
        return STATE_SCENE_VALUE;
      break;
    case STATE_SCENE_NAME:
      break;
    case STATE_SCENE_ID:
      break;
    case STATE_SCENE_VALUE:
      
      break;
    case STATE_SEQUENCE:
      if (strcmp (element_name, "Step") == 0)
        {
          const gchar **name, **val;

          xml_step = step_new (project);

          name = attribute_names;
          val = attribute_values;
          
          while (*name && *val)
            {
              if (strcmp (*name, "type") == 0)
                {
                  if (strcmp (*val, "dimm") == 0)
                    {
                      xml_step->type = STEP_TYPE_DIMM;
                    }
                  else if (strcmp (*val, "sleep") == 0)
                    {
                      xml_step->type = STEP_TYPE_SLEEP;
                    }
                  else if (strcmp (*val, "sync") == 0)
                    {
                      xml_step->type = STEP_TYPE_SYNC;
                    }
                  else if (strcmp (*val, "audio") == 0)
                    {
                      xml_step->type = STEP_TYPE_AUDIO;
                    }
                  else
                    g_warning ("unknown step type!");
                }
              else if (strcmp (*name, "scene") == 0)
                {
                  gint id = strtol (*val, NULL, 0);
                  xml_step->scene = project_get_scene_by_id (project, id);
                }
              else if (strcmp (*name, "time") == 0)
                {
                  xml_step->time = strtol (*val, NULL, 0);
                }
              else if (strcmp (*name, "filename") == 0)
                {
                  xml_step->filename = g_strdup (*val);
                }
              else
                g_warning ("unhandled: >%s<", *name);

              name++;
              val++;
            }

          return STATE_STEP;
        }
      break;
    case STATE_STEP:
      g_printerr ("Step tags should be empty!\n");
      break;
    case STATE_CHANNELS:
      if (strcmp (element_name, "Channel") == 0)
        return STATE_CHANNEL;
      break;
    case STATE_CHANNEL:
      g_printerr ("Channel tags should be empty!\n");
      break;
    default:
      break;
    }

  return XML_PARSER_STATE_UNKNOWN;
}

static XMLParserState 
project_end_element   (XMLParserState   state,
                       const gchar     *element_name,
                       const gchar     *cdata,
                       gsize            cdata_len,
                       gpointer         user_data,
                       GError         **error)
{
  Project *project = PROJECT (user_data);

  switch (state)
    {
    case XML_PARSER_STATE_TOPLEVEL:
      break;
    case STATE_PROJECT:
      return STATE_FINISHED;
    case STATE_SCENES:
      return STATE_PROJECT;
    case STATE_SCENE:
      project_add_scene (project, xml_scene);
      xml_scene = NULL;
      return STATE_SCENES;
    case STATE_SCENE_NAME:
      if (!xml_scene)
        break;
      g_free (xml_scene->name);
      xml_scene->name = g_strdup (cdata);
      return STATE_SCENE;
    case STATE_SCENE_ID:
      if (!xml_scene)
        break;
      xml_scene->id = strtol (cdata, NULL, 0);
      return STATE_SCENE;
    case STATE_SCENE_VALUE:
      if (xml_scene_channel_count >= N_CHANNELS)
        {
          g_printerr ("too many channels!\n");
          break;
        }
      xml_scene->channel[xml_scene_channel_count] = strtol (cdata, NULL, 0);
      return STATE_SCENE;
    case STATE_SEQUENCE:
      return STATE_PROJECT;
    case STATE_STEP:
      {
        GtkTreeIter iter;
        project_add_step (project, NULL, xml_step, &iter);
        xml_step = NULL;
      }
      return STATE_SEQUENCE;
    case STATE_CHANNEL:
      gtk_list_store_set (project->channel_store, &xml_iter,
                          COLUMN_STRING, cdata,
                          -1);
      gtk_tree_model_iter_next (GTK_TREE_MODEL (project->channel_store),
                                &xml_iter);
      return STATE_CHANNELS;
    case STATE_CHANNELS:
      return STATE_PROJECT;
    default:
      break;
    }

  return XML_PARSER_STATE_UNKNOWN;
}


Project *
project_new_from_file (const gchar  *filename,
                       GtkListStore *scene_store,
                       GtkListStore *sequence_store,
                       GtkListStore *channel_store)
{
  Project *project;
  XMLParser *parser;
  GIOChannel  *io;
  gboolean retval;
  GError  **error = NULL;

  project = project_new (scene_store, sequence_store, channel_store);

  g_print ("loading from '%s'\n", filename);

  io = g_io_channel_new_file (filename, "r", error);
  if (!io)
    {
      g_print ("failed!\n");
      g_object_unref (project);
      return NULL;
    }

  parser = xml_parser_new (project_start_element, project_end_element, project);

  retval = xml_parser_parse_io_channel (parser, io, TRUE, error);

  if (retval && xml_parser_get_state (parser) != STATE_FINISHED)
    {
      g_printerr ("This does not look like a Glow Project file\n");
      /*
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
                   "This does not look like a Glow Project file");
      */
      retval = FALSE;

      g_object_unref (project);
      project = NULL;
    }

  g_io_channel_unref (io);

  xml_parser_free (parser);

  if (project)
    {
      project->dirty = FALSE;
      project->filename = g_strdup (filename);
    }

  return project;
}

gboolean
project_write_to_file (Project *project,
                       const gchar *filename)
{
  FILE *file;
  XMLWriter *writer;
  Scene *scene;
  Step  *step;
  gchar *str;
  GtkTreeIter iter;

  file = fopen (filename, "w");
  if (!file)
    {
      g_warning ("unable to write to file!");
      return FALSE;
    }

  writer = xml_writer_new (file, 2);
  
  xml_write_header (writer, "utf-8");

  xml_write_open_tag (writer, "Project", NULL);

  /* Scenes */
  xml_write_open_tag (writer, "Scenes", NULL);
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->scene_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->scene_store), &iter,
                              COLUMN_DATA, &scene,
                              -1);
          
          scene_dump (scene, writer);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->scene_store), &iter));
    }    
     
  xml_write_close_tag (writer, "Scenes");
  
  /* Sequence */
  xml_write_open_tag (writer, "Sequence", NULL);
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->sequence_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->sequence_store), &iter,
                              COLUMN_DATA, &step,
                              -1);
          
          step_dump (step, writer);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->sequence_store), &iter));
    }      

  xml_write_close_tag (writer, "Sequence");
     
  /* Channels */
  xml_write_open_tag (writer, "Channels", NULL);
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->channel_store), &iter))
    {
      do
        {
          gtk_tree_model_get (GTK_TREE_MODEL (project->channel_store), &iter,
                              COLUMN_STRING, &str,
                              -1);
          
          xml_write_element (writer, "Channel", str, NULL);
        }
      while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->channel_store), &iter));
    }

  xml_write_close_tag (writer, "Channels");
  xml_write_close_tag (writer, "Project");

  xml_writer_free (writer);
  fclose (file);

  project->dirty = FALSE;
  
  return TRUE;
}


/* Scenes */

void
project_add_scene (Project *project,
                   Scene   *scene)
{
  GtkTreeIter  iter;
  GtkTreePath *path;

  gtk_list_store_append (project->scene_store, &iter);
  gtk_list_store_set (project->scene_store, &iter,
                      COLUMN_ICON, pixbuf_scene,
                      COLUMN_STRING, scene->name,
                      COLUMN_DATA, scene,
                      -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (project->scene_store), &iter);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (project->scene_store), path, &iter);

  project->dirty = TRUE;
}

void
project_set_scene_name (Project *project,
                        Scene   *scene,
                        gchar   *name)
{
  GtkTreeIter  iter;
  GtkTreePath *path;
  Scene       *s = NULL;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->scene_store), &iter))
    return;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (project->scene_store), &iter,
                          COLUMN_DATA, &s,
                          -1);
      if (s == scene)
        break;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->scene_store), &iter));

  g_return_if_fail (s != NULL);

  if (s->name)
    g_free (s->name);

  s->name = g_strdup (name);

  gtk_list_store_set (project->scene_store, &iter,
                      COLUMN_STRING, s->name,
                      -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (project->scene_store), &iter);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (project->scene_store), path, &iter);

  if (scene->editor)
    scene_editor_set_title (scene->editor, scene->name);

  project->dirty = TRUE;
}

void
project_open_scene_editor (Project *project,
                           Scene   *scene)
{
  g_return_if_fail (IS_SCENE (scene));

  if (scene->editor)
    {
      gtk_window_present (GTK_WINDOW (scene->editor->widget));
      return;
    }

  scene->editor = scene_editor_new (scene);
}

Scene *
project_get_scene_by_id (Project *project,
                         guint    id)
{
  GtkTreeIter  iter;
  Scene       *s = NULL;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->scene_store), &iter))
    return NULL;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (project->scene_store), &iter,
                          COLUMN_DATA, &s,
                          -1);
      if (s->id == id)
        return s;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->scene_store), &iter));

  return NULL;
}

/* Sequence */

static void
set_step_entry (Project     *project,
                GtkTreeIter *iter,
                Step        *step)
{
  GtkTreePath *path;
  gchar       *str;
  GdkPixbuf   *pixbuf = NULL;

  switch (step->type)
    {
    case STEP_TYPE_DIMM:
      pixbuf = pixbuf_dimm;
      str = g_strdup_printf ("%s -> '%s' (%d ms)", _("Dimm"), 
                             (step->scene)?step->scene->name:"?", step->time);
      break;
    case STEP_TYPE_SLEEP:
      pixbuf = pixbuf_sleep;
      str = g_strdup_printf ("%s %d ms", _("Sleep"), step->time);
      break;
    case STEP_TYPE_SYNC:
      pixbuf = pixbuf_sync;
      str = g_strdup_printf ("%s", _("Sync"));
      break;
    case STEP_TYPE_AUDIO:
      pixbuf = pixbuf_audio;
      str = g_strdup_printf ("%s '%s'", _("Play audio file"), g_basename (step->filename));
      break;
    default:
      g_warning ("unknown step type!?");
      return;
    }

  gtk_list_store_set (project->sequence_store, iter,
                      COLUMN_ICON, pixbuf,
                      COLUMN_STRING, str,
                      COLUMN_DATA, step,
                      -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (project->sequence_store), iter);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (project->sequence_store), path, iter);

  g_free (str);
}

void
project_update_step (Project *project,
                     Step    *step)
{
  GtkTreeIter  iter;
  Step        *s = NULL;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (project->sequence_store), &iter))
    return;

  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (project->sequence_store), &iter,
                          COLUMN_DATA, &s,
                          -1);
      if (s == step)
        break;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (project->sequence_store), &iter));

  g_return_if_fail (s != NULL);

  set_step_entry (project, &iter, s);

  project->dirty = TRUE;
}

void
project_add_step (Project     *project,
                  GtkTreeIter *sibling,
                  Step        *step,
                  GtkTreeIter *iter)
{
  if (sibling)
    gtk_list_store_insert_after (project->sequence_store, iter, sibling);
  else
    gtk_list_store_append (project->sequence_store, iter);

  set_step_entry (project, iter, step);

  project->dirty = TRUE;
}

void
project_open_step_editor (Project *project,
                          Step    *step)
{
   g_return_if_fail (IS_STEP (step));

  if (project->step_editor)
    g_object_unref (project->step_editor);

  project->step_editor = step_editor_new (step); 
}


/* Channels */

gchar *
project_get_nth_channel_name (Project *project,
                              gint     n)
{
  GtkTreeIter iter;
  gchar *name;

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (project->channel_store),
                                 &iter, NULL, n);

  gtk_tree_model_get (GTK_TREE_MODEL (project->channel_store), &iter, 
                      COLUMN_STRING, &name, 
                      -1);
  
  return name;
}

