#ifndef GNOME_CONFIG_H
#define GNOME_CONFIG_H

#include <glib.h>

BEGIN_GNOME_DECLS

/* Prototypes for the profile management functions */

/*
 * DOC: gnome configuration routines.
 *
 * All of the routines receive a pathname, the pathname has the following
 * form:
 *
 *      /filename/section/key[=default]
 *
 * This format reprensents: a filename relative to the Gnome config
 * directory called filename (ie, ~/.gnome/filename), in that file there
 * is a section called [section] and key is the left handed side of the
 * values.
 *
 * If default is provided, it cane be used to return a default value
 * if none is specified on the config file.
 *
 * Examples:
 * 
 * /gmix/Balance/Ratio=0.5
 * /filemanager/Panel Display/html=1
 *
 * If the pathname starts with '=', then instead of being a ~/.gnome relative
 * file, it is an abolute pathname, example:
 *
 * =/home/miguel/.mc.ini=/Left Panel/reverse=1
 *
 * This reprensents the config file: /home/miguel/.mc.ini, section [Left Panel],
 * variable reverse.
 */

/* These functions look for the config option named in PATH.  If the
   option does not exist, and a default is specified in PATH, then the
   default will be returned.  In all cases, *DEF is set to 1 if the
   default was return, 0 otherwise.  If DEF is NULL then it will not
   be set.  */

char *gnome_config_get_string_with_default    (const char *path,
					       gboolean *def);
gint  gnome_config_get_int_with_default        (const char *path,
					       gboolean *def);
gboolean gnome_config_get_bool_with_default   (const char *path,
					       gboolean *def);
void gnome_config_get_vector_with_default     (const char *path, gint *argcp,
					       char ***argvp,
					       gboolean *def);

/* Convenience wrappers for the case when you don't care if you see
   the default.  */

#define gnome_config_get_string(Path) \
	(gnome_config_get_string_with_default ((Path), NULL))
#define gnome_config_get_int(Path) \
	(gnome_config_get_int_with_default ((Path), NULL))
#define gnome_config_get_bool(Path) \
	(gnome_config_get_bool_with_default ((Path), NULL))
#define gnome_config_get_vector(Path, Argcp, Argvp) \
        (gnome_config_get_vector_with_default ((Path), (Argcp), (Argvp), NULL))


/* Set a config variable.  */
void gnome_config_set_string     (const char *path, const char *value);
void gnome_config_set_int        (const char *path, int value);
void gnome_config_set_bool       (const char *path, gboolean value);
void gnome_config_set_vector     (const char *path,
				  int argc,
				  const char * const argv[]);

/* Returns true if /path/section is defined */
gboolean  gnome_config_has_section    (const char *path);

/* Returns a pointer for iterating on /file/section contents */
void *gnome_config_init_iterator (const char *path);

/* Returns a pointer for iterating on /file contents */
void *gnome_config_init_iterator_sections (const char *path);

/* Get next key and value value from a section */
void *gnome_config_iterator_next (void *s, char **key, char **value);

void gnome_config_drop_all       (void);

void gnome_config_sync           (void);

/* This routine drops the information about /file */
void gnome_config_clean_file     (const char *path);

/* This routine drops all of the information related to /file/section */
void gnome_config_clean_section  (const char *path);

/* Drops the information for a specific key */
void gnome_config_clean_key (const char *path);

/* Set an active prefix and remove an active prefix */
void gnome_config_push_prefix (const char *path);
void gnome_config_pop_prefix (void);

END_GNOME_DECLS

#endif
