/* gnomelib-init.c - Implement libgnome module
   Copyright (C) 1997, 1998, 1999 Free Software Foundation
                 1999, 2000 Red Hat, Inc.
		 2001 SuSE Linux AG.
   All rights reserved.

   This file is part of GNOME 2.0.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */
/*
  @NOTATION@
 */

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
#include <glib/gstdio.h>

#include "gnome-i18n.h"
#include "gnome-init.h"
#include "gnome-gconfP.h"
#include "gnome-util.h"
#include "gnome-sound.h"
#include "gnome-triggers.h"
#include "libgnome-private.h"

#include <bonobo-activation/bonobo-activation.h>
#include <bonobo-activation/bonobo-activation-version.h>
#include <libbonobo.h>

#include <libgnomevfs/gnome-vfs-init.h>

/* implemented in gnome-sound.c */
G_GNUC_INTERNAL extern void _gnome_sound_set_enabled (gboolean);

/*****************************************************************************
 * bonobo
 *****************************************************************************/

static void
bonobo_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	int dumb_argc = 1;
	char *dumb_argv[] = {NULL};

	dumb_argv[0] = g_get_prgname ();

	bonobo_init (&dumb_argc, dumb_argv);
}

/**
* gnome_bonobo_module_info_get:
*
* Retrieves the bonobo module version and indicate that it requires the current
* libgnome and its dependencies (although libbonobo does not depend on
* libgnome, libbonoboui does and this will also be initialised when
* initialising a GNOME app).
*
* Returns: a new #GnomeModuleInfo structure describing the version of the
* bonobo modules and its dependents.
*/
const GnomeModuleInfo *
gnome_bonobo_module_info_get (void)
{
	static GnomeModuleInfo module_info = {
		"bonobo",
		/* FIXME: get this from bonobo */"1.101.2",
		N_("Bonobo Support"),
		NULL, NULL,
		NULL, bonobo_post_args_parse,
		NULL, NULL, NULL, NULL, NULL
	};

	if (module_info.requirements == NULL) {
		static GnomeModuleRequirement req[2];

		req[0].required_version = VERSION;
		req[0].module_info = LIBGNOME_MODULE;

		req[1].required_version = NULL;
		req[1].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}

/*****************************************************************************
 * bonobo-activation
 *****************************************************************************/

static void
bonobo_activation_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
        if (!g_thread_supported ())
		g_thread_init (NULL);

	if (!bonobo_activation_is_initialized ())
		bonobo_activation_preinit (program, mod_info);
}

static void
bonobo_activation_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	if (!bonobo_activation_is_initialized ()) {
		int dumb_argc = 1;
		char *dumb_argv[] = {NULL};

		dumb_argv[0] = g_get_prgname ();
		(void) bonobo_activation_orb_init (&dumb_argc, dumb_argv);

		bonobo_activation_postinit (program, mod_info);
	}
}

/* No need to make this public, always pulled in */
static const GnomeModuleInfo *
gnome_bonobo_activation_module_info_get (void)
{
	static GnomeModuleInfo module_info = {
		"bonobo-activation", NULL, N_("Bonobo activation Support"),
		NULL, NULL,
		bonobo_activation_pre_args_parse, bonobo_activation_post_args_parse,
		bonobo_activation_popt_options, NULL, NULL, NULL,
		bonobo_activation_get_goption_group
	};

	if (module_info.version == NULL) {
		module_info.version = g_strdup_printf
			("%d.%d.%d",
			 BONOBO_ACTIVATION_MAJOR_VERSION,
			 BONOBO_ACTIVATION_MINOR_VERSION,
			 BONOBO_ACTIVATION_MICRO_VERSION);
	}
	return &module_info;
}

/*****************************************************************************
 * libgnome
 *****************************************************************************/

enum { ARG_DISABLE_SOUND = 1, ARG_ENABLE_SOUND, ARG_ESPEAKER, ARG_VERSION };

