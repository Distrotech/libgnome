/*
 *
 * Gnome utility routines.
 * (C)  1997, 1998 the Free Software Foundation.
 *
 * Author: Miguel de Icaza, 
 */
#include <config.h>
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
gnome_dirrelative_file (char *base, char *sub, char *filename, int unconditional)
{
        static char *gnomedir = NULL;
	char *f, *t, *u, *v;
	
	/* First try the env GNOMEDIR relative path */
	if (!gnomedir)
		gnomedir = getenv ("GNOMEDIR");
	
	if (gnomedir){
		t = g_concat_dir_and_file (gnomedir, sub);
		u = g_copy_strings (t, "/", filename, NULL);
		g_free (t);
		
		if (g_file_exists (u))
			return u;

		t = g_concat_dir_and_file (gnome_util_user_home (), sub);
		v = g_copy_strings (t, "/", filename, NULL);
		g_free (t);

		if (g_file_exists (v)){
			g_free (u);
			return v;
		}

		g_free (v);
		if (unconditional)
			return u;
		
		g_free (u);
	}

	/* Then try the hardcoded path */
	f = g_concat_dir_and_file (base, filename);
	
	if (g_file_exists (f) || unconditional)
		return f;
	
	g_free (f);
	
	/* Finally, attempt to find it in the current directory */
	f = g_concat_dir_and_file (".", filename);
	
	if (g_file_exists (f))
		return f;
	
	g_free (f);
	return NULL;
}

/* DOC: gnome_libdir_file (char *filename)
 * Returns a newly allocated pathname pointing to a file in the gnome libdir
 */
char *
gnome_libdir_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, FALSE));
}

/* DOC: gnome_sharedir_file (char *filename)
 * Returns a newly allocated pathname pointing to a file in the gnome sharedir
 */
char *
gnome_datadir_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, FALSE));
}

char *
gnome_pixmap_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/pixmaps", "share/pixmaps", filename, FALSE));
}

char *
gnome_unconditional_pixmap_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/pixmaps", "share/pixmaps", filename, TRUE));
}

/* DOC: gnome_unconditional_libdir_file (char *filename)
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file in the gnome libdir
 */
char *
gnome_unconditional_libdir_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, TRUE));
}

/* DOC: gnome_unconditional_datadir_file (char *filename)
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file in the gnome libdir
 */
char *
gnome_unconditional_datadir_file (char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, TRUE));
}

/* DOC: g_file_exists (const char *filename)
 * Returns true if filename exists
 */
int
g_file_exists (const char *filename)
{
	struct stat s;
	
	return stat (filename, &s) == 0;
}


/* DOC: g_copy_strings (const char *first,...)
 * returns a new allocated char * with the concatenation of its arguments
 */
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

/* DOC: g_unix_error_string (int error_num)
 * Returns a pointer to a static location with a description of the errno
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

/* DOC: g_concat_dir_and_file (const char *dir, const char *file)
 * returns a new allocated string that is the concatenation of dir and file,
 * takes care of the exact details for concatenating them.
 */
char *
g_concat_dir_and_file (const char *dir, const char *file)
{
        /* If the directory name doesn't have a / on the end, we need
	   to add one so we get a proper path to the file */
	if (dir [strlen(dir) - 1] != PATH_SEP)
		return g_copy_strings (dir, PATH_SEP_STR, file, NULL);
	else
		return g_copy_strings (dir, file, NULL);
}

/* DOC: gnome_util_user_shell ()
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
