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
gnome_dirrelative_file (const char *base, const char *sub, const char *filename, int unconditional)
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
gnome_libdir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, FALSE));
}

/* DOC: gnome_sharedir_file (char *filename)
 * Returns a newly allocated pathname pointing to a file in the gnome sharedir
 */
char *
gnome_datadir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, FALSE));
}

char *
gnome_sound_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/sounds", "share/sounds", filename, FALSE));
}

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

char *
gnome_unconditional_pixmap_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR "/pixmaps", "share/pixmaps", filename, TRUE));
}

char *
gnome_config_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMESYSCONFDIR, "etc", filename, FALSE));
}

char *
gnome_unconditional_config_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMESYSCONFDIR, "etc", filename, TRUE));
}

/* DOC: gnome_unconditional_libdir_file (char *filename)
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file in the gnome libdir
 */
char *
gnome_unconditional_libdir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMELIBDIR, "lib", filename, TRUE));
}

/* DOC: gnome_unconditional_datadir_file (char *filename)
 * Returns a newly allocated pathname pointing to a (possibly
 * non-existent) file in the gnome libdir
 */
char *
gnome_unconditional_datadir_file (const char *filename)
{
	return (gnome_dirrelative_file (GNOMEDATADIR, "share", filename, TRUE));
}

/* DOC: g_file_test (const char *filename, int test)
 * Returns true if filename passes the specified test (an or expression of
 * tests) */
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

/* DOC: g_file_exists (const char *filename)
 * Returns true if filename exists
 * left in for binary compatibility for a while FIXME: remove
 */
#undef g_file_exists
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



int g_filename_index (const char * path)
{
  int last_path_sep;

  g_return_val_if_fail(path != NULL, 0);

  last_path_sep = strlen(path) - 2;  /* Note that the last character
					is skipped; if it's a /, we
					don't want to see it
					anyway. */

  while ( last_path_sep >= 0 ) {
    if ( path[last_path_sep] == PATH_SEP ) break;
    --last_path_sep; 
  }

  return last_path_sep + 1; /* This is 0 if -1 was reached, i.e. no
			       path separators were found. */

  /* FIXME when I wrote this I didn't know about strrchr; perhaps
     return (g_filename_pointer(path) - path); would work? */
}

const char * g_filename_pointer (const gchar * path)
{
  char * s;

  g_return_val_if_fail(path != NULL, NULL);
  
  s = strrchr(path, PATH_SEP);

  if ( s == NULL ) return path; /* There is no directory part. */
  else {
    ++s; /* skip path separator */
    return s;
  }
}

/* Code from gdk_imlib */
const char * g_extension_pointer (const char * path)
{
  char * s; 

  g_return_val_if_fail(path != NULL, NULL);
  
  s = strrchr(path, '.');

  if ( s == NULL ) return ""; /* There is no extension. */
  else {
    ++s;      /* pass the . */
    return s;
  }
}

char ** g_copy_vector (char ** vec)
{
  char ** new_vec;
  int size = 0;

  while ( vec[size] != NULL ) {
    ++size;
  }

  new_vec = g_malloc( sizeof(char *) * (size + 1) );

  size = 0;
  while ( vec[size] ) {
    new_vec[size] = g_strdup(vec[size]);
    ++size;
  }
  new_vec[size] = NULL;

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

int   g_is_image_filename(const char * path)
{
  const char * s;
  int i = 0;
  
  g_return_val_if_fail(path != NULL, FALSE);

  s = g_extension_pointer(path);

  while (image_extensions[i]) {
    if ( strcasecmp(image_extensions[i], s) == 0 ) {
      return TRUE;
    }
    ++i;
  }
  return FALSE;
}


