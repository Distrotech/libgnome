#ifndef __GNOME_HELP_H__
#define __GNOME_HELP_H__ 1

#include <glib.h>
#include "gnome-defs.h"

BEGIN_GNOME_DECLS

typedef struct {
    gchar *name;
    gchar *path;
} GnomeHelpMenuEntry;

gchar *gnome_help_file_find_file(gchar *app, gchar *path);

gchar *gnome_help_file_path(gchar *app, gchar *path);

void gnome_help_display(void *ignore, GnomeHelpMenuEntry *ref);

void gnome_help_goto(void *ignore, gchar *file);

END_GNOME_DECLS

#endif /* __GNOME_HELP_H__ */
