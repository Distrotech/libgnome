/* gnome-history.h - Keep history about file visitations.
   Copyright (C) 1998 Elliot Lee

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef __GNOME_HISTORY_H__
#define __GNOME_HISTORY_H__ 1

/* WARNING ____ IMMATURE API ____ liable to change */

BEGIN_GNOME_DECLS

/* One of these structures is created for each file in the history.  */
struct _GnomeHistoryEntry
{
	char *filename;		/* Name of the visited file. */
	char *filetype;		/* MIME type of the visited file.  */
	char *creator;		/* What program created the file.  */
	char *desc;		/* Description of what choosing this
				   item would do.  This is some
				   explanatory text that might be
				   presented to the user.  */
};

typedef struct _GnomeHistoryEntry * GnomeHistoryEntry;

/* This function registers a new history entry.  All arguments are
   copied internally.  */
void
gnome_history_recently_used (char *filename, char *filetype,
			     char *creator, char *desc);

/* Return a list of all the recently used files we know about.  Each
   element of the GList is a GnomeHistoryEntry.  The list is freshly
   allocated, and should be freed with
   gnome_history_free_recently_used_list.  */
GList *
gnome_history_get_recently_used (void);

/* Free a history list as returned by
   gnome_history_get_recently_used.  */
void
gnome_history_free_recently_used_list (GList *alist);

END_GNOME_DECLS

#endif /* __GNOME_HISTORY_H__ */
