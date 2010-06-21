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
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <linux/ioctl.h>
#include <linux/input.h>

#include "types.h"
#include "remote.h"
#include "main.h"
#include "main_window.h"

#define INPUT_DEVDIR "/dev/input/"
#define REMOTE_STRING "Keyspan"

static GIOChannel *iochan = NULL;

static gboolean
remote_event (GIOChannel   *chan,
              GIOCondition  cond,
              gpointer      foo)
{
  struct input_event ev;
  gsize  n;

  g_io_channel_read_chars (chan, (gchar *) &ev, sizeof (ev), &n, NULL);

  if (n < sizeof(ev))
    return TRUE;

  if ((ev.type != EV_KEY) || (ev.value != 1))
    return TRUE;

  switch (ev.code) {
    case BTN_LEFT:
      sync_received = TRUE;
      break;

    case BTN_RIGHT:
      sequence_back_to_sync ();
      break;
  }

  return TRUE;
}

gboolean
remote_init (void)
{
  const gchar *name;
  GDir *dir;
  gint fd = -1;

  dir = g_dir_open (INPUT_DEVDIR, 0, NULL);
  if (!dir)
    {
      g_warning ("unable to open %s\n", INPUT_DEVDIR);
      return FALSE;
    }

  while ((name = g_dir_read_name (dir)))
    {
      gchar tmp[100];
      gchar *fullpath = g_strconcat (INPUT_DEVDIR, G_DIR_SEPARATOR_S, name, NULL);

      if (!fullpath)
        continue;

      fd = g_open (fullpath, O_RDONLY, 0444);
      if (fd < 0)
        continue;

      g_free(fullpath);

      if (ioctl(fd, EVIOCGNAME(sizeof(tmp)), tmp) < 0)
        continue;

      if (g_strncasecmp(tmp, REMOTE_STRING, strlen(REMOTE_STRING)) == 0)
        break;

      fd = -1;
    }

  g_dir_close(dir);

  if (fd < 0)
    {
      g_warning ("unable to open remote control device\n");
      return FALSE;
    }

  fcntl (fd, F_SETFL, O_NONBLOCK);

  iochan = g_io_channel_unix_new (fd);

  g_io_channel_set_encoding (iochan, NULL, NULL);
  g_io_add_watch (iochan, G_IO_IN, remote_event, NULL);

  return TRUE;
}
