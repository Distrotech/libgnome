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

#ifndef __GNOME_DENTRY_H__
#define __GNOME_DENTRY_H__

BEGIN_GNOME_DECLS


typedef struct {
	char *name;		/* These contain the translated name/comment */
	char *comment;
	int exec_length;	/* Length of `exec' vector.  Does not
				   include the NULL terminator.  */
	char **exec;		/* Command to execute.  Must be NULL
				   terminated.  */
	char *tryexec;		/* Test program to execute */
	char *icon;		/* Icon name */
	char *docpath;		/* Path to the documentation */
	int   terminal;		/* flag: requires a terminal to run */
	char *type;		/* type of this dentry */
	char *location;		/* path of this dentry */
	char *geometry;		/* geometry for this icon */
	
	unsigned int multiple_args:1;
	unsigned int is_kde:1;  /* If this is a .kdelink file */
} GnomeDesktopEntry;

GnomeDesktopEntry *gnome_desktop_entry_load (const char *file);
GnomeDesktopEntry *gnome_desktop_entry_load_flags (const char *file, gboolean clean_from_memory_after_load);
GnomeDesktopEntry *gnome_desktop_entry_load_flags_conditional (const char *file, gboolean clean_from_memory_after_load, gboolean unconditional);
GnomeDesktopEntry *gnome_desktop_entry_load_unconditional (const char *file);
void gnome_desktop_entry_save (GnomeDesktopEntry *dentry);
void gnome_desktop_entry_free (GnomeDesktopEntry *item);
void gnome_desktop_entry_destroy (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch_with_args (GnomeDesktopEntry *item, int the_argc, char *the_argv[]);

GnomeDesktopEntry *gnome_desktop_entry_copy (GnomeDesktopEntry * source);

/*hacks to make the i18n thing working*/

typedef struct _GnomeDesktopEntryI18N GnomeDesktopEntryI18N;
struct _GnomeDesktopEntryI18N {
	char *lang;
	char *name;
	char *comment;
};

GList * gnome_desktop_entry_get_i18n_list (GnomeDesktopEntry *item);
void gnome_desktop_entry_set_i18n_list (GnomeDesktopEntry *item, GList *list);
void gnome_desktop_entry_free_i18n_list (GList *list);

END_GNOME_DECLS

#endif
