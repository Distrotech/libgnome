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

#ifndef __GNOME_MAGIC_H__
#define __GNOME_MAGIC_H__

/*
 * Do not include this file in libgnome.h as it is of little use for
 * other applications.
 *
 * If you are only interested inthe gnome_mime_type_from_magic, see
 * the gnome-mime.h header file.
 */
BEGIN_GNOME_DECLS

/* Used internally by the magic code, exposes the parsing to users */
typedef enum {
	T_END, /* end of array */
	T_BYTE,
	T_SHORT,
	T_LONG,
	T_STR,
	T_DATE, 
	T_BESHORT,
	T_BELONG,
	T_BEDATE,
	T_LESHORT,
	T_LELONG,
	T_LEDATE
} GnomeMagicType;

typedef struct _GnomeMagicEntry {
	guint32 mask;
	GnomeMagicType type;
	guint16 offset, level;
	
	char test[48];
	guchar test_len;
	enum { CHECK_EQUAL, CHECK_LT, CHECK_GT, CHECK_AND, CHECK_XOR,
	       CHECK_ANY } comptype;
	guint32 compval;
	
	char mimetype[48];
} GnomeMagicEntry;

GnomeMagicEntry *gnome_magic_parse(const gchar *filename, gint *nents);

END_GNOME_DECLS

#endif
