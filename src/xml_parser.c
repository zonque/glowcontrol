/* 
 * Copyright (C) 2002  The Blinkenlights Crew
 *                     Sven Neumann <sven@gimp.org>
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
#include <stdlib.h>

#include <glib-object.h>

#include "types.h"
#include "xml_parser.h"


struct _XMLParser
{
  GMarkupParseContext *context;
  XMLParserState       state;
  XMLParserState       last_state;
  gint                 unknown_depth;
  GString             *cdata;
  gpointer             user_data;
  XMLParserStartFunc   start_element;
  XMLParserEndFunc     end_element;
};


static void    parser_start_element (GMarkupParseContext  *context,
                                     const gchar          *element_name,
                                     const gchar         **attribute_names,
                                     const gchar         **attribute_values,
                                     gpointer              user_data,
                                     GError              **error);
static void    parser_end_element   (GMarkupParseContext  *context,
                                     const gchar          *element_name,
                                     gpointer              user_data,
                                     GError              **error);
static void    parser_text          (GMarkupParseContext  *context,
                                     const gchar          *text,
                                     gsize                 text_len,
                                     gpointer              user_data,
                                     GError              **error);
static void    parser_start_unknown (XMLParser            *parser);
static void    parser_end_unknown   (XMLParser            *parser);


static const GMarkupParser markup_parser =
{
  parser_start_element,
  parser_end_element,
  parser_text,
  NULL, /* passthrough */
  NULL, /* error       */
};


XMLParser *
xml_parser_new (XMLParserStartFunc  start_element,
                XMLParserEndFunc    end_element,
                gpointer            user_data)
{
  XMLParser *parser;

  parser = g_new0 (XMLParser, 1);

  parser->context = g_markup_parse_context_new (&markup_parser,
                                                0, parser, NULL);

  parser->state         = XML_PARSER_STATE_TOPLEVEL;
  parser->cdata         = g_string_new (NULL);
  parser->user_data     = user_data;

  parser->start_element = start_element;
  parser->end_element   = end_element;

  return parser;
}

gboolean
xml_parser_parse (XMLParser    *parser,
                  const gchar  *text,
                  gssize        text_len,  
                  GError      **error)
{
  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return g_markup_parse_context_parse (parser->context, text, text_len, error);
}

gboolean
xml_parser_end_parse (XMLParser  *parser,
                      GError  **error)
{
  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return g_markup_parse_context_end_parse (parser->context, error);
}

