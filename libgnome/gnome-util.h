#ifndef __GNOME_UTIL_H__
#define __GNOME_UTIL_H__

#include <stdlib.h>
#include <glib.h>

BEGIN_GNOME_DECLS

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

/* Prepend the correct prefix to a filename you expect to find in the
   GNOME libdir, datadir, or pixmap directory. Return NULL if the file
   does not exist. "unconditional" versions always return a full path,
   even if the file doesn't exist. */

char *gnome_libdir_file (const char *filename); 
char *gnome_datadir_file (const char *filename);
char *gnome_sound_file (const char *filename); 
char *gnome_pixmap_file (const char *filename); 
char *gnome_config_file (const char *filename);
char *gnome_unconditional_libdir_file (const char *filename); 
char *gnome_unconditional_datadir_file (const char *filename); 
char *gnome_unconditional_sound_file (const char *filename); 
char *gnome_unconditional_pixmap_file (const char *filename); 
char *gnome_unconditional_config_file (const char *filename);

enum {
	G_FILE_TEST_EXISTS=(1<<0)|(1<<1)|(1<<2), /*any type of file*/
	G_FILE_TEST_ISFILE=1<<0,
	G_FILE_TEST_ISLINK=1<<1,
	G_FILE_TEST_ISDIR=1<<2
};

int g_file_test   (const char *filename, int test);
int g_file_exists (const char *filename);

/* locate a program in $PATH, or return NULL if not found */
gchar *gnome_is_program_in_path (const gchar *program);

/* g_copy_strings superceded by GLib's g_strconcat */
#define g_copy_strings g_strconcat

/* like strerror() */
const char *g_unix_error_string (int error_num);

/* Make a dir and a file into a full path, handling / issues. */
char *g_concat_dir_and_file (const char *dir, const char *file);

/* Find out where the filename starts; note that it may actually
   be a directory, it's just the last segment of the full path. 
   Return is an index into the char array. */
/* int g_filename_index       (const char * path); */
#define g_filename_index(path) (g_basename(path)-(path))
/* Pointer to the start of the filename */ 
/* const char * g_filename_pointer  (const char * path); */
#define g_filename_pointer g_basename

/* Return pointer to the character after the last .,
   or "" if no dot. */
const char * g_extension_pointer (const char * path);

/* vec has to be NULL-terminated */
char ** g_copy_vector    (char ** vec);
/* separator can be NULL */
#define g_flatten_vector g_strjoinv

/* returns the home directory of the user
 * This one is NOT to be free'd or changed.
 * 
 */
#define gnome_util_user_home() g_get_home_dir()

/* pass in a string, and it will add the users home dir ie,
 * pass in .gnome/bookmarks.html and it will return
 * /home/imain/.gnome/bookmarks.html
 * 
 * Remember to g_free() returned value! */
#define gnome_util_prepend_user_home(x) (g_concat_dir_and_file(gnome_util_user_home(), (x)))

/* very similar to above, but adds $HOME/.gnome/ to beginning
 * This is meant to be the most useful version.
 */
#define gnome_util_home_file(afile) (g_strconcat(gnome_util_user_home(), "/.gnome/", (afile), NULL))

/* Find the name of the user's shell.  */
char *gnome_util_user_shell (void);

/* Return TRUE if the filename looks like an image file */
gboolean g_is_image_filename(const char * path);

END_GNOME_DECLS

#endif
