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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>

#include "types.h"
#include "remote.h"
#include "main.h"
#include "main_window.h"

static GIOChannel *iochan = NULL;
static gint button_state[2];

static gboolean
remote_event (GIOChannel   *chan,
              GIOCondition  cond,
              gpointer      foo)
{
  guchar buf[0xff];
  gint state[2];
  gsize  n;

  g_io_channel_read_chars (chan, buf, sizeof (buf), &n, NULL);

  if (n < 2)
    return TRUE;

  state[0] = buf[0] & 1 ? 1:0;
  state[1] = buf[0] & 2 ? 1:0;

  if (state[0])
    sync_received = TRUE;

  if (state[1])
    sequence_back_to_sync ();

  return TRUE;
}

gboolean
remote_init (void)
{
  gint fd;

  fd = open (REMOTE_DEVICE, O_RDWR);
  if (fd < 0)
    {
      g_warning ("unable to open remote control device '%s'\n",
                 REMOTE_DEVICE);
      return FALSE;
    }

  fcntl (fd, F_SETFL, O_NONBLOCK);

  iochan = g_io_channel_unix_new (fd);

  button_state[0] = 0;
  button_state[1] = 0;

  g_io_channel_set_encoding (iochan, NULL, NULL);
  g_io_add_watch (iochan, G_IO_IN, remote_event, NULL);

  return TRUE;
}
