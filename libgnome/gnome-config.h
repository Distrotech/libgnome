#ifndef GNOME_CONFIG_H
#define GNOME_CONFIG_H

BEGIN_GNOME_DECLS

/* Prototypes for the profile management functions */

char *gnome_config_get_string    (char *path);
int  gnome_config_get_int        (char *path);
void gnome_config_set_string     (char *path, char *value);
void gnome_config_set_int        (char *path, int value);

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
void gnome_config_set_prefix (char *path);
void gnome_config_drop_prefix (void);

END_GNOME_DECLS

#endif
