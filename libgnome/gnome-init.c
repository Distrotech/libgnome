#include <config.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>
#include "gnome-defs.h"
#include "gnome-util.h"

#ifdef HAVE_LIBINTL
#include "libintl.h"
#endif

char *gnome_user_home_dir = 0;
char *gnome_user_dir = 0;

void
gnomelib_init (int *argc, char ***argv)
{
	gnome_user_home_dir = getenv ("HOME");
	gnome_user_dir = g_concat_dir_and_file (gnome_user_home_dir, ".gnome");
	mkdir (gnome_user_dir, 0755);

	setlocale (LC_ALL, "");
#ifdef HAVE_LIBINTL
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
#endif
}



