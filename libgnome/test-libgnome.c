#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libgnome.h"

#include <gobject/gvaluetypes.h>

static int foo = 0;

static struct poptOption options[] = {
    {"foo", '\0', POPT_ARG_INT, &foo, 0, N_("Test option."), NULL},
    {NULL}
};

int
main (int argc, char **argv)
{
    GValue value = { 0, };
    GnomeProgram *program;
    gchar *app_prefix, *app_id, *app_version;
    const gchar *human_readable_name;

    program = gnome_program_init ("test-libgnome", VERSION, argc, argv,
				  GNOME_PARAM_POPT_TABLE, options,
				  GNOME_PARAM_HUMAN_READABLE_NAME,
				  _("The Application Name"),
				  LIBGNOME_INIT, NULL);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_prefix", &value);
    app_prefix = g_value_dup_string (&value);
    g_value_unset (&value);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_id", &value);
    app_id = g_value_dup_string (&value);
    g_value_unset (&value);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_version", &value);
    app_version = g_value_dup_string (&value);
    g_value_unset (&value);

    human_readable_name = gnome_program_get_human_readable_name (program);

    g_message (G_STRLOC ": %d - `%s' - `%s' - `%s' - `%s'", foo, app_prefix,
	       app_id, app_version, human_readable_name);

    return 0;
}
