#ifndef GNOME_CONFIG_H
#define GNOME_CONFIG_H

BEGIN_GNOME_DECLS

/* Prototypes for the profile management functions */

/* These functions look for the config option named in PATH.  If the
   option does not exist, and a default is specified in PATH, then the
   default will be returned.  In all cases, *DEF is set to 1 if the
   default was return, 0 otherwise.  If DEF is NULL then it will not
   be set.  */

char *gnome_config_get_string_with_default    (char *path, int *def);
int  gnome_config_get_int_with_default        (char *path, int *def);
int  gnome_config_get_bool_with_default       (char *path, int *def);

/* Convenience wrappers for the case when you don't care if you see
   the default.  */

#define gnome_config_get_string(Path) \
	(gnome_config_get_string_with_default ((Path), NULL))
#define gnome_config_get_int(Path) \
	(gnome_config_get_int_with_default ((Path), NULL))
#define gnome_config_get_bool(Path) \
	(gnome_config_get_bool_with_default ((Path), NULL))


/* Set a config variable.  */
void gnome_config_set_string     (char *path, char *value);
void gnome_config_set_int        (char *path, int value);
void gnome_config_set_bool       (char *path, int value);

/* Returns true if /path/section is defined */
int  gnome_config_has_section    (char *path);

/* Returns a pointer for iterating on /file/section contents */
void *gnome_config_init_iterator (char *path);

/* Get next key and value value from a section */
void *gnome_config_iterator_next (void *s, char **key, char **value);

void gnome_config_drop_all       (void);

void gnome_config_sync           (void);

/* This routine drops the information about /file */
void gnome_config_clean_file     (char *path);

/* This routine drops all of the information related to /file/section */
void gnome_config_clean_section  (char *path);

/* Drops the information for a specific key */
void gnome_config_clean_key (char *path);

/* Set an active prefix and remove an active prefix */
void gnome_config_push_prefix (char *path);
void gnome_config_pop_prefix (void);

END_GNOME_DECLS

#endif
