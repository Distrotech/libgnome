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
	char *dir = NULL, *fil = NULL, *odir = NULL, *ofil = NULL;
	char *retval = NULL;
	
	/* First try the env GNOMEDIR relative path */
	if (!gnomedir)
		gnomedir = getenv ("GNOMEDIR");
	
	if (gnomedir) {
		dir = g_concat_dir_and_file (gnomedir, sub);
		fil = g_concat_dir_and_file (dir, filename);

		if (g_file_exists (fil)) {
			retval = fil; fil = NULL; goto out;
		}

		odir = dir; ofil = fil;
		dir = g_concat_dir_and_file (gnome_util_user_home (), sub);
		fil = g_concat_dir_and_file (dir, filename);

		if (strcmp (odir, dir) != 0 && g_file_exists (fil)) {
			retval = fil; fil = NULL; goto out;
		}

		if (unconditional) {
			retval = ofil; ofil = NULL; goto out;
		}
	}

	if ((!dir || strcmp (base, dir) != 0)
	    && (!odir || strcmp (base, odir) != 0)) {
		/* Then try the hardcoded path */
		g_free (fil);
		fil = g_concat_dir_and_file (base, filename);
		
		if (unconditional || g_file_exists (fil)) {
			retval = fil; fil = NULL; goto out;
		}
	}

	/* Finally, attempt to find it in the current directory */
	g_free (fil);
	fil = g_concat_dir_and_file (".", filename);
	
	if (g_file_exists (fil)) {
		retval = fil; fil = NULL; goto out;
	}

out:	
	g_free (dir); g_free (odir); g_free (fil); g_free (ofil);

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
 * gnome_datadir_file:
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

/**
 * gnome_pixmap_file:
 * @filename: pixmap filename
 *
 * Returns a newly allocated filename from the GNOMEDIR tree or from the
 * GNOME installation directory for the pixmap directory ($prefix/share/pixmaps),
 * or NULL if the file does not exist.
 */
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

	g_return_val_if_fail (filename != NULL,FALSE);
	
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
#if 0
char *
g_copy_strings (const char *first, ...)
{
	va_list ap;
	int len;
	char *data, *result;
	static int warned = 0;

	if(!warned) {
	  g_warning("\ng_copy_strings is about to disappear. Please recompile your apps. <<----");
	  warned = 1;
	}

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
#endif

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


/**
 * g_is_image_filename:
 * @path: Filename or file path.
 *
 * Deprecated. Extra lame way of figuring if a filename is an image file.  You
 * should use the gnome_mime functions instead and match against "image/".
 *
 * Returns: TRUE if the filename is an image.
 */
gboolean
g_is_image_filename (const char *path)
{
	const char * s;
	int i = 0;
	
	g_return_val_if_fail (path != NULL, FALSE);

	{
		static int warn_shown = 0;

		if (!warn_shown){
			warn_shown = 1;
			g_warning ("g_is_image_filename called, you "
				   "should use gnome-mime instead\n");
		}
	}
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

