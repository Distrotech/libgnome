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

#include <libgnome/libgnome-init.h>

#include "libgnomeP.h"
#include <errno.h>

#include <gobject/gobject.h>
#include <gobject/gparamspecs.h>
#include <gobject/gvaluetypes.h>

#include <liboaf/liboaf.h>
#include <libbonobo.h>

#ifdef HAVE_GNOMESUPPORT
#include <gnomesupport.h>
#endif

#include <libgnomevfs/gnome-vfs-init.h>

/*****************************************************************************
 * oaf
 *****************************************************************************/

static void
gnome_oaf_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	g_thread_init (NULL);

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
	NULL, NULL,
	gnome_oaf_pre_args_parse, gnome_oaf_post_args_parse,
	oaf_popt_options
};

/*****************************************************************************
 * libbonobo
 *****************************************************************************/

typedef struct {
	guint desktop_config_database_id;
	guint desktop_config_moniker_id;
	guint config_database_id;
	guint config_moniker_id;
} GnomeProgramClass_libbonobo;

typedef struct {
	gboolean constructed;

	gchar *config_moniker;
	Bonobo_ConfigDatabase config_database;

	gchar *desktop_config_moniker;
	Bonobo_ConfigDatabase desktop_config_database;
} GnomeProgramPrivate_libbonobo;

static GQuark quark_gnome_program_private_libbonobo = 0;
static GQuark quark_gnome_program_class_libbonobo = 0;

static Bonobo_ConfigDatabase
get_db (GnomeProgram *program, const char *key, CORBA_Environment *opt_ev)
{
	GValue value = { 0, };
	Bonobo_ConfigDatabase database;

	g_return_val_if_fail (program != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (GNOME_IS_PROGRAM (program), CORBA_OBJECT_NIL);

	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program), key, &value);
	database = (Bonobo_ConfigDatabase) g_value_get_pointer (&value);
	g_value_unset (&value);

	return bonobo_object_dup_ref (database, opt_ev);
}

Bonobo_ConfigDatabase
gnome_program_get_config_database (GnomeProgram *program)
{
	return get_db (program, GNOME_PARAM_CONFIG_DATABASE, NULL);
}

Bonobo_ConfigDatabase
gnome_program_get_desktop_config_database (GnomeProgram *program)
{
	return get_db (program, GNOME_PARAM_DESKTOP_CONFIG_DATABASE, NULL);
}

static void
libbonobo_get_property (GObject *object, guint param_id, GValue *value,
			GParamSpec *pspec)
{
	GnomeProgramClass_libbonobo *cdata;
	GnomeProgramPrivate_libbonobo *priv;
	GnomeProgram *program;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PROGRAM (object));

	program = GNOME_PROGRAM (object);

	cdata = g_type_get_qdata (G_OBJECT_TYPE (program), quark_gnome_program_class_libbonobo);
	priv = g_object_get_qdata (G_OBJECT (program), quark_gnome_program_private_libbonobo);

	if (param_id == cdata->config_database_id)
		g_value_set_object (value, (GObject *) priv->config_database);

	else if (param_id == cdata->config_moniker_id)
		g_value_set_string (value, priv->config_moniker);

	else if (param_id == cdata->desktop_config_database_id)
		g_value_set_object (value, (GObject *) priv->desktop_config_database);

	else if (param_id == cdata->desktop_config_moniker_id)
		g_value_set_string (value, priv->desktop_config_moniker);

	else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
}

static void
libbonobo_set_property (GObject *object, guint param_id,
			const GValue *value, GParamSpec *pspec)
{
	GnomeProgramClass_libbonobo *cdata;
	GnomeProgramPrivate_libbonobo *priv;
	GnomeProgram *program;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_PROGRAM (object));

	program = GNOME_PROGRAM (object);

	cdata = g_type_get_qdata (G_OBJECT_TYPE (program), quark_gnome_program_class_libbonobo);
	priv = g_object_get_qdata (G_OBJECT (program), quark_gnome_program_private_libbonobo);

	if (!priv->constructed) {
		if (param_id == cdata->config_database_id) {
			bonobo_object_release_unref (priv->config_database, NULL);
			priv->config_database =
				bonobo_object_dup_ref (g_value_get_pointer (value), NULL);

		} else if (param_id == cdata->config_moniker_id)
			priv->config_moniker = g_value_dup_string (value);
	
		else if (param_id == cdata->desktop_config_database_id) {
			bonobo_object_release_unref (priv->desktop_config_database, NULL);
			priv->desktop_config_database =
				bonobo_object_dup_ref (g_value_get_pointer (value), NULL);

		} else if (param_id == cdata->desktop_config_moniker_id)
			priv->desktop_config_moniker = g_value_dup_string (value);

		else
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	} else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
}

static void
libbonobo_init_pass (const GnomeModuleInfo *mod_info)
{
	if (!quark_gnome_program_private_libbonobo)
		quark_gnome_program_private_libbonobo = g_quark_from_static_string
			("gnome-program-private:libbonobo");

	if (!quark_gnome_program_class_libbonobo)
		quark_gnome_program_class_libbonobo = g_quark_from_static_string
			("gnome-program-class:libbonobo");
}