static char *gnome_user_dir = NULL;
static char *gnome_user_private_dir = NULL;
static char *gnome_user_accels_dir = NULL;

/**
* gnome_user_dir_get:
*
* Retrieves the user-specific directory for GNOME apps to use ($HOME/.gnome2
* is the usual GNOME 2 value).
*
* Returns: An absolute path to the directory.
*/
const char *
gnome_user_dir_get (void)
{
	return gnome_user_dir;
}

/**
* gnome_user_private_dir_get:
*
* Differs from gnome_user_dir_get() in that the directory returned here will
* have had permissions of 0700 (rwx------) enforced when it was created.  Of
* course, the permissions may have been altered since creation, so care still
* needs to be taken.
*
* Returns: An absolute path to the user-specific private directory that GNOME
* apps can use.
*/
const char *
gnome_user_private_dir_get (void)
{
	return gnome_user_private_dir;
}

/**
* gnome_user_accels_dir_get:
*
* Retrieves the user-specific directory that stores the keyboard shortcut files
* for each GNOME app. Note that most applications should be using GConf for
* storing this information, but it may be necessary to use the
* gnome_user_accels_dir_get() directory for legacy applications.
*
* Returns: The absolute path to the directory.
*/
const char *
gnome_user_accels_dir_get (void)
{
	return gnome_user_accels_dir;
}

static void
libgnome_option_cb (poptContext ctx, enum poptCallbackReason reason,
		    const struct poptOption *opt, const char *arg,
		    void *data)
{
	GnomeProgram *program;
	GValue value = { 0 };

	program = gnome_program_get ();

	switch(reason) {
	case POPT_CALLBACK_REASON_OPTION:
		switch(opt->val) {
		case ARG_ESPEAKER:
			g_value_init (&value, G_TYPE_STRING);
			g_value_set_string (&value, arg);
			g_object_set_property (G_OBJECT (program),
					       GNOME_PARAM_ESPEAKER, &value);
			g_value_unset (&value);
			break;

		case ARG_DISABLE_SOUND:
			g_value_init (&value, G_TYPE_BOOLEAN);
			g_value_set_boolean (&value, FALSE);
			g_object_set_property (G_OBJECT (program),
					       GNOME_PARAM_ENABLE_SOUND, &value);
			g_value_unset (&value);
			break;

		case ARG_ENABLE_SOUND:
			g_value_init (&value, G_TYPE_BOOLEAN);
			g_value_set_boolean (&value, TRUE);
			g_object_set_property (G_OBJECT (program),
					       GNOME_PARAM_ENABLE_SOUND, &value);
			g_value_unset (&value);
			break;

		case ARG_VERSION:
			g_print ("GNOME %s %s\n",
				 gnome_program_get_app_id (program),
				 gnome_program_get_app_version (program));
			exit(0);
			break;
		}
	default:
		/* do nothing */
		break;
	}
}

static gboolean
libgnome_goption_epeaker (const gchar *option_name,
			  const gchar *value,
			  gpointer data,
			  GError **error)
{
	g_object_set (G_OBJECT (gnome_program_get ()),
		      GNOME_PARAM_ESPEAKER, value, NULL);

	return TRUE;
}

static gboolean
libgnome_goption_disable_sound (const gchar *option_name,
				const gchar *value,
				gpointer data,
				GError **error)
{
	g_object_set (G_OBJECT (gnome_program_get ()),
		      GNOME_PARAM_ENABLE_SOUND, FALSE, NULL);

	return TRUE;
}

static gboolean
libgnome_goption_enable_sound (const gchar *option_name,
			       const gchar *value,
			       gpointer data,
			       GError **error)
{
	g_object_set (G_OBJECT (gnome_program_get ()),
		      GNOME_PARAM_ENABLE_SOUND, TRUE, NULL);

	return TRUE;
}

static void
libgnome_goption_version (void)
{
	GnomeProgram *program;

	program = gnome_program_get ();

	g_print ("GNOME %s %s\n",
		 gnome_program_get_app_id (program),
		 gnome_program_get_app_version (program));

	exit (0);
}

