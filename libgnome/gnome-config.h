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

/*use the wrappers below*/
char *_gnome_config_get_string_with_default    (const char *path,
					        gboolean *def,
						gint priv);
char *_gnome_config_get_translated_string_with_default(const char *path,
						       gboolean *def,
						       gint priv);
gint  _gnome_config_get_int_with_default       (const char *path,
					        gboolean *def,
						gint priv);
gboolean _gnome_config_get_bool_with_default   (const char *path,
					        gboolean *def,
					        gint priv);
void _gnome_config_get_vector_with_default     (const char *path, gint *argcp,
					        char ***argvp,
					        gboolean *def,
					        gint priv);

/*these just call the above functions, but devide them into two groups,
  in the future these may be different functions, so use these defines*/
/*normal functions*/
#define gnome_config_get_string_with_default(Path,Def) \
	(_gnome_config_get_string_with_default((Path),(Def),FALSE))
#define gnome_config_get_translated_string_with_default(Path,Def) \
	(_gnome_config_get_translated_string_with_default((Path),(Def),FALSE))
#define gnome_config_get_int_with_default(Path,Def) \
	(_gnome_config_get_int_with_default((Path),(Def),FALSE))
#define gnome_config_get_bool_with_default(Path,Def) \
	(_gnome_config_get_bool_with_default((Path),(Def),FALSE))
#define gnome_config_get_vector_with_default(Path, Argcp, Argvp, Def) \
        (_gnome_config_get_vector_with_default ((Path),(Argcp),(Argvp), \
						(Def),FALSE))

/*private functions*/
#define gnome_config_private_get_string_with_default(Path,Def) \
	(_gnome_config_get_string_with_default((Path),(Def),TRUE))
#define gnome_config_private_get_translated_string_with_default(Path,Def) \
	(_gnome_config_get_translated_string_with_default((Path), (Def),TRUE))
#define gnome_config_private_get_int_with_default(Path,Def) \
	(_gnome_config_get_int_with_default((Path),(Def),TRUE))
#define gnome_config_private_get_bool_with_default(Path,Def) \
	(_gnome_config_get_bool_with_default((Path),(Def),TRUE))
#define gnome_config_private_get_vector_with_default(Path, Argcp, Argvp, Def) \
        (_gnome_config_get_vector_with_default ((Path),(Argcp), (Argvp), \
        					(Def), TRUE))

/* Convenience wrappers for the case when you don't care if you see
   the default.  */
/*normal functions*/
#define gnome_config_get_string(Path) \
	(_gnome_config_get_string_with_default ((Path), NULL, FALSE))
#define gnome_config_get_translated_string(Path) \
	(_gnome_config_get_translated_string_with_default ((Path), NULL, FALSE))
#define gnome_config_get_int(Path) \
	(_gnome_config_get_int_with_default ((Path), NULL, FALSE))
#define gnome_config_get_bool(Path) \
	(_gnome_config_get_bool_with_default ((Path), NULL, FALSE))
#define gnome_config_get_vector(Path, Argcp, Argvp) \
        (_gnome_config_get_vector_with_default ((Path), (Argcp), (Argvp), \
        					NULL, FALSE))

/*private functions*/
#define gnome_config_private_get_string(Path) \
	(_gnome_config_get_string_with_default ((Path), NULL, TRUE))
#define gnome_config_private_get_translated_string(Path) \
	(_gnome_config_get_translated_string_with_default ((Path), NULL, TRUE))
#define gnome_config_private_get_int(Path) \
	(_gnome_config_get_int_with_default ((Path), NULL, TRUE))
#define gnome_config_private_get_bool(Path) \
	(_gnome_config_get_bool_with_default ((Path), NULL, TRUE))
#define gnome_config_private_get_vector(Path, Argcp, Argvp) \
        (_gnome_config_get_vector_with_default ((Path), (Argcp), (Argvp), \
        					NULL, TRUE))

/* Set a config variable.  Use the warppers below*/
void _gnome_config_set_string     (const char *path, const char *value,
				   gint priv);
void _gnome_config_set_translated_string (const char *path, const char *value,
					  gint priv);
void _gnome_config_set_int        (const char *path, int value,
				   gint priv);
void _gnome_config_set_bool       (const char *path, gboolean value,
				   gint priv);
void _gnome_config_set_vector     (const char *path,
				   int argc,
				   const char * const argv[],
				   gint priv);


/* normal functions */
#define gnome_config_set_string(Path,Val) \
	(_gnome_config_set_string((Path),(Val),FALSE))
#define gnome_config_set_translated_string(Path,Val) \
	(_gnome_config_set_translated_string((Path),(Val),FALSE))
#define gnome_config_set_int(Path,Val) \
	(_gnome_config_set_int((Path),(Val),FALSE))
