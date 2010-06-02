/* 
 * Copyright (C) 2002  The Blinkenlights Crew
 *                     Sven Neumann <sven@gimp.org>
 * 
 * Based on code written 2001 for convergence integrated media GmbH.
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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "types.h"
#include "xml_writer.h"

struct _XMLWriter
{
  FILE *stream;
  gint  indent;
  gint  indent_level;
};


static void        xml_write_attributes  (XMLWriter *writer,
                                          va_list  attributes);
static void inline xml_write_indent      (XMLWriter *writer);


XMLWriter *
xml_writer_new (FILE *stream,
                gint  indent)
{
  XMLWriter *writer;
  
  g_return_val_if_fail (stream != NULL, NULL);
  g_return_val_if_fail (indent >= 0, NULL);

  writer = g_new0 (XMLWriter, 1);

  writer->stream = stream;
  writer->indent = indent;

  return writer;
}

void
xml_writer_free (XMLWriter *writer)
{
  g_return_if_fail (writer != NULL);

  g_free (writer);
}

void
xml_write_header (XMLWriter     *writer,
                  const gchar *encoding)
{
  g_return_if_fail (writer != NULL);

  if (encoding && *encoding)
    fprintf (writer->stream,
             "<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding);
  else
    fprintf (writer->stream, "<?xml version=\"1.0\"?>\n");
}

void
xml_write_open_tag (XMLWriter     *writer,
                  const gchar *tag,
                  ...)
{
  va_list attributes;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (tag != NULL);

  va_start (attributes, tag);

  xml_write_indent (writer);

  fprintf (writer->stream, "<%s", tag);
  xml_write_attributes (writer, attributes);
  fprintf (writer->stream, ">\n");

  writer->indent_level++;

  va_end (attributes);
}

void
xml_write_close_tag (XMLWriter     *writer,
                     const gchar *tag)
{
  g_return_if_fail (writer != NULL);
  g_return_if_fail (tag != NULL);

  writer->indent_level--;
  xml_write_indent (writer);

  fprintf (writer->stream, "</%s>\n", tag);
}

void
xml_write_element (XMLWriter     *writer,
                   const gchar *tag,
                   const gchar *value,
                   ...)
{
  va_list attributes;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (tag != NULL);

  va_start (attributes, value);

  xml_write_indent (writer);

  fprintf (writer->stream, "<%s", tag);
  xml_write_attributes (writer, attributes);

  if (value)
    {
      gchar *escaped = g_markup_escape_text (value, strlen (value));
      fprintf (writer->stream, ">%s</%s>\n", escaped, tag);
      g_free (escaped);
    }
  else
    {
      fprintf (writer->stream, "/>\n");
    }

  va_end (attributes);
}


/*  private functions  */

static void
xml_write_attributes (XMLWriter *writer,
                      va_list  attributes)
{
  const gchar *name;
  const gchar *attribute;

  name = va_arg (attributes, const gchar *);

  while (name)
    {
      attribute = va_arg (attributes, const gchar *);

      fprintf (writer->stream, " %s=\"%s\"", name, attribute);
      
      name = va_arg (attributes, const gchar *);
    }
}

static const gchar *spaces = "                ";  /* 16 spaces */

static inline void
xml_write_indent (XMLWriter *writer)
{
  gint indent = writer->indent * writer->indent_level;

  while (indent > 16)
    {
      fprintf (writer->stream, spaces);
      indent -= 16;
    }
  fprintf (writer->stream, spaces + 16 - indent);
}