static int
safe_mkdir (const char *pathname, mode_t mode)
{
	char *safe_pathname;
	int len, ret;

	safe_pathname = g_strdup (pathname);
	len = strlen (safe_pathname);

	if (len > 1 && G_IS_DIR_SEPARATOR (safe_pathname[len - 1]))
		safe_pathname[len - 1] = '\0';

	ret = g_mkdir (safe_pathname, mode);

	g_free (safe_pathname);

	return ret;
}

static void
libgnome_userdir_setup (gboolean create_dirs)
{
	struct stat statbuf;
	
	if(!gnome_user_dir) {
                const char *override;

                /* FIXME this env variable should be changed
                 * for each major GNOME release, would be easier to
                 * remember if not hardcoded.
                 */
                override = g_getenv ("GNOME22_USER_DIR");

                if (override != NULL) {
                        int len;

                        gnome_user_dir = g_strdup (override);

                        /* chop trailing slash */
                        len = strlen (gnome_user_dir);
                        if (len > 1 && G_IS_DIR_SEPARATOR (gnome_user_dir[len - 1]))
                                gnome_user_dir[len - 1] = '\0';

                        gnome_user_private_dir = g_strconcat (gnome_user_dir,
                                                              "_private",
                                                              NULL);
                } else {
                        gnome_user_dir = g_build_filename (g_get_home_dir(), GNOME_DOT_GNOME, NULL);
                        gnome_user_private_dir = g_build_filename (g_get_home_dir(),
                                                                   GNOME_DOT_GNOME_PRIVATE, NULL);
                }

		gnome_user_accels_dir = g_build_filename (gnome_user_dir,
							  "accels", NULL);
	}

	if (!create_dirs)
		return;

	if (safe_mkdir (gnome_user_dir, 0700) < 0) { /* private permissions, but we
							don't check that we got them */
		if (errno != EEXIST) {
			g_printerr (_("Could not create per-user gnome configuration directory `%s': %s\n"),
				gnome_user_dir, strerror(errno));
			exit(1);
		}
	}

	if (safe_mkdir (gnome_user_private_dir, 0700) < 0) { /* This is private
								per-user info mode
								700 will be
								enforced!  maybe
								even other security
								meassures will be
								taken */
		if (errno != EEXIST) {
			g_printerr (_("Could not create per-user gnome configuration directory `%s': %s\n"),
				 gnome_user_private_dir, strerror(errno));
			exit(1);
		}
	}


       if (g_stat (gnome_user_private_dir, &statbuf) < 0) {
               g_printerr (_("Could not stat private per-user gnome configuration directory `%s': %s\n"),
                       gnome_user_private_dir, strerror(errno));
               exit(1);
       }

	/* change mode to 0700 on the private directory */
	if (((statbuf.st_mode & 0700) != 0700 )  &&
            g_chmod (gnome_user_private_dir, 0700) < 0) {
		g_printerr (_("Could not set mode 0700 on private per-user gnome configuration directory `%s': %s\n"),
			gnome_user_private_dir, strerror(errno));
		exit(1);
	}

	if (safe_mkdir (gnome_user_accels_dir, 0700) < 0) {
		if (errno != EEXIST) {
			g_printerr (_("Could not create gnome accelerators directory `%s': %s\n"),
				gnome_user_accels_dir, strerror(errno));
			exit(1);
		}
	}
}

static void
libgnome_post_args_parse (GnomeProgram *program,
			  GnomeModuleInfo *mod_info)
{
        gboolean enable_sound = TRUE, create_dirs = TRUE;
        char *espeaker = NULL;

        g_object_get (program,
                      GNOME_PARAM_CREATE_DIRECTORIES, &create_dirs,
                      GNOME_PARAM_ENABLE_SOUND, &enable_sound,
                      GNOME_PARAM_ESPEAKER, &espeaker,
                      NULL);

        gnome_sound_init (espeaker);
        g_free (espeaker);

        _gnome_sound_set_enabled (enable_sound);

        libgnome_userdir_setup (create_dirs);
}

