#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include <stdio.h>

#include "gnome-url.h"
#include "gnome-program.h"
#include "gnome-init.h"

int
main (int argc, char *argv[])
{
  GError *err = NULL;
  GFile *file;
  char *uri;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <url>\n", argv[0]);
      return 1;
    }

  gnome_program_init ("gnome-url-show", VERSION,
		      LIBGNOME_MODULE,
		      argc, argv,
		      NULL);

  file = g_file_new_for_commandline_arg (argv[1]);
  uri = g_file_get_uri (file);
  g_object_unref (file);

  if (g_app_info_launch_default_for_uri (uri, NULL, &err))
    return 0;

  fprintf (stderr, _("Error showing url: %s\n"), err->message);
  g_error_free (err);

  return 1;
}
