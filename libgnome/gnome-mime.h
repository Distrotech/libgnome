/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef __GNOME_MIME_H__
#define __GNOME_MIME_H__

BEGIN_GNOME_DECLS

/* do not free() any of the returned values */
const char  *gnome_mime_type					(const gchar* filename);
GList       *gnome_mime_type_list				(const gchar* filename);
GList       *gnome_mime_type_list_or_default			(const gchar *filename,
								 const gchar *defaultv);
GList       *gnome_mime_type_list_of_file			(const gchar* filename);
GList       *gnome_mime_type_list_or_default_of_file		(const gchar *filename,
								 const gchar *defaultv);
const char  *gnome_mime_type_or_default				(const gchar *filename,
								 const gchar *defaultv);
const char  *gnome_mime_type_of_file				(const char *existing_filename);
const char  *gnome_mime_type_or_default_of_file			(const char *existing_filename,
								 const gchar *defaultv);
const char  *gnome_mime_type_from_magic				(const gchar *filename);

/* functions for working with uri lists */
GList       *gnome_uri_list_extract_filenames			(const gchar* uri_list);
GList       *gnome_uri_list_extract_uris			(const gchar* uri_list);
void         gnome_uri_list_free_strings			(GList *list);

/* utility function for getting a filename only from a single uri */
gchar       *gnome_uri_extract_filename                         (const gchar* uri);

END_GNOME_DECLS

#endif

