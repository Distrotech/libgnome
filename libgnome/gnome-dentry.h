#ifndef __GNOME_DENTRY_H__
#define __GNOME_DENTRY_H__

BEGIN_GNOME_DECLS

typedef struct {
	char *exec;
	char *tryexec;
	char *icon_base;
	char *docpath;
	char *info;
	int  terminal;
	char *type;
	char *location;
	
	/* These are computed from icon_base */
	char *small_icon;
	char *transparent_icon;
	char *opaque_icon;
} GnomeDesktopEntry;

GnomeDesktopEntry *gnome_desktop_entry_load (char *file);
void gnome_desktop_entry_save (GnomeDesktopEntry *dentry);
void gnome_desktop_entry_free (GnomeDesktopEntry *item);
void gnome_desktop_entry_launch (GnomeDesktopEntry *item);

int gnome_is_program_in_path (char *progname);

END_GNOME_DECLS

#endif
