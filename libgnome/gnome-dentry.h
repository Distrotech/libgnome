#ifndef __GNOME_DENTRY_H__
#define __GNOME_DENTRY_H__

BEGIN_GNOME_DECLS

typedef struct {
	char *name;		/* These contain the translated name/comment */
	char *comment;
	int exec_length;	/* Length of `exec' vector.  Does not
				   include the NULL terminator.  */
	char **exec;		/* Command to execute.  Must be NULL
				   terminated.  */
	char *tryexec;		/* Test program to execute */
	char *icon;		/* Icon name */
	char *docpath;		/* Path to the documentation */
	int   terminal;		/* flag: requires a terminal to run */
	char *type;		/* type of this dentry */
	char *location;		/* path of this dentry */
	char *geometry;		/* geometry for this icon */
	
	/* These are computed from icon_base */
	unsigned int multiple_args:1;
} GnomeDesktopEntry;

GnomeDesktopEntry *gnome_desktop_entry_load (const char *file);
GnomeDesktopEntry *gnome_desktop_entry_load_flags (const char *file, int clean_from_memory_after_load);
GnomeDesktopEntry *gnome_desktop_entry_load_flags_conditional (const char *file, int clean_from_memory_after_load, int unconditional);
GnomeDesktopEntry *gnome_desktop_entry_load_unconditional (const char *file);
void gnome_desktop_entry_save (GnomeDesktopEntry *dentry);
void gnome_desktop_entry_free (GnomeDesktopEntry *item);
void gnome_desktop_entry_destroy (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch_with_args (GnomeDesktopEntry *item, int the_argc, char *the_argv[]);

GnomeDesktopEntry *gnome_desktop_entry_copy (GnomeDesktopEntry * source);

char *gnome_is_program_in_path (const char *progname);

END_GNOME_DECLS

#endif
