#ifndef __GNOME_DENTRY_H__
#define __GNOME_DENTRY_H__

BEGIN_GNOME_DECLS

typedef struct {
	char *name;		/* These contain the translated name/comment */
	char *comment;
	char *exec;		/* Program to execute */
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

GnomeDesktopEntry *gnome_desktop_entry_load (char *file);
GnomeDesktopEntry *gnome_desktop_entry_load_flags (char *file, int clean_from_memory_after_load);
void gnome_desktop_entry_save (GnomeDesktopEntry *dentry);
void gnome_desktop_entry_free (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch (GnomeDesktopEntry *item);

char *gnome_is_program_in_path (char *progname);

END_GNOME_DECLS

#endif
