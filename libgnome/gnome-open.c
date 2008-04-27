#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <stdio.h>

#include "gnome-url.h"
#include "gnome-program.h"
#include "gnome-init.h"

#include <libgnomevfs/gnome-vfs-utils.h>

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
