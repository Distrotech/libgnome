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
#include "gnome-portability.h"
#include "gnome-defs.h"
#include "gnome-util.h"
#include "gnomelib-init2.h"
#include "gnomelib-init.h"

const char gnome_file_domain_libdir[]="libdir";
const char gnome_file_domain_datadir[]="datadir";
const char gnome_file_domain_sound[]="sound";
const char gnome_file_domain_pixmap[]="pixmap";
const char gnome_file_domain_config[]="config";
const char gnome_file_domain_help[]="help";
const char gnome_file_domain_app_help[]="help";

/**
 * gnome_file_locate:
 * @domain: A 
 * @filename: A file name or path inside the 'domain' to find.
 * @only_if_exists: Only return a full pathname if the specified file actually exists
 * @ret_locations: If this is not NULL, a list of all the possible locations of the file will be returned.
 *
 * This function finds the full path to a file located in the specified "domain". A domain
 * is a name for a collection of related files. For example, common domains are "libdir", "pixmap",
 * and "config".
 *
 * The ret_locations list and its contents should be freed by the caller.
 *
 * Returns: The full path to the file (if it exists or only_if_exists is FALSE) or NULL.
 */
char *
gnome_file_locate (const char *domain, const char *filename, gboolean only_if_exists, GSList **ret_locations)
{
  char *retval = NULL, *lastval, *dir = NULL, *prefix_rel = NULL, *envvar = NULL;
  const char *attr_name = NULL, *attr_rel = NULL;
  char fnbuf[PATH_MAX];

  g_return_val_if_fail(domain, NULL);
  g_return_val_if_fail(filename, NULL);

#define ADD_FILENAME(x) { \
lastval = (x); \
if(lastval) { if(ret_locations) *ret_locations = g_slist_append(*ret_locations, lastval); \
if(!retval) retval = lastval; } \
}

  if(filename[0] == '/')
    {
      ADD_FILENAME(g_strdup(filename));
      return retval;
    }

  /* First, check our hardcoded defaults */
  switch(domain[0])
    {
    case 'l':
      if(strcmp(domain, gnome_file_domain_libdir))
	break;
      dir = GNOMELIBDIR;
      prefix_rel = "lib";
      attr_name = LIBGNOME_PARAM_APP_LIBDIR;
      attr_rel = NULL;
      break;
    case 'd':
      if(strcmp(domain, gnome_file_domain_datadir))
	break;
      dir = GNOMEDATADIR;
      prefix_rel = "share";
      attr_name = LIBGNOME_PARAM_APP_DATADIR;
      attr_rel = NULL;
      break;
    case 's':
      if(strcmp(domain, gnome_file_domain_sound))
	break;
      dir = GNOMEDATADIR "/sounds";
      prefix_rel = "share/sounds";
      attr_name = LIBGNOME_PARAM_APP_DATADIR;
      attr_rel = "sounds";
      break;
    case 'p':
      if(strcmp(domain, gnome_file_domain_pixmap))
	break;
      dir = GNOMEDATADIR "/pixmaps";
      prefix_rel = "share/pixmaps";
      attr_name = LIBGNOME_PARAM_APP_DATADIR;
      attr_rel = "pixmaps";
      break;
    case 'c':
      if(strcmp(domain, gnome_file_domain_config))
	break;
      dir = GNOMESYSCONFDIR;
      prefix_rel = "etc";
      attr_name = LIBGNOME_PARAM_APP_SYSCONFDIR;
      attr_rel = NULL;
      break;
    case 'h':
      if(strcmp(domain, gnome_file_domain_help))
	break;
      dir = GNOMEDATADIR "/gnome/help";
      prefix_rel = "share/gnome/help";
      attr_name = LIBGNOME_PARAM_APP_DATADIR;
      attr_rel = "gnome/help";
      break;
    }

  if (dir)
    {
      g_snprintf(fnbuf, sizeof(fnbuf), "%s/%s", dir, filename);

      if (!only_if_exists || g_file_exists (fnbuf))
	ADD_FILENAME(g_strdup(fnbuf));
    }
  if (retval && !ret_locations)
    goto out;

  /* Now try $GNOMEDIR */
  envvar = getenv("GNOMEDIR");
  if (envvar && prefix_rel)
    {
      int i;
      char **dirs;

      dirs = g_strsplit(envvar, ":", -1);

      for (i = 0; dirs[i] && !retval; i++)
	{
	  g_snprintf(fnbuf, sizeof(fnbuf), "%s/%s/%s", dirs[i], prefix_rel, filename);
	  if (!only_if_exists || g_file_exists (fnbuf))
	    retval = g_strdup(fnbuf);
	}

      g_strfreev(dirs);
    }
  if (retval && !ret_locations)
    goto out;

  {
    GnomeFileLocatorFunc lfunc = NULL;

    gnome_program_attributes_get(gnome_program_get(), LIBGNOME_PARAM_FILE_LOCATOR, &lfunc, NULL);

    if (lfunc)
      {
	ADD_FILENAME(lfunc(domain, filename, only_if_exists));
      }
  }
  if (retval && !ret_locations)
    goto out;

  if (prefix_rel) {
    char *app_prefix = NULL, *app_dir = NULL;

    gnome_program_attributes_get(gnome_program_get(), attr_name, &app_dir, NULL);

    if(!app_dir)
      {
	gnome_program_attributes_get(gnome_program_get(), LIBGNOME_PARAM_APP_PREFIX, &app_prefix, NULL);

	if(!app_prefix)
	  goto out;

	sprintf(fnbuf, "%s/%s/%s", app_prefix, prefix_rel, filename);
      }
    else
      {
	sprintf(fnbuf, "%s/%s%s%s", app_dir, attr_rel?attr_rel:"", attr_rel?"/":"", filename);
      }

    if (!only_if_exists || g_file_exists (fnbuf))
      ADD_FILENAME(g_strdup(fnbuf));
  }

 out:
  return retval;
}

