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

/*
 *
 * Gnome utility routines.
 * (C)  1997, 1998, 1999 the Free Software Foundation.
 *
 * Author: Miguel de Icaza, 
 */
#include <config.h>

/* needed for S_ISLNK with 'gcc -ansi -pedantic' on GNU/Linux */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif
#include <sys/types.h>

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include "libgnomeP.h"
#include <libgnomebase/gnome-defs.h>
#include <libgnomebase/gnome-program.h>
#include <libgnomebase/gnome-portability.h>
#include <libgnome/gnome-util.h>

/**
 * g_concat_dir_and_file:
 * @dir:  directory name
 * @file: filename.
 *
 * returns a new allocated string that is the concatenation of dir and file,
 * takes care of the exact details for concatenating them.
 */
char *
g_concat_dir_and_file (const char *dir, const char *file)
{
	g_return_val_if_fail (dir != NULL, NULL);
	g_return_val_if_fail (file != NULL, NULL);

        /* If the directory name doesn't have a / on the end, we need
	   to add one so we get a proper path to the file */
	if (dir[0] != '\0' && dir [strlen(dir) - 1] != PATH_SEP)
		return g_strconcat (dir, PATH_SEP_STR, file, NULL);
	else
		return g_strconcat (dir, file, NULL);
}

/**
 * gnome_util_user_shell:
 *
 * Returns a newly allocated string that is the path to the user's
 * preferred shell.
 */
char *
gnome_util_user_shell (void)
{
	struct passwd *pw;
	int i;
	const char *shell;
	static char *shells [] = {
		"/bin/bash", "/bin/zsh", "/bin/tcsh", "/bin/ksh",
		"/bin/csh", "/bin/sh", 0
	};

	if ((shell = g_getenv ("SHELL"))){
		return g_strdup (shell);
	}
	pw = getpwuid(getuid());
	if (pw && pw->pw_shell) {
		return g_strdup (pw->pw_shell);
	} 

	for (i = 0; shells [i]; i++) {
		if (g_file_test (shells [i], G_FILE_TEST_EXISTS)) {
			return g_strdup (shells[i]);
		}
	}

	/* If /bin/sh doesn't exist, your system is truly broken.  */
	abort ();

	/* Placate compiler.  */
	return NULL;
}

/**
 * g_extension_pointer:
 * @path: a filename or file path
 *
 * Returns a pointer to the extension part of the filename, or a
 * pointer to the end of the string if the filename does not
 * have an extension.
 *
 */
const char *
g_extension_pointer (const char * path)
{
	char * s, * t;
	
	g_return_val_if_fail(path != NULL, NULL);

	/* get the dot in the last element of the path */
	t = strrchr(path, G_DIR_SEPARATOR);
	if (t != NULL)
		s = strrchr(t, '.');
	else
		s = strrchr(path, '.');
	
	if (s == NULL)
		return path + strlen(path); /* There is no extension. */
	else {
		++s;      /* pass the . */
		return s;
	}
}

/**
 * g_copy_vector:
 * @vec: an array of strings.  NULL terminated
 *
 * Returns a copy of a NULL-terminated string array.
 */
char **
g_copy_vector (const char **vec)
{
	char ** new_vec;
	int size = 0;
	
	if (vec == NULL)
	    return NULL;

	while (vec [size] != NULL){
		++size;
	}
	
	new_vec = g_malloc (sizeof(char *) * (size + 1));
	
	size = 0;
	while (vec [size]){
		new_vec [size] = g_strdup (vec [size]);
		++size;
	}
	new_vec [size] = NULL;
	
	return new_vec;
}

/**
 * gnome_is_program_in_path:
 * @program: a program name.
 *
 * Looks for program in the PATH, if it is found, a g_strduped
 * string with the full path name is returned.
 *
 * Returns NULL if program is not on the path or a string 
 * allocated with g_malloc with the full path name of the program
 * found
 */
char *
gnome_is_program_in_path (const gchar *program)
{
	static gchar **paths = NULL;
	gchar **p;
	gchar *f;
	
	if (!paths)
	  paths = g_strsplit(g_getenv("PATH"), ":", -1);

	p = paths;
	while (*p){
		f = g_strconcat (*p,"/",program, NULL);
		if (g_file_test (f, G_FILE_TEST_EXISTS))
			return f;
		g_free (f);
		p++;
	}
	return 0;
}
