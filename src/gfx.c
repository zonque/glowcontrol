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

#include <glib.h>
#include <gtk/gtk.h>

#include "gfx.h"

static gchar *search_path[] = {
  "../pixmaps",
  XPMDIR,
  NULL
};

GdkPixbuf *
pixbuf_new (const gchar *filename)
{
  gchar **p, *fn;
  GdkPixbuf *pix;

  for (p = search_path; p && *p; p++)
    {
      fn = g_build_filename (*p, filename, NULL);

      if (g_file_test (fn, G_FILE_TEST_EXISTS))
        {
          pix = gdk_pixbuf_new_from_file (fn, NULL);
          g_free (fn);
          return pix;
        }
      g_free (fn);
    }
  return NULL;
}


gboolean 
gfx_init (void)
{
  pixbuf_scene   = pixbuf_new ("scene.xpm");
  pixbuf_channel = pixbuf_new ("channel.xpm");
  pixbuf_dimm    = pixbuf_new ("dimm.xpm");
  pixbuf_sleep   = pixbuf_new ("sleep.xpm");
  pixbuf_sync    = pixbuf_new ("sync.xpm");
  pixbuf_audio   = pixbuf_new ("audio.xpm");

  return TRUE;
}

