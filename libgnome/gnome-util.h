#ifndef __GNOME_UTIL_H__
#define __GNOME_UTIL_H__

BEGIN_GNOME_DECLS

#define PATH_SEP '/'
#define PATH_SEP_STR "/"

char *gnome_libdir_file (char *filename);
char *gnome_datadir_file (char *filename);
char *gnome_pixmap_file (char *filename);
char *gnome_unconditional_libdir_file (char *filename);
char *gnome_unconditional_datadir_file (char *filename);
char *gnome_unconditional_pixmap_file (char *filename);
int   g_file_exists (char *filename);
char *g_copy_strings (const char *first, ...);
char *g_unix_error_string (int error_num);
char *g_concat_dir_and_file (const char *dir, const char *file);
char *gnome_util_user_home(void);
char *gnome_util_prepend_user_home(char *file);
char *gnome_util_home_file(char *file);

END_GNOME_DECLS

#endif
