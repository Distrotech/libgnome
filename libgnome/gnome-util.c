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
  char *retval = NULL, *lastval, *dir = NULL, *prefix_rel = NULL, *envvar, *attr_name, *attr_rel;
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
 * test is one of:
 *  G_FILE_TEST_ISFILE, to check if the pathname is a file
 *  G_FILE_TEST_ISLINK, to check if the pathname is a symlink
 *  G_FILE_TEST_ISDIR,  to check if the pathname is a directory
 *
 * Returns true if filename passes the specified test (an or expression of
 * tests)
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
      if((test & G_FILE_TEST_ISFILE) && !S_ISREG(s.st_mode))
	return FALSE;
      if((test & G_FILE_TEST_ISLINK) && !S_ISLNK(s.st_mode))
	return FALSE;
      if((test & G_FILE_TEST_ISDIR) && !S_ISDIR(s.st_mode))
	return FALSE;

      return TRUE;
    }
}

/**
 * g_file_exists
 * @filename: pathname to test for existance.
 *
 * Returns true if filename exists
 * left in for binary compatibility for a while FIXME: remove
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
	/* If the directory name is NULL, return only the filename */
	if (!dir)
		return g_strdup(file);

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
		return strdup (shell);
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

	t = g_basename(path);
	s = strrchr(t, '.');
	
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
