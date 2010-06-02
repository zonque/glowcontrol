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

#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

G_BEGIN_DECLS

typedef enum
{
  XML_PARSER_STATE_UNKNOWN,
  XML_PARSER_STATE_TOPLEVEL,
  XML_PARSER_STATE_USER = 0x10  /* first user state, use as many as you need */
} XMLParserState;


/*  Called for open tags <foo bar="baz">, returns the new state or
    XML_PARSER_STATE_UNKNOWN if it couldn't handle the tag.  */
typedef XMLParserState (* XMLParserStartFunc) (XMLParserState   state,
                                               const gchar   *element_name,
                                               const gchar  **attribute_names,
                                               const gchar  **attribute_values, 
                                               gpointer       user_data,
                                               GError       **error);
/*  Called for close tags </foo>, returns the new state.  */
typedef XMLParserState (* XMLParserEndFunc)   (XMLParserState   state,
                                               const gchar   *element_name,
                                               const gchar   *cdata,
                                               gsize          cdata_len,
                                               gpointer       user_data,
                                               GError       **error);



XMLParser *xml_parser_new            (XMLParserStartFunc   start_element,
                                      XMLParserEndFunc     end_element,
                                      gpointer             user_data);
void     xml_parser_free             (XMLParser           *parser);

/* chunk parsing API */
gboolean xml_parser_parse            (XMLParser           *parser,
                                      const gchar         *text,
                                      gssize               text_len,
                                      GError             **error);
gboolean xml_parser_end_parse        (XMLParser           *parser,
                                      GError             **error);

/* convenience function for IO channels */
gboolean xml_parser_parse_io_channel (XMLParser           *parser,
                                      GIOChannel          *io,
                                      gboolean             recode,
                                      GError             **error);

XMLParserState xml_parser_get_state    (XMLParser         *parser);

/* parses an XML header */
gchar *  xml_parse_encoding          (const gchar         *text,
                                      gint                 text_len);

G_END_DECLS

#endif  /* __XML_PARSER_H__ */
