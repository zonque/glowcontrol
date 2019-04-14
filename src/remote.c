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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static GIOChannel *iochan = NULL;

struct remote_device
{
  const char *name;
  unsigned int sync_event;
  unsigned int back_to_sync_event;
};

struct remote_device devices[] =
{
  {
    .name = "Keyspan",
    .sync_event = BTN_LEFT,
    .back_to_sync_event = BTN_RIGHT,
  },
  {
    .name = "VEC  VEC USB Footpedal",
    .sync_event = BTN_0,
    .back_to_sync_event = BTN_2,
  },
};

static gboolean
remote_event (GIOChannel   *chan,
              GIOCondition  cond,
              gpointer      foo)
{
  struct input_event ev;
  char tmp[100];
  gsize n;
  guint i;

  g_io_channel_read_chars (chan, (gchar *) &ev, sizeof (ev), &n, NULL);

  if (n < sizeof(ev))
    return TRUE;

  if ((ev.type != EV_KEY) || (ev.value != 1))
    return TRUE;

  if (ioctl(g_io_channel_unix_get_fd (chan), EVIOCGNAME(sizeof(tmp)), tmp) < 0)
    return TRUE;

  for (i = 0; i < ARRAY_SIZE(devices); i++)
    {
      if (g_ascii_strncasecmp(tmp, devices[i].name, strlen(devices[i].name)) == 0)
        {
          if (ev.code == devices[i].sync_event)
            {
              g_print ("Sync event received from device '%s', code %d\n", tmp, ev.code);
              sync_received = TRUE;
              return TRUE;
            }

          if (ev.code == devices[i].back_to_sync_event) {
              g_print ("Back-to-sync event received from device '%s', code %d\n", tmp, ev.code);
              sequence_back_to_sync ();
              return TRUE;
          }
        }
    }

  return TRUE;
}

gboolean
remote_init (void)
{
  const gchar *name;
  GDir *dir;
  gint count = 0;

  dir = g_dir_open (INPUT_DEVDIR, 0, NULL);
  if (!dir)
    {
      g_warning ("unable to open %s\n", INPUT_DEVDIR);
      return FALSE;
    }

  while ((name = g_dir_read_name (dir)))
    {
      gint fd;
      guint i;
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

      printf ("Found input device with name >%s<\n", tmp);

      for (i = 0; i < ARRAY_SIZE(devices); i++)
        {
          if (g_ascii_strncasecmp(tmp, devices[i].name, strlen(devices[i].name)) == 0)
            {
              fcntl (fd, F_SETFL, O_NONBLOCK);

              iochan = g_io_channel_unix_new (fd);

              g_io_channel_set_encoding (iochan, NULL, NULL);
              g_io_add_watch (iochan, G_IO_IN, remote_event, NULL);

              count++;
            }
        }
    } /* while */

  g_dir_close(dir);

  if (count == 0) {
      g_warning ("unable to open any remote control device\n");
      return FALSE;
  }

  return TRUE;
}
