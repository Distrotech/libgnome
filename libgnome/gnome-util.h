/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Copyright (C) 1999, 2000 Red Hat, Inc.
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

#ifndef __GNOME_UTIL_H__
#define __GNOME_UTIL_H__

#include <stdlib.h>
#include <glib.h>
#include "gnome-defs.h"

BEGIN_GNOME_DECLS

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

/* Prepend the correct prefix to a filename you expect to find in the
   GNOME libdir, datadir, or pixmap directory. Return NULL if the file
   does not exist. "unconditional" versions always return a full path,
   even if the file doesn't exist. */

/* Returns the full path to a file in the specified "domain", possibly returning NULL if 'only_if_exists' is TRUE and
   the file is not found. Also see LIBGNOME_PARAM_FILE_LOCATOR and GnomeFileLocatorFunc in gnomelib-init.h */
char *gnome_file_locate (const char *domain, const char *filename, gboolean only_if_exists, GSList **ret_locations);

extern const char gnome_file_domain_libdir[];
extern const char gnome_file_domain_datadir[];
extern const char gnome_file_domain_sound[];
extern const char gnome_file_domain_pixmap[];
extern const char gnome_file_domain_config[];
extern const char gnome_file_domain_help[];

#define gnome_libdir_file(filename) gnome_file_locate(gnome_file_domain_libdir, (filename), TRUE, NULL)
#define gnome_datadir_file(filename) gnome_file_locate(gnome_file_domain_datadir, (filename), TRUE, NULL)
#define gnome_sound_file(filename) gnome_file_locate(gnome_file_domain_sound, (filename), TRUE, NULL)
#define gnome_pixmap_file(filename) gnome_file_locate(gnome_file_domain_pixmap, (filename), TRUE, NULL)
#define gnome_config_file(filename) gnome_file_locate(gnome_file_domain_config, (filename), TRUE, NULL)
#define gnome_help_file(filename) gnome_file_locate(gnome_file_domain_help, (filename), TRUE, NULL)
#define gnome_app_help_file(filename) gnome_file_locate(gnome_file_domain_help, (filename), TRUE, NULL)
#define gnome_unconditional_libdir_file(filename) gnome_file_locate(gnome_file_domain_libdir, (filename), FALSE, NULL)
#define gnome_unconditional_datadir_file(filename) gnome_file_locate(gnome_file_domain_datadir, (filename), FALSE, NULL)
#define gnome_unconditional_sound_file(filename) gnome_file_locate(gnome_file_domain_sound, (filename), FALSE, NULL)
#define gnome_unconditional_pixmap_file(filename) gnome_file_locate(gnome_file_domain_pixmap, (filename), FALSE, NULL)
#define gnome_unconditional_config_file(filename) gnome_file_locate(gnome_file_domain_config, (filename), FALSE, NULL)
#define gnome_unconditional_help_file(filename) gnome_file_locate(gnome_file_domain_config, (filename), FALSE, NULL)

gboolean g_file_exists (const char *filename);

/* locate a program in $PATH, or return NULL if not found */
char *gnome_is_program_in_path (const gchar *program);

/* Make a dir and a file into a full path, handling / issues. */
char *g_concat_dir_and_file (const char *dir, const char *file);

/* Return pointer to the character after the last .,
   or "" if no dot. */
const char * g_extension_pointer (const char * path);

/* vec has to be NULL-terminated */
char ** g_copy_vector    (const char ** vec);

/* pass in a string, and it will add the users home dir ie,
 * pass in .gnome/bookmarks.html and it will return
 * /home/imain/.gnome/bookmarks.html
 * 
 * Remember to g_free() returned value! */
#define gnome_util_prepend_user_home(x) (g_concat_dir_and_file(g_get_home_dir(), (x)))

/* very similar to above, but adds $HOME/.gnome/ to beginning
 * This is meant to be the most useful version.
 */
#define gnome_util_home_file(afile) (g_strconcat(g_get_home_dir(), "/.gnome/", (afile), NULL))

/* Find the name of the user's shell.  */
char *gnome_util_user_shell (void);

/* Return TRUE if the filename looks like an image file */
gboolean g_is_image_filename(const char * path);

END_GNOME_DECLS

#endif
