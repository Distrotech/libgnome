/* gnomelib-init.c - Implement libgnome module
   Copyright (C) 1997, 1998, 1999 Free Software Foundation
   Copyright (C) 1999, 2000 Red Hat, Inc.
   All rights reserved.

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
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
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
#include <pwd.h>
#include <sys/stat.h>

#include <glib.h>

#define GCONF_ENABLE_INTERNALS 1
#include <gconf/gconf-client.h>
extern struct poptOption gconf_options[];

#include "libgnomeP.h"
#include <errno.h>

#include <gobject/gobject.h>
#include <gobject/gvaluetypes.h>

#include <liboaf/liboaf.h>

#include <libgnomevfs/gnome-vfs-init.h>

/*****************************************************************************
 * oaf
 *****************************************************************************/

static void
gnome_oaf_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    oaf_preinit (program, mod_info);
}

static void
gnome_oaf_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    int dumb_argc = 1;
    char *dumb_argv[] = {NULL};

    oaf_postinit (program, mod_info);

    dumb_argv[0] = program_invocation_name;
    (void) oaf_orb_init (&dumb_argc, dumb_argv);
}

GnomeModuleInfo gnome_oaf_module_info = {
    "gnome-oaf", VERSION, N_("GNOME OAF Support"),
    NULL,
    gnome_oaf_pre_args_parse, gnome_oaf_post_args_parse,
    oaf_popt_options
};

/*****************************************************************************
 * gconf
 *****************************************************************************/

static gchar *
gnome_gconf_get_gnome_libs_settings_relative (const gchar *subkey)
{
    gchar *dir;
    gchar *key;

    dir = g_strconcat ("/apps/gnome-settings/",
		       gnome_program_get_name (gnome_program_get ()),
		       NULL);

    if (subkey && *subkey) {
	key = gconf_concat_dir_and_key (dir, subkey);
	g_free (dir);
    } else {
	/* subkey == "" */
	key = dir;
    }

    return key;
}

static gchar * G_GNUC_UNUSED
gnome_gconf_get_app_settings_relative (const gchar *subkey)
{
    gchar *dir;
    gchar *key;

    dir = g_strconcat ("/apps/",
		       gnome_program_get_name (gnome_program_get ()),
		       NULL);

    if (subkey && *subkey) {
	key = gconf_concat_dir_and_key (dir, subkey);
	g_free (dir);
    } else {
	/* subkey == "" */
	key = dir;
    }

    return key;
}

static GConfClient* global_client = NULL;

static GConfClient * G_GNUC_UNUSED
gnome_get_gconf_client (void)
{
    g_return_val_if_fail (global_client != NULL, NULL);
        
    return global_client;
}

static void
gnome_gconf_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    gconf_preinit (program, mod_info);
}

static void
gnome_gconf_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    gchar *settings_dir;

    gconf_postinit (program, mod_info);

    global_client = gconf_client_get_default();

    gconf_client_add_dir (global_client,
			  "/desktop/gnome",
			  GCONF_CLIENT_PRELOAD_NONE, NULL);

    settings_dir = gnome_gconf_get_gnome_libs_settings_relative ("");

    gconf_client_add_dir (global_client,
			  settings_dir,
			  /* Possibly we should turn preload on for this */
			  GCONF_CLIENT_PRELOAD_NONE,
			  NULL);
    g_free (settings_dir);
}

static GnomeModuleRequirement gnome_gconf_requirements[] = {
    { VERSION, &gnome_oaf_module_info },
    { NULL, NULL }
};

GnomeModuleInfo gnome_gconf_module_info = {
    "gnome-gconf", VERSION, N_("GNOME GConf Support"),
    gnome_gconf_requirements,
    gnome_gconf_pre_args_parse, gnome_gconf_post_args_parse,
    gconf_options
};

/*****************************************************************************
 * libgnome
 *****************************************************************************/

char *gnome_user_dir = NULL, *gnome_user_private_dir = NULL, *gnome_user_accels_dir = NULL;

static void libgnome_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
static void libgnome_loadinit(const GnomeModuleInfo *mod_info);
static void libgnome_option_cb(poptContext ctx, enum poptCallbackReason reason,
			       const struct poptOption *opt, const char *arg,
			       void *data);
static void libgnome_userdir_setup(gboolean create_dirs);

enum { ARG_VERSION=1 };

static struct poptOption gnomelib_options[] = {
    { NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},

