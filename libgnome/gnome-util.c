#include <config.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/stat.h>
#include "gnome-defs.h"
#include "gnome-util.h"

static char *
gnome_dirrelative_file (char *base, char *sub, char *filename, int unconditional)
{
        static char *gnomedir = NULL;
	char *f, *t, *u;
	
	/* First try the env GNOMEDIR relative path */
	if (!gnomedir)
		gnomedir = getenv ("GNOMEDIR");
	
	if (gnomedir){
		t = g_concat_dir_and_file (gnomedir, sub);
		u = g_copy_strings (t, "/", filename, NULL);
		g_free (t);
		
		if (g_file_exists (u) || unconditional)
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

/* DOC: g_file_exists (char *filename)
 * Returns true if filename exists
 */
int
g_file_exists (char *filename)
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
		return NULL;
	
	len = strlen (first);
	va_start (ap, first);
	
	while ((data = va_arg (ap, char *)) != NULL)
		len += strlen (data);
	
	len++;
	
	result = (char *) g_malloc (len);
	va_end (ap);
	va_start (ap, first);
	strcpy (result, first);
	while ((data = va_arg (ap, char *)) != NULL)
		strcat (result, data);
	va_end (ap);
	
	return result;
}


/* DOC: g_unix_error_string (int error_num)
 * Returns a pointer to a static location with a description of the errno
 */
char *
g_unix_error_string (int error_num)
{
	static char buffer [256];
	sprintf (buffer, "%s (%d)", g_strerror (error_num), error_num);
	return buffer;
}


/* DOC: g_concat_dir_and_file (const char *dir, const char *file)
 * returns a new allocated string that is the concatenation of dir and file,
 * takes care of the exact details for concatenating them.
 */
char *
g_concat_dir_and_file (const char *dir, const char *file)
{
	int l = strlen (dir);
	
	if (dir [l - 1] == PATH_SEP)
		return g_copy_strings (dir, file, NULL);
	else
		return g_copy_strings (dir, PATH_SEP_STR, file, NULL);
}


/* returns the home directory of the user
 * This one is NOT to be free'd as it points into the 
 * env structure.
 * 
 */

char *
gnome_util_user_home(void)
{
	static char *home_dir;
	static int init = 1;
	
	if (init) {
		home_dir = getenv("HOME");
		init = 0;
	}
	
	return(home_dir);
}

/* pass in a string, and it will add the users home dir ie,
 * pass in .gnome/bookmarks.html and it will return
 * /home/imain/.gnome/bookmarks.html
 * 
 * Remember to g_free() returned value! */

char *
gnome_util_prepend_user_home(char *file)
{
	return g_concat_dir_and_file(gnome_util_user_home(), file);
}

/* very similar to above, but adds $HOME/.gnome/ to beginning
 * This is meant to be the most useful version.
 */

char *
gnome_util_home_file(char *file)
{
	char *path = g_concat_dir_and_file(gnome_util_user_home(), ".gnome");
	char *ret = g_copy_strings(path, "/", file, NULL);

	g_free(path);
	return ret;
}
