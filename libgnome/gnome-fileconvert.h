/*
 * Copyright (C) 1997, 1998, 1999 Elliot Lee
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

#ifndef __GNOME_FILECONVERT_H__
#define __GNOME_FILECONVERT_H__ 1

#include <glib.h>
#include <libgnome/gnome-defs.h>

/* WARNING ____ IMMATURE API ____ liable to change */

BEGIN_GNOME_DECLS

/* Returns -1 if no conversion is possible */
gint
gnome_file_convert_fd(gint fd, const char *fromtype, const char *totype);
/* Convenience wrapper for the above function */
gint
gnome_file_convert(const char *filename, const char *fromtype, const char *totype);

END_GNOME_DECLS

#endif /* __GNOME_FILECONVERT_H__ */
