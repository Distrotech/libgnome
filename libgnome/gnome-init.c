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
#include "libgnomeP.h"
#include <errno.h>

char *gnome_user_home_dir = 0;
char *gnome_user_dir = 0;
char *gnome_user_private_dir = 0;
char *gnome_app_id = 0;
char gnome_do_not_create_directories = 0;

static void
create_user_gnome_directories (void)
{
	if (gnome_do_not_create_directories)
		return;
	
	if(
	   mkdir(gnome_user_dir, 0700) /* This is per-user info
					   - no need for others to see it */
	   && errno != EEXIST)
	  g_error("Could not create per-user Gnome directory <%s> - aborting\n",
		  gnome_user_dir);

	gnome_user_private_dir = g_concat_dir_and_file (gnome_user_home_dir,
							".gnome_private");
	if(
	   mkdir(gnome_user_private_dir, 0700) /* This is private per-user info
						  mode 700 will be enforced!
						  maybe even other security
						  meassures will be taken */
	   && errno != EEXIST)
	  g_error("Could not create private per-user Gnome directory <%s> - aborting\n",
		  gnome_user_private_dir);

	/*change mode to 0700 on the private directory*/
	if(chmod(gnome_user_private_dir,0700)!=0)
		g_error("Could not set mode 0700 on private per-user Gnome directory <%s> - aborting\n",
		        gnome_user_private_dir);

}

void
gnomelib_init (char *app_id)
{
	gnome_user_home_dir = getenv ("HOME");
	/* never freed - gnome_config currently uses this, and it's better
	   to figure it out once than to repeatedly get it */
	gnome_user_dir = g_concat_dir_and_file (gnome_user_home_dir, ".gnome");
	create_user_gnome_directories ();

	gnome_app_id = app_id;

	gnome_i18n_init ();

	if(getenv("ESOUNDHOST"))
		gnome_sound_init(getenv("ESOUNDHOST"));
	else
		gnome_sound_init("localhost");

	gnome_triggers_init();

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
}

/* Register any command-line arguments we might have.  */
void
gnomelib_register_arguments (void)
{
}
