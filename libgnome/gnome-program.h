/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */


/* This module takes care of handling application and library
   initialization and command line parsing */

#ifndef GNOME_PROGRAM_H
#define GNOME_PROGRAM_H

#include <glib.h>
#include <popt.h>
#include <stdarg.h>

#include "gnome-defs.h"
#include <gobject/gobject.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_PROGRAM            (gnome_program_get_type ())
#define GNOME_PROGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_PROGRAM, GnomeProgram))
#define GNOME_PROGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_PROGRAM, GnomeProgramClass))
#define GNOME_IS_PROGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_PROGRAM))
#define GNOME_IS_PROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_PROGRAM))

typedef struct _GnomeProgram          GnomeProgram;
typedef struct _GnomeProgramPrivate   GnomeProgramPrivate;
typedef struct _GnomeProgramClass     GnomeProgramClass;

typedef enum _GnomeFileDomain         GnomeFileDomain;

enum _GnomeFileDomain {
    GNOME_FILE_DOMAIN_UNKNOWN = 0,
    GNOME_FILE_DOMAIN_LIBDIR,
    GNOME_FILE_DOMAIN_DATADIR,
    GNOME_FILE_DOMAIN_SOUND,
    GNOME_FILE_DOMAIN_PIXMAP,
    GNOME_FILE_DOMAIN_CONFIG,
    GNOME_FILE_DOMAIN_HELP,
    GNOME_FILE_DOMAIN_APP_HELP
};

struct _GnomeProgram
{
    GObject object;

    GnomeProgramPrivate *_priv;
};

struct _GnomeProgramClass
{
    GObjectClass object_class;
};

GType
gnome_program_get_type                  (void);

GnomeProgram *
gnome_program_get                       (void);

const char*
gnome_program_get_human_readable_name   (GnomeProgram *program);

const char *
gnome_program_get_name                  (GnomeProgram *program);

const char *
gnome_program_get_version               (GnomeProgram *program);

gchar *
gnome_program_locate_file               (GnomeProgram    *program,
					 GnomeFileDomain  domain,
					 const gchar     *file_name,
					 gboolean         only_if_exists,
					 GSList         **ret_locations);

#define GNOME_PARAM_MODULE_INFO         "module-info"
#define GNOME_PARAM_MODULES             "modules"
#define GNOME_PARAM_POPT_TABLE          "popt-table"
#define GNOME_PARAM_POPT_FLAGS          "popt-flags"
#define GNOME_PARAM_POPT_CONTEXT        "popt-context"
#define GNOME_PARAM_CREATE_DIRECTORIES  "create-directories"
#define GNOME_PARAM_ESPEAKER            "espeaker"
#define GNOME_PARAM_ENABLE_SOUND        "enable-sound"
#define GNOME_PARAM_FILE_LOCATOR        "file-locator"
#define GNOME_PARAM_APP_ID              "app-id"
#define GNOME_PARAM_APP_VERSION         "app-version"
#define GNOME_PARAM_APP_PREFIX          "app-prefix"
#define GNOME_PARAM_APP_SYSCONFDIR      "app-sysconfdir"
#define GNOME_PARAM_APP_DATADIR         "app-datadir"
#define GNOME_PARAM_APP_LIBDIR          "app-libdir"
#define GNOME_PARAM_HUMAN_READABLE_NAME "human-readable-name"
#define GNOME_PARAM_GNOME_PATH          "gnome-path"

/***** application modules (aka libraries :) ******/
#define GNOME_TYPE_MODULE_INFO          (gnome_module_info_get_type ())

GType
gnome_module_info_get_type              (void);

typedef struct _GnomeModuleInfo GnomeModuleInfo;
typedef struct _GnomeModuleRequirement GnomeModuleRequirement;

struct _GnomeModuleRequirement {
    const char *required_version;
    GnomeModuleInfo *module_info;
};

typedef void (*GnomeModuleInitHook) (const GnomeModuleInfo *mod_info);
typedef void (*GnomeModuleConstructor) (GType type, guint n_construct_props,
					GObjectConstructParam *construct_props,
					const GnomeModuleInfo *mod_info);
typedef void (*GnomeModuleHook) (GnomeProgram *program,
				 GnomeModuleInfo *mod_info);

struct _GnomeModuleInfo {
    const char *name, *version, *description;
    GnomeModuleRequirement *requirements; /* last element has NULL version */

    GnomeModuleHook pre_args_parse, post_args_parse;

    struct poptOption *options;

    GnomeModuleInitHook init_pass; /* This gets run before the preinit
				      function to allow the module to
				      register other modules as needed. The
				      module cannot assume its required
				      modules are initialized (they aren't). */

    GnomeModuleConstructor constructor;

    gpointer expansion1, expansion2;
};

/* This function should be called before gnomelib_preinit() - it's an
 * alternative to the "module" property passed by the app.
 */
void
gnome_program_module_register (const GnomeModuleInfo *module_info);

gboolean
gnome_program_module_registered (const GnomeModuleInfo *module_info);

void
gnome_program_module_load (const char *mod_name);

guint
gnome_program_install_property (GnomeProgramClass *pclass,
				GObjectGetPropertyFunc get_fn,
				GObjectSetPropertyFunc set_fn,
				GParamSpec *pspec);

/*
 * If the application writer wishes to use getopt()-style arg
 * processing, they can do it using a while looped sandwiched between
 * calls to these two functions.
 */
poptContext
gnome_program_preinit (GnomeProgram *program,
		       const char *app_id,
		       const char *app_version,
		       int argc, char **argv);

void
gnome_program_parse_args (GnomeProgram *program);

void
gnome_program_postinit (GnomeProgram *program);

/* These are convenience functions that calls gnomelib_preinit(...), have
   popt parse all args, and then call gnomelib_postinit() */
GnomeProgram *
gnome_program_init (const char *app_id, const char *app_version,
		    int argc, char **argv,
		    const char *first_property_name, ...);

GnomeProgram *
gnome_program_initv (const char *app_id, const char *app_version,
		     int argc, char **argv,
		     const char *first_property_name, va_list args);

/* Some systems, like Red Hat 4.0, define these but don't declare
   them.  Hopefully it is safe to always declare them here.  */
extern char *program_invocation_short_name;
extern char *program_invocation_name;

#endif