/**
 * g_file_test:
 * @filename:  filename to test
 * @test:      test to perform on the file
 *
 * Description: The test should be an ORed expression of:
 * G_FILE_TEST_ISFILE to check if the pathname is a file,
 * G_FILE_TEST_ISLINK to check if the pathname is a symlink and/or
 * G_FILE_TEST_ISDIR to check if the pathname is a directory.
 * Alternatively, it can just be G_FILE_TEXT_EXISTS to simply test
 * for existance of any type of file.
 *
 * Returns: %TRUE if filename passes the specified test, if the test
 * is an ORed expression, then if it passes at least one of those tests
 */
gboolean
g_file_test (const char *filename, int test)
{
  if( (test & G_FILE_TEST_EXISTS) == G_FILE_TEST_EXISTS )
    {
      return (access(filename, F_OK)==0);
    }
  else
    {
      struct stat s;
      
      if(stat (filename, &s) != 0)
	return FALSE;
      /* this should test if the file is at least one of those things 
	 specified */
      if((!(test & G_FILE_TEST_ISFILE) || !S_ISREG(s.st_mode)) &&
	 (!(test & G_FILE_TEST_ISLINK) || !S_ISLNK(s.st_mode)) &&
	 (!(test & G_FILE_TEST_ISDIR) || !S_ISDIR(s.st_mode)))
	return FALSE;

      return TRUE;
    }
}

/**
 * g_file_exists
 * @filename: pathname to test for existance.
 *
 * Description:  Checks if a file exists or not.
 *
 * Returns: %TRUE if filename exists, %FALSE otherwise
 */
gboolean
g_file_exists (const char *filename)
{
	g_return_val_if_fail (filename != NULL, FALSE);
	
	return (access (filename, F_OK) == 0);
}

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
	if (dir [strlen(dir) - 1] != PATH_SEP)
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
	char *shell;
	static char *shells [] = {
		"/bin/bash", "/bin/zsh", "/bin/tcsh", "/bin/ksh",
		"/bin/csh", "/bin/sh", 0
	};

	if ((shell = getenv ("SHELL"))){
		return g_strdup (shell);
	}
	pw = getpwuid(getuid());
	if (pw && pw->pw_shell) {
		return g_strdup (pw->pw_shell);
	} 

	for (i = 0; shells [i]; i++) {
		if (g_file_exists (shells [i])){
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
	  paths = g_strsplit(getenv("PATH"), ":", -1);

	p = paths;
	while (*p){
		f = g_strconcat (*p,"/",program, NULL);
		if (g_file_exists (f))
			return f;
		g_free (f);
		p++;
	}
	return 0;
}
