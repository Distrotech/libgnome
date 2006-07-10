#include <config.h>

#include <glib.h>
#include <stdio.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gnome-i18nP.h"

int
main (int argc, char *argv[])
{
  GError *err = NULL;
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

  uri = gnome_vfs_make_uri_from_input_with_dirs (argv[1],
						 GNOME_VFS_MAKE_URI_DIR_CURRENT);
  if (gnome_url_show (uri, &err))
    return 0;

  fprintf (stderr, _("Error showing url: %s\n"), err->message);
  g_error_free (err);

  return 1;
}
