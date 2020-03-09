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
#include "step.h"
#include "scene.h"
#include "main_window.h"
#include "project.h"
#include "audio.h"
#include "xml_writer.h"
#include "xml_parser.h"

static void step_class_init (StepClass *class);
static void step_init              (Step      *step);
static void step_finalize   (GObject    *object);

#define RASTER 10

static GObjectClass *parent_class = NULL;

GType
step_get_type (void)
{
  static GType step_type = 0;

  if (!step_type)
    {
      static const GTypeInfo step_info =
      {
        sizeof (StepClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) step_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (Step),
        0,              /* n_preallocs */
        (GInstanceInitFunc) step_init,
      };
      
      step_type = g_type_register_static (G_TYPE_OBJECT,
                                           "Step", &step_info, 0);
    }
  
  return step_type;
}


static void
step_class_init (StepClass *class)
{
  GObjectClass *object_class;
  
  parent_class = g_type_class_peek_parent (class);
  object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = step_finalize;
}

static void
step_init (Step *step)
{
  step->project = NULL;

  step->type = STEP_TYPE_DIMM;
  step->scene = NULL;
  step->time = 0;
  step->filename = NULL;
}

static void
step_finalize (GObject *object)
{
  Step *step;

  step = STEP (object);

  if (step->filename)
    {
      g_free (step->filename);
      step->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

Step *
step_new (Project *project)
{
  Step *step;

  step = STEP (g_object_new (TYPE_STEP, NULL));
  step->project = project;

  return step;
}

gboolean
step_tick (Step *step)
{
  step->progress += RASTER;

  switch (step->type)
    {
    case STEP_TYPE_DIMM:
      {
        gint i, old, new;
        gfloat dest;
        gint chn[N_CHANNELS];
        gint val[N_CHANNELS];

        if (!step->scene)
          return FALSE;

        step->progress += N_CHANNELS/2 + 5;

        for (i = 0; i < N_CHANNELS; i++)
          {
            chn[i] = i;
            
            if (last_scene)
              old = last_scene->channel[i];
            else
              old = 0;
                        
            new = step->scene->channel[i];

            if (step->time)
              dest = old + ((new - old) * (step->progress)) / step->time;
            else
              dest = new;

            val[i] = (gint) dest;
          }

        output_channel_set (chn, val, N_CHANNELS);
        return (step->progress < step->time);
      }
    case STEP_TYPE_SLEEP:
      step->progress += 5;
      return (step->progress < step->time);
    case STEP_TYPE_SYNC:
      if (sync_received) {
        sync_received = FALSE;
        return FALSE;
      }
      return TRUE;
    case STEP_TYPE_AUDIO:
      audio_start (step->filename);
      return FALSE;

    default:
      return FALSE;
    }
}

void
step_dump (Step      *step,
           XMLWriter *writer)
{
  gchar *id, *time;

  time = g_strdup_printf ("%d", step->time);
  
  switch (step->type)
    {
    case STEP_TYPE_DIMM:
      id = g_strdup_printf ("%d", step->scene?step->scene->id:-1);
      xml_write_open_tag (writer, "Step", 
                          "type", "dimm",
                          "scene", id,
                          "time", time,
                          NULL);
      g_free (id);
      break;
    case STEP_TYPE_SLEEP:
      
      xml_write_open_tag (writer, "Step", 
                          "type", "sleep",
                          "time", time,
                          NULL);
      break;
    case STEP_TYPE_SYNC:
      xml_write_open_tag (writer, "Step", 
                          "type", "sync", 
                          NULL);
      break;
    case STEP_TYPE_AUDIO:
      xml_write_open_tag (writer, "Step", 
                          "type", "audio",
                          "filename", step->filename,
                          NULL);
      break;
    default:
      break;
    }

  g_free (time);

  xml_write_close_tag (writer, "Step");
}
