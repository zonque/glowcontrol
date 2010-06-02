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

#include <gtk/gtk.h>

#include "types.h"
#include "main_window.h"
#include "project.h"
#include "main.h"
#include "gfx.h"
#include "out.h"
#include "remote.h"

gint
main (gint   argc, 
      gchar *argv[])
{
  GtkWidget *main_window;
  GtkWidget *dialog;

  g_type_init ();
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  
  gfx_init ();

  if (!output_init ())
    {
      dialog = gtk_message_dialog_new (NULL,
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("GlowEngine not found!"));
      
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }

  if (!remote_init ())
    {
      dialog = gtk_message_dialog_new (NULL,
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       _("Remote control device not found!!"));
      
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }

  channel_store  = gtk_list_store_new (5, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
  scene_store    = gtk_list_store_new (5, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
  sequence_store = gtk_list_store_new (5, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);

  if (argc > 1)
    project = project_new_from_file (argv[1], scene_store, sequence_store, channel_store);
  else
    project = project_new (scene_store, sequence_store, channel_store);


  last_scene = NULL;
  sync_received = FALSE;

  main_window = create_main_window ();
  gtk_widget_show (main_window);

  gtk_main ();

  return 0;
}

