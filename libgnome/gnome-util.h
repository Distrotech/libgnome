#ifndef __GNOME_UTIL_H__
#define __GNOME_UTIL_H__

#include <stdlib.h>

BEGIN_GNOME_DECLS

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

char *gnome_libdir_file (char *filename);
char *gnome_datadir_file (char *filename);
char *gnome_pixmap_file (char *filename);
char *gnome_unconditional_libdir_file (char *filename);
char *gnome_unconditional_datadir_file (char *filename);
char *gnome_unconditional_pixmap_file (char *filename);
int   g_file_exists (const char *filename);
char *g_copy_strings (const char *first, ...);
const char *g_unix_error_string (int error_num);

char *g_concat_dir_and_file (const char *dir, const char *file);

/* Find out where the filename starts; note that it may actually
   be a directory, it's just the last segment of the full path. 
   Return is an index into the char array. */
int g_filename_index (const char * path);

/* returns the home directory of the user
 * This one is NOT to be free'd as it points into the 
 * env structure.
 * 
 */
#define gnome_util_user_home() getenv("HOME")

/* pass in a string, and it will add the users home dir ie,
 * pass in .gnome/bookmarks.html and it will return
 * /home/imain/.gnome/bookmarks.html
 * 
 * Remember to g_free() returned value! */
#define gnome_util_prepend_user_home(x) (g_concat_dir_and_file(gnome_util_user_home(), (x)))

/* very similar to above, but adds $HOME/.gnome/ to beginning
 * This is meant to be the most useful version.
 */
#define gnome_util_home_file(afile) (g_copy_strings(gnome_util_user_home(), "/.gnome/", (afile), NULL))

/* Find the name of the user's shell.  */
char *gnome_util_user_shell ();

END_GNOME_DECLS

#endif