#define gnome_config_set_bool(Path,Val) \
	(_gnome_config_set_bool((Path),(Val),FALSE))
#define gnome_config_set_vector(Path,Argc,Argv) \
	(_gnome_config_set_vector((Path),(Argc),(Argv),FALSE))

/* private functions */
#define gnome_config_private_set_string(Path,Val) \
	(_gnome_config_set_string((Path),(Val),TRUE))
#define gnome_config_private_set_translated_string(Path,Val) \
	(_gnome_config_set_translated_string((Path),(Val),TRUE))
#define gnome_config_private_set_int(Path,Val) \
	(_gnome_config_set_int((Path),(Val),TRUE))
#define gnome_config_private_set_bool(Path,Val) \
	(_gnome_config_set_bool((Path),(Val),TRUE))
#define gnome_config_private_set_vector(Path,Argc,Argv) \
	(_gnome_config_set_vector((Path),(Argc),(Argv),TRUE))

/* Returns true if /path/section is defined */
gboolean  _gnome_config_has_section    (const char *path, gint priv);
#define gnome_config_has_section(Path) \
	(_gnome_config_has_section((Path),FALSE))
#define gnome_config_private_has_section(Path) \
	(_gnome_config_has_section((Path),TRUE))

/* Returns a pointer for iterating on /file/section contents */
void *_gnome_config_init_iterator (const char *path, gint priv);
#define gnome_config_init_iterator(Path) \
	(_gnome_config_init_iterator((Path),FALSE))
#define gnome_config_private_init_iterator(Path) \
	(_gnome_config_init_iterator((Path),TRUE))

/* Returns a pointer for iterating on /file contents */
void *_gnome_config_init_iterator_sections (const char *path, gint priv);
#define gnome_config_init_iterator_sections(Path) \
	(_gnome_config_init_iterator_sections((Path),FALSE))
#define gnome_config_private_init_iterator_sections(Path) \
	(_gnome_config_init_iterator_sections((Path),TRUE))

/* Get next key and value value from a section */
void *gnome_config_iterator_next (void *s, char **key, char **value);

void gnome_config_drop_all       (void);

void gnome_config_sync           (void);

/* This routine drops the information about /file */
void _gnome_config_clean_file     (const char *path, gint priv);
#define gnome_config_clean_file(Path) \
	(_gnome_config_clean_file((Path),FALSE))
#define gnome_config_private_clean_file(Path) \
	(_gnome_config_clean_file((Path),TRUE))

/* This routine drops all of the information related to /file/section */
void _gnome_config_clean_section  (const char *path, gint priv);
#define gnome_config_clean_section(Path) \
	(_gnome_config_clean_section((Path),FALSE))
#define gnome_config_private_clean_section(Path) \
	(_gnome_config_clean_section((Path),TRUE))

/* Drops the information for a specific key */
void _gnome_config_clean_key (const char *path, gint priv);
#define gnome_config_clean_key(Path) \
	(_gnome_config_clean_key((Path),FALSE))
#define gnome_config_private_clean_key(Path) \
	(_gnome_config_clean_key((Path),TRUE))

/* returns the true filename of the config file */
#define gnome_config_get_real_path(Path) \
	(g_concat_dir_and_file (gnome_user_dir,(Path)))
#define gnome_config_private_get_real_path(Path) \
	(g_concat_dir_and_file (gnome_user_private_dir,(Path)))

/* Set an active prefix and remove an active prefix */
void gnome_config_push_prefix (const char *path);
void gnome_config_pop_prefix (void);

/*these two are for removing the prefix list and then setting it back,
  it would be usefull in situations that you need to specify a different
  path, but don't want to destroy the prefix list used by the rest of the
  app, you should pass the same pointer you get from remove to set and the
  two should be paired. _set_ function will also pop any path you have pushed
  after the _remove_ call*/
GSList *gnome_config_remove_prefix_list (void);
void gnome_config_set_prefix_list (GSList *list);

/* Internal routines that we export
 * Used to go from string->vector and from vector->string
 */
void gnome_config_make_vector (const char *string, int *argcp, char ***argvp);
char *gnome_config_assemble_vector (int argc, const char *const argv []);

/* a set handler is a function which is called every time a gnome_config_set_*
   function is called, this can be used by the app to say guarantee a sync,
   apps using libgnomeui should not call this, libgnomeui already provides
   this, this would be for non-gui apps and apps that use a different toolkit
   then gtk*/
void gnome_config_set_set_handler(void (*func)(void *),void *data);

/* similiar to the above, but the function triggers on a sync */
void gnome_config_set_sync_handler(void (*func)(void *),void *data);

END_GNOME_DECLS

#endif