    { NULL, '\0', POPT_ARG_CALLBACK, (void *) libgnome_option_cb, 0, NULL, NULL},

    {"version", '\0', POPT_ARG_NONE, NULL, },

    { NULL, '\0', 0, NULL, 0 }
};

GnomeModuleInfo gnome_vfs_module_info = {
    "gnome-vfs", GNOMEVFSVERSION, "GNOME Virtual Filesystem",
    NULL,
    (GnomeModuleHook) gnome_vfs_preinit, (GnomeModuleHook) gnome_vfs_postinit,
    NULL,
    (GnomeModuleInitHook) gnome_vfs_loadinit,
    NULL
};

static GnomeModuleRequirement libgnome_requirements[] = {
    {VERSION, &gnome_gconf_module_info},
    {"0.3.0", &gnome_vfs_module_info},
    {NULL}
};

GnomeModuleInfo libgnome_module_info = {
    "libgnome", VERSION, "GNOME Library",
    libgnome_requirements,
    NULL, libgnome_post_args_parse,
    gnomelib_options,
    libgnome_loadinit
};

static void
libgnome_post_args_parse (GnomeProgram *program,
			  GnomeModuleInfo *mod_info)
{
    GValue value = { 0, };
    gboolean create_dirs_val;

    g_value_init (&value, G_TYPE_BOOLEAN);
    g_object_get_property (G_OBJECT (program), "create-directories", &value);
    create_dirs_val = g_value_get_boolean (&value);
    g_value_unset (&value);

    gnome_triggers_init ();

    libgnome_userdir_setup (create_dirs_val);

    setlocale (LC_ALL, "");
    /* XXX todo - handle multiple installation dirs */
    bindtextdomain (PACKAGE, GNOMELOCALEDIR);
    gnome_i18n_init ();
}

static void
libgnome_loadinit (const GnomeModuleInfo *mod_info)
{
    /* Initialize threads. */
    g_thread_init (NULL);
}

static void
libgnome_option_cb (poptContext ctx, enum poptCallbackReason reason,
		    const struct poptOption *opt, const char *arg,
		    void *data)
{
    GnomeProgram *program;

    program = gnome_program_get ();
	
    switch(reason) {
    case POPT_CALLBACK_REASON_OPTION:
	switch(opt->val) {
	case ARG_VERSION:
	    g_print ("Gnome %s %s\n",
		     gnome_program_get_name (program),
		     gnome_program_get_version (program));
	    exit(0);
	    break;
	}
    default:
	/* do nothing */
	break;
    }
}

static void
libgnome_userdir_setup (gboolean create_dirs)
{
    if(!gnome_user_dir) {
	gnome_user_dir = g_concat_dir_and_file (g_get_home_dir(), ".gnome");
	gnome_user_private_dir = g_concat_dir_and_file (g_get_home_dir(),
							".gnome_private");
	gnome_user_accels_dir = g_concat_dir_and_file (gnome_user_dir, "accels");
    }

    if (!create_dirs)
	return;
	
    if (mkdir (gnome_user_dir, 0700) < 0) { /* private permissions, but we
					       don't check that we got them */
	if (errno != EEXIST) {
	    fprintf(stderr, _("Could not create per-user gnome configuration directory `%s': %s\n"),
		    gnome_user_dir, strerror(errno));
	    exit(1);
	}
    }
    
    if (mkdir (gnome_user_private_dir, 0700) < 0) { /* This is private
						       per-user info mode
						       700 will be
						       enforced!  maybe
						       even other security
						       meassures will be
						       taken */
	if (errno != EEXIST) {
	    fprintf (stderr, _("Could not create per-user gnome configuration directory `%s': %s\n"),
		     gnome_user_private_dir, strerror(errno));
	    exit(1);
	}
    }


    /* change mode to 0700 on the private directory */
    if (chmod (gnome_user_private_dir, 0700) < 0) {
	fprintf(stderr, _("Could not set mode 0700 on private per-user gnome configuration directory `%s': %s\n"),
		gnome_user_private_dir, strerror(errno));
	exit(1);
    }
  
    if (mkdir (gnome_user_accels_dir, 0700) < 0) {
	if (errno != EEXIST) {
	    fprintf(stderr, _("Could not create gnome accelerators directory `%s': %s\n"),
		    gnome_user_accels_dir, strerror(errno));
	    exit(1);
	}
    }
}
