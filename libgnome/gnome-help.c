#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "gnome-help.h"

#define HELP_PROG "gnome-help-browser"

gchar *gnome_help_file_path(gchar *app, gchar *path)
{
    gchar buf[BUFSIZ];
    gchar *lang, *res;
    
    lang = getenv("LANGUAGE");
    if (!lang)
	lang = getenv("LANG");
    if (!lang)
	lang = "C";

    /* XXX need to traverse LANGUAGE var to find appropriate topic.dat */

    g_snprintf(buf, sizeof(buf), "gnome/help/%s/%s/%s", app, lang, path);
    res = (gchar *)gnome_unconditional_datadir_file(buf);

    return res;
}

void gnome_help_display(void *ignore, GnomeHelpMenuEntry *ref)
{
    gchar *file;

    file = gnome_help_file_path(ref->name, ref->path);
    gnome_help_goto(ignore, file);
    g_free(file);
}

void gnome_help_goto(void *ignore, gchar *file)
{
    pid_t pid;
    
    printf("gnome_help_goto: %s\n", (char *)file);

    if (!(pid = fork())) {
	execlp(HELP_PROG, HELP_PROG, file, NULL);
	g_error("gnome_help_goto: exec:", g_strerror(errno));
    }
}