static void
gnome_vfs_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	gnome_vfs_init ();
}

/* No need for this to be public */
static const GnomeModuleInfo *
gnome_vfs_module_info_get (void)
{
	static GnomeModuleInfo module_info = {
		"gnome-vfs", GNOMEVFSVERSION, N_("GNOME Virtual Filesystem"),
		NULL, NULL,
		NULL, gnome_vfs_post_args_parse,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
	return &module_info;
}

static GOptionGroup *
libgnome_module_get_goption_group (void)
{
	GOptionGroup *option_group;
	const GOptionEntry gnomelib_goptions [] = {
		{ "disable-sound", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  libgnome_goption_disable_sound, N_("Disable sound server usage"), NULL },

		{ "enable-sound", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  libgnome_goption_enable_sound, N_("Enable sound server usage"), NULL },

		{ "espeaker", '\0',0, G_OPTION_ARG_CALLBACK,
		  libgnome_goption_epeaker,
		  N_("Host:port on which the sound server to use is running"),
		  N_("HOSTNAME:PORT") },

		{ "version", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  (GOptionArgFunc) libgnome_goption_version, NULL, NULL },

		{ NULL }
	};

	option_group = g_option_group_new ("gnome",
					   N_("GNOME Library"),
					   N_("Show GNOME options"),
					   NULL, NULL);
	g_option_group_set_translation_domain (option_group, GETTEXT_PACKAGE);
	g_option_group_add_entries (option_group, gnomelib_goptions);

	return option_group;
}

/**
* libgnome_module_info_get:
*
* Retrieves the current libgnome version and the modules it depends on.
*
* Returns: a new #GnomeModuleInfo structure describing the version and
* the versions of the dependents.
*/
const GnomeModuleInfo *
libgnome_module_info_get (void)
{
	static const struct poptOption gnomelib_options [] = {
		{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL},

		{ NULL, '\0', POPT_ARG_CALLBACK, (void *) libgnome_option_cb, 0, NULL, NULL},

		{ "disable-sound", '\0', POPT_ARG_NONE,
		  NULL, ARG_DISABLE_SOUND, N_("Disable sound server usage"), NULL},

		{ "enable-sound", '\0', POPT_ARG_NONE,
		  NULL, ARG_ENABLE_SOUND, N_("Enable sound server usage"), NULL},

		{ "espeaker", '\0', POPT_ARG_STRING,
		  NULL, ARG_ESPEAKER, N_("Host:port on which the sound server to use is"
					" running"),
		N_("HOSTNAME:PORT")},

		{ "version", '\0', POPT_ARG_NONE,
		  NULL, ARG_VERSION, VERSION, NULL},

		{ NULL, '\0', 0,
		  NULL, 0 , NULL, NULL}
	};

	static GnomeModuleInfo module_info = {
		"libgnome", VERSION, N_("GNOME Library"),
		NULL, NULL,
		NULL, libgnome_post_args_parse,
		gnomelib_options,
		NULL, NULL, NULL,
		libgnome_module_get_goption_group
	};
	int i = 0;

	if (module_info.requirements == NULL) {
		static GnomeModuleRequirement req[4];

		bindtextdomain (GETTEXT_PACKAGE, LIBGNOME_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
		req[i].required_version = "0.9.1";
		req[i].module_info = gnome_bonobo_activation_module_info_get ();
		i++;

		req[i].required_version = "0.3.0";
		req[i].module_info = gnome_vfs_module_info_get ();
		i++;

		req[i].required_version = "1.1.1";
		req[i].module_info = gnome_gconf_module_info_get ();
		i++;

		req[i].required_version = NULL;
		req[i].module_info = NULL;
		i++;

		module_info.requirements = req;
	}

	return &module_info;
}
