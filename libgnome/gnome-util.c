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
#include "gnome-defs.h"
#include "gnome-util.h"

static char *
gnome_dirrelative_file (const char *base, const char *sub, const char *filename, int unconditional)
{
        static char *gnomedir = NULL;
	char *f = NULL, *t = NULL, *u = NULL, *v = NULL;
	char *retval = NULL;
	
	/* First try the env GNOMEDIR relative path */
	if (!gnomedir)
		gnomedir = getenv ("GNOMEDIR");
	
	if (gnomedir) {
		t = g_concat_dir_and_file (gnomedir, sub);
		u = g_strconcat (t, "/", filename, NULL);
		g_free (t); t = NULL;
		
		if (g_file_exists (u)) {
			retval = u; u = NULL; goto out;
		}

		t = g_concat_dir_and_file (gnome_util_user_home (), sub);
		v = g_strconcat (t, "/", filename, NULL);
		g_free (t); t = NULL;

		if (g_file_exists (v)){
			retval = v; v = NULL; goto out;
		}

		if (unconditional) {
			retval = u; u = NULL; goto out;
		}
		
	}

	g_free(t); t = NULL;
	if(gnomedir)
		t = g_concat_dir_and_file (gnomedir, sub);
	else
		t = g_concat_dir_and_file (gnome_util_user_home (), sub);
	if(t && strcmp(base, t)) {
		/* Then try the hardcoded path */
		f = g_concat_dir_and_file (base, filename);
	
		if (g_file_exists (f) || unconditional) {
			retval = f; f = NULL; goto out;
		}
	}
	
	/* Finally, attempt to find it in the current directory */
	g_free (f);
	f = g_concat_dir_and_file (".", filename);
	
	if (g_file_exists (f)) {
		retval = f; f = NULL; goto out;
	}

out:	
	g_free(f); g_free(t); g_free(v); g_free(u);

	return retval;
}

/**
 * gnome_libdir_file:
 * @filename: filename to locate in libdir
 *
 * Locates a shared file either in the GNOMEDIR tree, the GNOME
 * installation directory or in the current directory
 *
 * Returns a newly allocated pathname pointing to a file in the gnome libdir
 * or NULL if the file does not exist.
 */
char *
gnome_libdir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, FALSE));
}

/**
 * gnome_sharedir_file:
 * @filename: shared filename to locate
 *
 * Locates a shared file either in the GNOMEDIR tree, the GNOME
 * installation directory or in the current directory
 *
 * Returns a newly allocated pathname pointing to a file in the gnome sharedir
 * or NULL if the file does not exist.
 */
char *
gnome_datadir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, FALSE));
}

/**
 * gnome_sound_file:
 * @filename: sound filename to locate.
 *
 * Locates a sound file either in the GNOMEDIR tree, the GNOME
 * installation directory or in the current directory
 *
 * Returns a newly allocated pathname pointing to a file in the gnome sound directory
 * or NULL if the file does not exist.
 */
char *
gnome_sound_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/sounds", "share/sounds", filename, FALSE));
}

/**
 * gnome_unconditional_sound_file:
 * @filename: sound filename
 *
 * Returns a newly allocated filename from the GNOMEDIR tree or from the
 * GNOME installation directory
 */
char *
gnome_unconditional_sound_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/sounds", "share/sounds", filename, TRUE));
}

char *
gnome_pixmap_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/pixmaps", "share/pixmaps", filename, FALSE));
}

/**
 * gnome_unconditional_pixmap_file:
 * @filename: pixmap filename
 *
 * Returns a newly allocated filename from the GNOMEDIR tree or from the
 * GNOME installation directory for the pixmap directory ($prefix/share/pixmaps)
 */
char *
gnome_unconditional_pixmap_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/pixmaps", "share/pixmaps", filename, TRUE));
}

/**
 * gnome_config_file:
 * @filename: config filename
 *
 * Locates a configuration file ($prefix/etc) in the GNOMEDIR tree, the
 * GNOME installation direcory or the current directory.
 *
 * Returns a newly allocated filename from the GNOMEDIR tree or from the
 * GNOME installation directory
 */
char *
gnome_config_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMESYSCONFDIR, "etc", filename, FALSE));
}

/**
 * gnome_unconditional_config_file:
 * @filename: configuration filename
 *
 * Returns a newly allocated filename pointing to a (possibly
 * non-existent) file from the GNOMEDIR tree or from the GNOME
 * installation directory for the configuration directory
 * ($prefix/etc).
 */
char *
gnome_unconditional_config_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMESYSCONFDIR, "etc", filename, TRUE));
}

/**
 * gnome_unconditional_libdir_file:
 * @filename: library filename
 *
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file from the GNOMEDIR tree or from the GNOME
 * installation directory
 */
char *
gnome_unconditional_libdir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, TRUE));
}

