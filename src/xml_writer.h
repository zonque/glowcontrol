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

#ifndef __XML_WRITER_H__
#define __XML_WRITER_H__

#include <stdio.h>

G_BEGIN_DECLS


XMLWriter * xml_writer_new      (FILE        *stream,
                                 gint         indent);
void        xml_writer_free     (XMLWriter   *writer);

void        xml_write_header    (XMLWriter   *writer,
                                 const gchar *encoding);
void        xml_write_open_tag  (XMLWriter   *writer,
                                 const gchar *tag,
                                 ...);
void        xml_write_close_tag (XMLWriter   *writer,
                                 const gchar *tag);
void        xml_write_element   (XMLWriter   *writer,
                                 const gchar *tag,
                                 const gchar *value,
                                 ...);

G_END_DECLS

#endif  /* __XML_WRITER_H__ */
