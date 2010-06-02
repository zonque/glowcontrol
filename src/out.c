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

#include <stdio.h>
#include <stdlib.h>
#include <usb.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "types.h"
#include "main.h"
#include "out.h"

#define        VENDOR_ID  0x0403
#define        PRODUCT_ID 0x6001


static usb_dev_handle *handle = NULL;
static gint bin_chn = 0;

/* table generated with test/acos.c */
static guchar val_table[101] = 
  {
    127, 119, 116, 113, 111, 109, 107, 105, 104, 102,
    101, 100,  98,  97,  96,  95,  94,  93,  92,  91,
     90,  89,  88,  87,  86,  85,  84,  83,  82,  81,
     80,  79,  78,  78,  77,  76,  75,  74,  73,  72, 
     72,  71,  70,  69,  68,  68,  67,  66,  65,  64, 
     64,  63,  62,  61,  60,  59,  59,  58,  57,  56, 
     55,  55,  54,  53,  52,  51,  50,  49,  49,  48, 
     47,  46,  45,  44,  43,  42,  41,  40,  39,  38,
     37,  36,  35,  34,  33,  32,  31,  30,  29,  27, 
     26,  25,  23,  22,  20,  18,  16,  14,  11,   8,  0
  };

static guchar chn_table[20] = 
  {
    0, 1,  2,  3,  4,  5,  6,  7, 100, 101,
    8, 9, 10, 11, 12, 13, 14, 15, 102, 103
  };

void 
output_channel_set (gint *chn, 
                    gint *val,
                    gint  num)
{
  guchar *buf;
  gint i, n, v, c;

  if (!handle)
    return;

  buf = g_new (guchar, num * 2);
  for (n = 0; n < num; n++)
    {
      v = val[n];
      c = chn[n];

      if (v < 0)
        v = 0;
      if (v > 100)
        v = 100;

      if (c < 0 || c >= 20)
        continue;

      c = chn_table[c];

      if (c >= 100)
        {
          if (v >= 50)
            bin_chn |= 1 << (c - 100);
          else
            bin_chn &= ~(1 << (c - 100));

          c = 0x10;
          v = bin_chn;
        }
      else
        v = val_table[v];
  
      buf[n*2]   = c | 0x80;
      buf[n*2+1] = v;
    }

  do
    {
      i = usb_bulk_write (handle, 0x2, buf, num*2, 20);
    }
  while (i != num*2);

  g_free (buf);
}


gboolean 
output_init (void)
{
  struct usb_bus *bus;
  struct usb_device *_dev, *dev = NULL;
  gint intno = 0;

  usb_init ();
  usb_find_busses ();
  usb_find_devices ();
  
  usb_set_debug (10);
  
  for (bus = usb_busses; bus; bus = bus->next)
    for (_dev = bus->devices; _dev; _dev = _dev->next)
      if (_dev->descriptor.idVendor == VENDOR_ID &&
          _dev->descriptor.idProduct == PRODUCT_ID)
        dev = _dev;
  
  if (!dev) {
    g_warning ("device not found\n");
    return FALSE;
  }
  
  g_print ("device found: %s/%s (0x%04x/0x%04x)\n", 
           dev->bus->dirname, dev->filename,
           dev->descriptor.idVendor, dev->descriptor.idProduct);
  
  handle = usb_open (dev);

  if (usb_claim_interface (handle, intno) >= 0)
    g_print ("successfully claimed interface %d\n", intno);
  else
    {
      g_warning ("failed to claim interface %d\n", intno);
      return FALSE;
    }
      
  return TRUE;
}