/**
 * gnome_unconditional_datadir_file:
 * @filename: datadir filename
 *
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file from the GNOMEDIR tree or from the GNOME
 * installation directory
 */
char *
gnome_unconditional_datadir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, TRUE));
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
int
g_file_test (const char *filename, int test)
{
	struct stat s;
	if(stat (filename, &s) != 0)
		return FALSE;
	if(!(test & G_FILE_TEST_ISFILE) && S_ISREG(s.st_mode))
		return FALSE;
	if(!(test & G_FILE_TEST_ISLINK) && S_ISLNK(s.st_mode))
		return FALSE;
	if(!(test & G_FILE_TEST_ISDIR) && S_ISDIR(s.st_mode))
		return FALSE;
	return TRUE;
}

/**
 * g_file_exists
 * @filename: pathname to test for existance.
 *
 * Returns true if filename exists
 * left in for binary compatibility for a while FIXME: remove
 */
int
g_file_exists (const char *filename)
{
	struct stat s;
	
	return stat (filename, &s) == 0;
}


/**
 * g_copy_strings:
 * @first: first string
 *
 * returns a new allocated char * with the concatenation of its arguments,
 * the list of strings is terminated by a NULL pointer.
 *
 * NOTE: This function is deprecated.  Use GLib's g_strconcat() instead.
 */
#undef g_copy_strings
char *
g_copy_strings (const char *first, ...)
{
	va_list ap;
	int len;
	char *data, *result;
	
	if (!first)
		return 0;
	
	len = strlen (first);
	va_start (ap, first);
	
	while ((data = va_arg (ap, char *))!=0)
		len += strlen (data);
	
	len++;
	
	result = g_malloc (len);
	va_end (ap);
	va_start (ap, first);
	strcpy (result, first);
	while ((data = va_arg (ap, char *)) != 0)
		strcat (result, data);
	va_end (ap);
	
	return result;
}
#define g_copy_strings g_strconcat

/**
 * g_unix_error_string:
 * @error_num: The errno number.
 *
 * Returns a pointer to a static buffer containing the description of
 * the error reported by the errno.
 */
const gchar *
g_unix_error_string (int error_num)
{
  static GString *buffer = NULL;

  if(!buffer)
    buffer = g_string_new(NULL);

  g_string_sprintf (buffer, "%s (%d)", g_strerror (error_num), error_num);
  return buffer->str;
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
gnome_util_user_shell ()
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
		return strdup (pw->pw_shell);
	} 

	for (i = 0; shells [i]; i++) {
		if (g_file_exists (shells [i])){
			return strdup (shells[i]);
		}
	}

	/* If /bin/sh doesn't exist, your system is truly broken.  */
	abort ();

	/* Placate compiler.  */
	return NULL;
}

/**
 * g_filename_index:
 * @path: Pathname
 *
 */
int
g_filename_index (const char * path)
{
	int last_path_sep;
	
	g_return_val_if_fail(path != NULL, 0);

	/* Note that the last character
	 * is skipped; if it's a /, we
	 * don't want to see it
	 * anyway.
	 */
	last_path_sep = strlen (path) - 2;
	
	while (last_path_sep >= 0){
		if (path [last_path_sep] == PATH_SEP)
			break;
		--last_path_sep;
	}

	 /* This is 0 if -1 was reached, i.e. no path separators were found. */
	return last_path_sep + 1;

	/*
	 * FIXME when I wrote this I didn't know about strrchr; perhaps
	 * return (g_filename_pointer(path) - path); would work?
	 */
}


const char *
g_filename_pointer (const gchar * path)
{
	char * s;
	
	g_return_val_if_fail (path != NULL, NULL);
	
	s = strrchr (path, PATH_SEP);
	
	if (s == NULL)
		return path; /* There is no directory part. */
	else {
		++s; /* skip path separator */
		return s;
	}
}

/* Code from gdk_imlib */
const char *
g_extension_pointer (const char * path)
{
	char * s; 
	
	g_return_val_if_fail(path != NULL, NULL);
	
	s = strrchr(path, '.');
	
	if (s == NULL)
		return ""; /* There is no extension. */
	else {
		++s;      /* pass the . */
		return s;
	}
}

char **
g_copy_vector (char **vec)
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

/* should be in order of decreasing frequency, since
   the first ones are checked first. */

/* FIXME add more? Too many obscure ones will just slow things down.  */

static const char * const image_extensions[] = {
  "png",
  "xpm",
  "jpeg",
  "jpg",
  "gif",
  NULL
};

/*
 * Extra lame way of figuring if a filename is an image file
 */
int
g_is_image_filename (const char * path)
{
	const char * s;
	int i = 0;
	
	g_return_val_if_fail (path != NULL, FALSE);
	
	s = g_extension_pointer (path);
	
	while (image_extensions [i]) {
		if ( strcasecmp (image_extensions[i], s) == 0 ) {
			return TRUE;
		}
		++i;
	}
	return FALSE;
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
gchar *
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

