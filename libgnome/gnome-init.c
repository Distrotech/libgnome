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
#include "libgnomeP.h"
#include <errno.h>

const char libgnome_param_create_directories[]="B:libgnome/create_directories";
const char libgnome_param_espeaker[]="S:libgnome/espeaker";
const char libgnome_param_enable_sound[]="B:libgnome/enable_sound";
const char libgnome_param_file_locator[]="P:libgnome/file_locator";
const char libgnome_param_app_prefix[]="S:libgnome/app_prefix";
const char libgnome_param_app_sysconfdir[]="S:libgnome/app_sysconfdir";
const char libgnome_param_app_datadir[]="S:libgnome/app_datadir";
const char libgnome_param_app_libdir[]="S:libgnome/app_libdir";
const char libgnome_param_human_readable_name[] = "S:libgnome/human_readable_name";

/**
 * gnome_program_get_human_readable_name
 * @app: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as a human readable name. The app
 * should provide the name with the GNOME_PARAM_HUMAN_READABLE_NAME
 * init argument. Returns NULL if no name was set.
 *
 * Returns: application human-readable name string.
 */

const char*
gnome_program_get_human_readable_name(const GnomeProgram *app)
{
  const GnomeAttributeValue* val;
  
  g_return_val_if_fail (app, NULL);

  val = gnome_program_attribute_get(app, LIBGNOME_PARAM_HUMAN_READABLE_NAME);

  if (val) {
    g_assert(val->type == GNOME_ATTRIBUTE_STRING);
    return val->u.string_value;
  } else {
    return NULL;
  }
}

char *gnome_user_dir = NULL, *gnome_user_private_dir = NULL, *gnome_user_accels_dir = NULL;

static void libgnome_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);
static void libgnome_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info);
static void libgnome_option_cb(poptContext ctx, enum poptCallbackReason reason,
			       const struct poptOption *opt, const char *arg,
			       void *data);
static void libgnome_userdir_setup(gboolean create_dirs);

enum { ARG_VERSION=1 };

static struct poptOption gnomelib_options[] = {
        { NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},

	{ NULL, '\0', POPT_ARG_CALLBACK, libgnome_option_cb, 0, NULL, NULL},

	{"version", '\0', POPT_ARG_NONE, NULL, },

	{ NULL, '\0', 0, NULL, 0 }
};

GnomeModuleInfo libgnome_module_info = {
  "libgnome", VERSION, "GNOME Library",
  NULL,
  libgnome_pre_args_parse, libgnome_post_args_parse,
  gnomelib_options
};

static void
libgnome_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  /* Provide default settings */
  gnome_program_attributes_set(app,
			       LIBGNOME_PARAM_CREATE_DIRECTORIES, TRUE,
			       NULL);
}

static void
libgnome_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  gboolean create_dirs_val = TRUE;

  gnome_program_attributes_get(app,
			  LIBGNOME_PARAM_CREATE_DIRECTORIES, &create_dirs_val,
			  NULL);
			  
  gnome_triggers_init ();

  libgnome_userdir_setup(create_dirs_val);

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, GNOMELOCALEDIR); /* XXX todo - handle multiple installation dirs */
  gnome_i18n_init ();
}

static void
libgnome_option_cb(poptContext ctx, enum poptCallbackReason reason,
		   const struct poptOption *opt, const char *arg,
		   void *data)
{
  GnomeProgram *app;

  app = gnome_program_get();
	
  switch(reason)
    {
    case POPT_CALLBACK_REASON_OPTION:
      switch(opt->val) {
      case ARG_VERSION:
	g_print ("Gnome %s %s\n", gnome_program_get_name(app), gnome_program_get_version(app));
	exit(0);
	break;
      }
    default:
      /* do nothing */
      break;
    }
}

static void
libgnome_userdir_setup(gboolean create_dirs)
{
  if(!gnome_user_dir)
    {
      gnome_user_dir = g_concat_dir_and_file (g_get_home_dir(), ".gnome");
      gnome_user_private_dir = g_concat_dir_and_file (g_get_home_dir(),
						      ".gnome_private");
      gnome_user_accels_dir = g_concat_dir_and_file (gnome_user_dir, "accels");
    }

  if(!create_dirs)
    return;
	
  if (mkdir(gnome_user_dir, 0700) < 0) { /* private permissions, but we
                                            don't check that we got them */
    if (errno != EEXIST) {
      fprintf(stderr, _("Could not create per-user gnome configuration directory `%s': %s\n"),
              gnome_user_dir, strerror(errno));
      exit(1);
    }
  }

  if(mkdir(gnome_user_private_dir, 0700) < 0) { /* This is private
                                                  per-user info mode
                                                  700 will be
                                                  enforced!  maybe
                                                  even other security
                                                  meassures will be
                                                  taken */
    if (errno != EEXIST) {
      fprintf(stderr, _("Could not create per-user gnome configuration directory `%s': %s\n"),
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
  
  if (mkdir(gnome_user_accels_dir, 0700) < 0) {
    if (errno != EEXIST) {
      fprintf(stderr, _("Could not create gnome accelerators directory `%s': %s\n"),
              gnome_user_accels_dir, strerror(errno));
      exit(1);
    }
  }
}


