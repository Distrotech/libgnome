/*
 * Copyright (C) 1998, 1999, 2000 Red Hat Inc.
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

#ifndef __GNOME_HELP_H__
#define __GNOME_HELP_H__ 1

#include <glib.h>
#include "gnome-defs.h"

BEGIN_GNOME_DECLS

typedef struct {
    gchar *name;
    gchar *path;
} GnomeHelpMenuEntry;

gchar *gnome_help_file_find_file(gchar *app, gchar *path);

gchar *gnome_help_file_path(gchar *app, gchar *path);

void gnome_help_display(void *ignore, GnomeHelpMenuEntry *ref);

void gnome_help_pbox_display (void *ignore, gint page_num,
			      GnomeHelpMenuEntry *ref);

void gnome_help_pbox_goto (void *ignore, int ignore2, GnomeHelpMenuEntry *ref);

void gnome_help_goto(void *ignore, gchar *file);

END_GNOME_DECLS

#endif /* __GNOME_HELP_H__ */
