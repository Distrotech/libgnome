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
#include <libgnome/gnome-program.h>

G_BEGIN_DECLS

#define gnome_libdir_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_LIBDIR,  f, TRUE, NULL))
#define gnome_datadir_file(f) (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, f, TRUE, NULL))
#define gnome_sound_file(f)   (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_SOUND,   f, TRUE, NULL))
#define gnome_pixmap_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,  f, TRUE, NULL))
#define gnome_config_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_CONFIG,  f, TRUE, NULL))

#define gnome_unconditional_libdir_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_LIBDIR,  f, FALSE, NULL))
#define gnome_unconditional_datadir_file(f) (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, f, FALSE, NULL))
#define gnome_unconditional_sound_file(f)   (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_SOUND,   f, FALSE, NULL))
#define gnome_unconditional_pixmap_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_PIXMAP,  f, FALSE, NULL))
#define gnome_unconditional_config_file(f)  (gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_CONFIG,  f, FALSE, NULL))

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

G_END_DECLS

#endif