gboolean
xml_parser_parse_io_channel (XMLParser     *parser,
                             GIOChannel  *io,
                             gboolean     recode,
                             GError     **error)
{
  GIOStatus  status;
  gchar      buffer[8196];
  gsize      len = 0;
  gsize      bytes;

  g_return_val_if_fail (parser != NULL, FALSE);
  g_return_val_if_fail (io != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (recode)
    {
      const gchar *io_encoding = g_io_channel_get_encoding (io);
      gchar       *encoding    = NULL;
      
      if (io_encoding && strcmp (io_encoding, "UTF-8"))
        {
          g_warning ("xml_parser_parse_io_channel(): "
                     "The encoding has already been set on this IOChannel!");
          return FALSE;
        }

      /* try to determine the encoding */

      while (len < sizeof (buffer) && !encoding)
        {
          status = g_io_channel_read_chars (io,
                                            buffer + len, 1, &bytes, error);
          len += bytes;

          if (status == G_IO_STATUS_ERROR)
            return FALSE;
          if (status == G_IO_STATUS_EOF)
            break;

          encoding = xml_parse_encoding (buffer, len);
        }

      if (encoding)
        {
          if (!g_io_channel_set_encoding (io, encoding, error))
            return FALSE;
     
          g_free (encoding);
        }
    }

  while (TRUE)
    {
      if (!xml_parser_parse (parser, buffer, len, error))
        return FALSE;

      status = g_io_channel_read_chars (io,
                                        buffer, sizeof(buffer), &len, error);

      switch (status)
        {
        case G_IO_STATUS_ERROR:
          return FALSE;
        case G_IO_STATUS_EOF:
          return xml_parser_end_parse (parser, error);
        case G_IO_STATUS_NORMAL:
        case G_IO_STATUS_AGAIN:
          break;
        }
    }
}

void
xml_parser_free (XMLParser *parser)
{
  g_return_if_fail (parser != NULL);

  g_markup_parse_context_free (parser->context);

  g_string_free (parser->cdata, TRUE);
  g_free (parser);
}

XMLParserState
xml_parser_get_state (XMLParser *parser)
{
  g_return_val_if_fail (parser != NULL, XML_PARSER_STATE_UNKNOWN);

  return parser->state;
}

static void
parser_start_element (GMarkupParseContext  *context,
                      const gchar          *element_name,
                      const gchar         **attribute_names,
                      const gchar         **attribute_values,
                      gpointer              user_data,
                      GError              **error)
{
  XMLParserState  new_state;
  XMLParser      *parser = (XMLParser *) user_data;

  switch (parser->state)
    {
    case XML_PARSER_STATE_TOPLEVEL:
    default:
      if (parser->start_element &&
          (new_state = parser->start_element (parser->state,
                                              element_name,
                                              attribute_names,
                                              attribute_values,
                                              parser->user_data,
                                              error)))
        {
          parser->last_state = parser->state;
          parser->state      = new_state;
          break;
        }
      /* else fallthru */
    case XML_PARSER_STATE_UNKNOWN:
      parser_start_unknown (parser);
      break;
    }

  g_string_truncate (parser->cdata, 0);
}

static void  
parser_end_element (GMarkupParseContext  *context,
                    const gchar          *element_name,
                    gpointer              user_data,
                    GError              **error)
{
  XMLParser *parser = (XMLParser *) user_data;

  switch (parser->state)
    {
    case XML_PARSER_STATE_TOPLEVEL:
      g_assert_not_reached ();
      break;

    default:
      if (parser->end_element)
        {
          gint len;

          /* strip trailing spaces */
          for (len = parser->cdata->len;
               len > 0 && g_ascii_isspace (parser->cdata->str[len-1]);
               len--)
            ; /* do nothing */
           
          g_string_truncate (parser->cdata, len);

          parser->state = parser->end_element (parser->state,
                                               element_name,
                                               parser->cdata->str,
                                               parser->cdata->len,
                                               parser->user_data,
                                               error);
          break;
        }
      /* else fallthru */
    case XML_PARSER_STATE_UNKNOWN:
      parser_end_unknown (parser);
      break;
    }

  g_string_truncate (parser->cdata, 0);
}

static void
parser_text (GMarkupParseContext  *context,
             const gchar          *text,
             gsize                 text_len,
             gpointer              user_data,
             GError              **error)
{
  XMLParser   *parser = (XMLParser *) user_data;
  gboolean     space;
  gint         i;

  space = (parser->cdata->len == 0 ||
           g_ascii_isspace (parser->cdata->str[parser->cdata->len]));

  for (i = 0; i < text_len; i++)
    {
      if (g_ascii_isspace (text[i]))
        {
          if (space)
            continue;
          space = TRUE;
        }
      else
        {
          space = FALSE;
        }

      g_string_append_c (parser->cdata, text[i]); 
    }
}

static void
parser_start_unknown (XMLParser *parser)
{
  if (parser->unknown_depth == 0)
    {
      parser->last_state = parser->state;
      parser->state = XML_PARSER_STATE_UNKNOWN;
    }

  parser->unknown_depth++;
}

static void
parser_end_unknown (XMLParser *parser)
{
  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_state;
}

gchar * 
xml_parse_encoding (const gchar *text,
                    gint         text_len)
{
  const gchar *start;
  const gchar *end;
  gint         i;

  g_return_val_if_fail (text, NULL);

  if (text_len < 20)
    return NULL;

  start = g_strstr_len (text, text_len, "<?xml");
  if (!start)
    return NULL;

  end = g_strstr_len (start, text_len - (start - text), "?>");
  if (!end)
    return NULL;
  
  text_len = end - start;
  if (text_len < 12)
    return NULL;

  start = g_strstr_len (start + 1, text_len - 1, "encoding=");
  if (!start)
    return NULL;

  start += 9;
  if (*start != '\"' && *start != '\'')
    return NULL;
  
  text_len = end - start;
  if (text_len < 1)
    return NULL;

  for (i = 1; i < text_len; i++)
    if (start[i] == start[0])
      break;

  if (i == text_len || i < 3)
    return NULL;

  return g_strndup (start + 1, i - 1);
}