static void
libbonobo_class_init (GnomeProgramClass *klass, const GnomeModuleInfo *mod_info)
{
	GnomeProgramClass_libbonobo *cdata = g_new0 (GnomeProgramClass_libbonobo, 1);

	g_type_set_qdata (G_OBJECT_CLASS_TYPE (klass), quark_gnome_program_class_libbonobo, cdata);

	cdata->config_moniker_id = gnome_program_install_property
		(klass, libbonobo_get_property, libbonobo_set_property,
		 g_param_spec_string (GNOME_PARAM_CONFIG_MONIKER, NULL, NULL,
				      NULL,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT_ONLY)));

	cdata->config_database_id = gnome_program_install_property
		(klass, libbonobo_get_property, libbonobo_set_property,
		 g_param_spec_object (GNOME_PARAM_CONFIG_DATABASE, NULL, NULL,
				      G_TYPE_POINTER,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT_ONLY)));

	cdata->desktop_config_moniker_id = gnome_program_install_property
		(klass, libbonobo_get_property, libbonobo_set_property,
		 g_param_spec_string (GNOME_PARAM_DESKTOP_CONFIG_MONIKER, NULL, NULL,
				      NULL,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT_ONLY)));

	cdata->desktop_config_database_id = gnome_program_install_property
		(klass, libbonobo_get_property, libbonobo_set_property,
		 g_param_spec_object (GNOME_PARAM_DESKTOP_CONFIG_DATABASE, NULL, NULL,
				      G_TYPE_POINTER,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT_ONLY)));
}

static void
libbonobo_instance_init (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	GnomeProgramPrivate_libbonobo *priv = g_new0 (GnomeProgramPrivate_libbonobo, 1);

	g_object_set_qdata (G_OBJECT (program), quark_gnome_program_private_libbonobo, priv);

	priv->config_database = CORBA_OBJECT_NIL;
	priv->desktop_config_database = CORBA_OBJECT_NIL;
}

static void
libbonobo_post_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	int dumb_argc = 1;
	char *dumb_argv[] = {NULL};
	GnomeProgramPrivate_libbonobo *priv = g_new0 (GnomeProgramPrivate_libbonobo, 1);
	CORBA_Environment ev;

	g_message (G_STRLOC);

	dumb_argv [0] = program_invocation_name;

	bonobo_init (&dumb_argc, dumb_argv);

	priv = g_object_get_qdata (G_OBJECT (program), quark_gnome_program_private_libbonobo);

	priv->constructed = TRUE;

	g_message (G_STRLOC ": %p - `%s'", priv->config_database, priv->config_moniker);

	CORBA_exception_init (&ev);
	bonobo_get_object (priv->config_moniker, "Bonobo/ConfigDatabase", &ev);
	CORBA_exception_free (&ev);
}

static GnomeModuleRequirement libbonobo_requirements [] = {
	{ VERSION, &gnome_oaf_module_info },
	{ NULL }
};

GnomeModuleInfo libbonobo_module_info = {
	"libbonobo", VERSION, N_("Bonobo Support"),
	libbonobo_requirements, libbonobo_instance_init,
	NULL, libbonobo_post_args_parse,
	NULL,
	libbonobo_init_pass, libbonobo_class_init,
	NULL, NULL
};

/*****************************************************************************
 * libgnome
 *****************************************************************************/

enum { ARG_VERSION = 1 };

char *gnome_user_dir = NULL, *gnome_user_private_dir = NULL, *gnome_user_accels_dir = NULL;

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
	bindtextdomain (PACKAGE, LIBGNOME_LOCALEDIR);
	gnome_i18n_init ();
}

static struct poptOption gnomelib_options [] = {
	{ NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},

	{ NULL, '\0', POPT_ARG_CALLBACK, (void *) libgnome_option_cb, 0, NULL, NULL},

	{"version", '\0', POPT_ARG_NONE, NULL, },

	{ NULL, '\0', 0, NULL, 0 }
};

static void
gnome_vfs_pre_args_parse (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
	gnome_vfs_init ();
}

GnomeModuleInfo gnome_vfs_module_info = {
	"gnome-vfs", GNOMEVFSVERSION, "GNOME Virtual Filesystem",
	NULL, NULL,
	gnome_vfs_pre_args_parse, NULL,
	NULL,
	(GnomeModuleInitHook) gnome_vfs_loadinit,
	NULL
};

static GnomeModuleRequirement libgnome_requirements [] = {
	{ VERSION, &libbonobo_module_info },
	{ VERSION, &libbonobo_module_info },
	{ "0.3.0", &gnome_vfs_module_info },
	{ NULL }
};

GnomeModuleInfo libgnome_module_info = {
	"libgnome", VERSION, "GNOME Library",
	libgnome_requirements, NULL,
	NULL, libgnome_post_args_parse,
	gnomelib_options,
	NULL, NULL, NULL, NULL
};
