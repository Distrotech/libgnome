
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmodule.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-i18n.h>

#include "libgnomeP.h"

#include <gobject/gboxed.h>
#include <gobject/gvaluearray.h>
#include <gobject/gvaluetypes.h>
#include <gobject/gparamspecs.h>

struct _GnomeProgramPrivate {
    enum {
	APP_UNINIT=0,
	APP_CREATE_DONE=1,
	APP_PREINIT_DONE=2,
	APP_POSTINIT_DONE=3
    } state;

    /* Construction properties */
    GnomeModuleInfo *prop_module_info;
    gchar *prop_module_list;
    int prop_popt_flags;
    struct poptOptions *prop_popt_table;
    gchar *prop_human_readable_name;
    gchar *prop_app_prefix;
    gchar *prop_app_libdir;
    gchar *prop_app_sysconfdir;
    gchar *prop_app_datadir;
    gboolean prop_create_directories;

    gchar **gnome_path;

    /* valid-while: state > APP_CREATE_DONE */
    char *app_id;
    char *app_version;
    char **argv;
    int argc;

    /* valid-while: state == APP_PREINIT_DONE */
    poptContext arg_context;

    /* valid-while: state == APP_PREINIT_DONE */
    struct poptOptions *app_options;

    /* valid-while: state == APP_PREINIT_DONE */
    int app_popt_flags;

    /* valid-while: state == APP_PREINIT_DONE */
    GArray *top_options_table;
};

enum {
    PROP_0,
    PROP_MODULE_INFO,
    PROP_MODULES,
    PROP_APP_ID,
    PROP_APP_VERSION,
    PROP_HUMAN_READABLE_NAME,
    PROP_GNOME_PATH,
    PROP_APP_PREFIX,
    PROP_APP_LIBDIR,
    PROP_APP_DATADIR,
    PROP_APP_SYSCONFDIR,
    PROP_CREATE_DIRECTORIES,
    PROP_POPT_TABLE,
    PROP_POPT_FLAGS,
    PROP_POPT_CONTEXT,
    PROP_LAST
};

static void gnome_program_class_init    (GnomeProgramClass *klass);
static void gnome_program_instance_init (GnomeProgram      *program);
static void gnome_program_finalize      (GObject           *object);

static GQuark quark_get_prop = 0;
static GQuark quark_set_prop = 0;

static GPtrArray *program_modules = NULL;
static GPtrArray *program_module_list = NULL;
static gboolean program_initialized = FALSE;
static GnomeProgram *global_program = NULL;

static guint last_property_id = PROP_LAST;
static gpointer parent_class = NULL;

#define	PREALLOC_CPARAMS (8)
#define	PREALLOC_MODINFOS (8)

GType
gnome_program_get_type (void)
{
    static GType program_type = 0;

    if (!program_type) {
	static const GTypeInfo program_info = {
	    sizeof (GnomeProgramClass),
	    (GBaseInitFunc)         NULL,
	    (GBaseFinalizeFunc)     NULL,
	    (GClassInitFunc)        gnome_program_class_init,
	    NULL,                   /* class_finalize */
	    NULL,                   /* class_data */
	    sizeof (GnomeProgram),
	    0,                      /* n_preallocs */
	    (GInstanceInitFunc)     gnome_program_instance_init
	};

	program_type = g_type_register_static
	    (G_TYPE_OBJECT, "GnomeProgram", &program_info, 0);
    }

    return program_type;
}

static void
gnome_program_set_property (GObject *object, guint param_id,
			    const GValue *value, GParamSpec *pspec)
{
    GnomeProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (object));

    program = GNOME_PROGRAM (object);

    switch (param_id) {
    case PROP_MODULE_INFO:
	program->_priv->prop_module_info = g_value_dup_boxed ((GValue *) value);
	break;
    case PROP_MODULES:
	program->_priv->prop_module_list = g_value_dup_string (value);
	break;
    case PROP_POPT_TABLE:
	program->_priv->prop_popt_table = g_value_peek_pointer (value);
	break;
    case PROP_POPT_FLAGS:
	program->_priv->prop_popt_flags = g_value_get_int (value);
	break;
    case PROP_HUMAN_READABLE_NAME:
	program->_priv->prop_human_readable_name = g_value_dup_string (value);
	break;
    case PROP_GNOME_PATH:
	if (program->_priv->gnome_path) {
	    g_strfreev (program->_priv->gnome_path);
	    program->_priv->gnome_path = NULL;
	}
	if (g_value_get_string (value))
	    program->_priv->gnome_path = g_strsplit
		(g_value_get_string (value), ":", -1);
	break;
    case PROP_APP_PREFIX:
	program->_priv->prop_app_prefix = g_value_dup_string (value);
	break;
    case PROP_APP_SYSCONFDIR:
	program->_priv->prop_app_sysconfdir = g_value_dup_string (value);
	break;
    case PROP_APP_DATADIR:
	program->_priv->prop_app_datadir = g_value_dup_string (value);
	break;
    case PROP_APP_LIBDIR:
	program->_priv->prop_app_libdir = g_value_dup_string (value);
	break;
    case PROP_CREATE_DIRECTORIES:
	program->_priv->prop_create_directories = g_value_get_boolean (value);
	break;
    default: {
	    GObjectSetPropertyFunc set_func;

	    set_func = g_param_spec_get_qdata (pspec, quark_set_prop);
	    if (set_func)
		set_func (object, param_id, value, pspec);
	    else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);

	    break;
	}
    }
}

static void
gnome_program_get_property (GObject *object, guint param_id, GValue *value,
			    GParamSpec *pspec)
{
    GnomeProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (object));

    program = GNOME_PROGRAM (object);

    switch (param_id) {
    case PROP_APP_ID:
	g_value_set_string (value, program->_priv->app_id);
	break;
    case PROP_APP_VERSION:
	g_value_set_string (value, program->_priv->app_version);
	break;
    case PROP_HUMAN_READABLE_NAME:
	g_value_set_string (value, program->_priv->prop_human_readable_name);
	break;
    case PROP_POPT_CONTEXT:
	g_value_set_pointer (value, program->_priv->arg_context);
	break;
    case PROP_GNOME_PATH:
	if (program->_priv->gnome_path)
	    g_value_set_string (value, g_strjoinv (":", program->_priv->gnome_path));
	else
	    g_value_set_string (value, NULL);
	break;
    case PROP_APP_PREFIX:
	g_value_set_string (value, program->_priv->prop_app_prefix);
	break;
    case PROP_APP_SYSCONFDIR:
	g_value_set_string (value, program->_priv->prop_app_sysconfdir);
	break;
    case PROP_APP_DATADIR:
	g_value_set_string (value, program->_priv->prop_app_datadir);
	break;
    case PROP_APP_LIBDIR:
	g_value_set_string (value, program->_priv->prop_app_libdir);
	break;
    case PROP_CREATE_DIRECTORIES:
	g_value_set_boolean (value, program->_priv->prop_create_directories);
	break;
    default: {
	    GObjectSetPropertyFunc get_func;

	    get_func = g_param_spec_get_qdata (pspec, quark_get_prop);
	    if (get_func)
		get_func (object, param_id, value, pspec);
	    else
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);

	    break;
	}
    }
}

static void
add_to_module_list (GPtrArray *module_list, const gchar *module_name)
{
    char **modnames;
    int i, j;

    if (!module_name)
	return;

    modnames = g_strsplit (module_name, ",", -1);
    for (i = 0; modnames && modnames[i]; i++) {
	for (j = 0; j < module_list->len; j++)
	    if (!strcmp (modnames[i], g_ptr_array_index (module_list, j)))
		return;

	g_ptr_array_add (module_list, g_strdup (modnames[i]));
    }
    g_strfreev (modnames);
}

static int
find_module_in_array (GnomeModuleInfo *ptr, GnomeModuleInfo **array)
{
    int i;

    for (i = 0; array[i] && array[i] != ptr; i++) {
	if (!strcmp(array[i]->name, ptr->name))
	    break;
    }

    if (array[i])
	return i;
    else
	return -1;
}

static void /* recursive */
gnome_program_module_addtolist (GnomeModuleInfo **new_list,
				int *times_visited,
				int *num_items_used,
				int new_item_idx)
{
    GnomeModuleInfo *new_item;
    int i;

    g_assert (new_item >= 0);

    new_item = g_ptr_array_index (program_modules, new_item_idx);
    if(!new_item)
	return;

    if (find_module_in_array (new_item, new_list) >= 0)
	return; /* already cared for */

    /* Does this item have any dependencies? */
    if (times_visited[new_item_idx] > 0) {
	/* We already tried to satisfy all the dependencies for this module,
	 *  and we've come back to it again. There's obviously a loop going on.
	 */
	g_error ("Module '%s' version '%s' has a requirements loop.",
		 new_item->name, new_item->version);
    }
    times_visited[new_item_idx]++;

    if (new_item->requirements) {
	for (i = 0; new_item->requirements[i].required_version; i++) {
	    int n;

	    n = find_module_in_array (new_item->requirements[i].module_info,
				      (GnomeModuleInfo **)program_modules->pdata);
	    gnome_program_module_addtolist
		(new_list, times_visited, num_items_used, n);
	}
    }

    /* now add this module on */
    new_list[*num_items_used] = new_item;
    (*num_items_used)++;
    new_list[*num_items_used] = NULL;
}

static void
gnome_program_module_list_order (void)
{
    int i;
    GnomeModuleInfo **new_list;
    int *times_visited; /* Detects dependency loops */
    int num_items_used;

    new_list = alloca (program_modules->len * sizeof(gpointer));
    new_list[0] = NULL;
    num_items_used = 0;
  
    times_visited = alloca (program_modules->len * sizeof(int));
    memset(times_visited, '\0', program_modules->len * sizeof(int));

    /* Create the new list with proper ordering */
    for(i = 0; i < (program_modules->len - 1); i++) {
	gnome_program_module_addtolist (new_list, times_visited,
					&num_items_used, i);
    }

    /* Now stick the new, ordered list in place */
    memcpy (program_modules->pdata, new_list,
	    program_modules->len * sizeof(gpointer));
}

static void
gnome_program_class_init (GnomeProgramClass *class)
{
    GObjectClass *object_class;

    object_class = (GObjectClass*) class;
    parent_class = g_type_class_peek_parent (class);

    quark_set_prop = g_quark_from_static_string ("gnome-program-set-property");
    quark_get_prop = g_quark_from_static_string ("gnome-program-g-property");

    object_class->set_property = gnome_program_set_property;
    object_class->get_property = gnome_program_get_property;

    g_object_class_install_property
	(object_class,
	 PROP_MODULE_INFO,
	 g_param_spec_boxed (GNOME_PARAM_MODULE_INFO, NULL, NULL,
			     GNOME_TYPE_MODULE_INFO,
			     (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_MODULES,
	 g_param_spec_string (GNOME_PARAM_MODULES, NULL, NULL,
			      NULL,
			      (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_POPT_TABLE,
	 g_param_spec_pointer (GNOME_PARAM_POPT_TABLE, NULL, NULL,
			       (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_POPT_FLAGS,
	 g_param_spec_int (GNOME_PARAM_POPT_FLAGS, NULL, NULL,
			   G_MININT, G_MAXINT, 0,
			   (G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_POPT_CONTEXT,
	 g_param_spec_pointer (GNOME_PARAM_POPT_CONTEXT, NULL, NULL,
			       (G_PARAM_READABLE)));

    g_object_class_install_property
	(object_class,
	 PROP_HUMAN_READABLE_NAME,
	 g_param_spec_string (GNOME_PARAM_HUMAN_READABLE_NAME, NULL, NULL,
			      NULL,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_GNOME_PATH,
	 g_param_spec_string (GNOME_PARAM_GNOME_PATH, NULL, NULL,
			      g_getenv ("GNOME2_PATH"),
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_ID,
	 g_param_spec_string (GNOME_PARAM_APP_ID, NULL, NULL,
			      NULL, G_PARAM_READABLE));

    g_object_class_install_property
	(object_class,
	 PROP_APP_VERSION,
	 g_param_spec_string (GNOME_PARAM_APP_VERSION, NULL, NULL,
			      NULL, G_PARAM_READABLE));

    g_object_class_install_property
	(object_class,
	 PROP_APP_PREFIX,
	 g_param_spec_string (GNOME_PARAM_APP_PREFIX, NULL, NULL,
			      LIBGNOME_PREFIX,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_LIBDIR,
	 g_param_spec_string (GNOME_PARAM_APP_LIBDIR, NULL, NULL,
			      LIBGNOME_LIBDIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_DATADIR,
	 g_param_spec_string (GNOME_PARAM_APP_DATADIR, NULL, NULL,
			      LIBGNOME_DATADIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_APP_SYSCONFDIR,
	 g_param_spec_string (GNOME_PARAM_APP_SYSCONFDIR, NULL, NULL,
			      LIBGNOME_SYSCONFDIR,
			      (G_PARAM_READABLE | G_PARAM_WRITABLE |
			       G_PARAM_CONSTRUCT_ONLY)));

    g_object_class_install_property
	(object_class,
	 PROP_CREATE_DIRECTORIES,
	 g_param_spec_boolean (GNOME_PARAM_CREATE_DIRECTORIES, NULL, NULL,
			       TRUE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE |
				G_PARAM_CONSTRUCT_ONLY)));

    object_class->finalize  = gnome_program_finalize;
}

static void
gnome_program_instance_init (GnomeProgram *program)
{
    guint i;

    program->_priv = g_new0 (GnomeProgramPrivate, 1);

    program->_priv->state = APP_CREATE_DONE;

    for (i = 0; i < program_modules->len; i++) {
	GnomeModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	if (a_module && a_module->instance_init)
	    a_module->instance_init (program, a_module);
    }
}

static void
gnome_program_finalize (GObject* object)
{
    GnomeProgram *program = GNOME_PROGRAM (object);

    g_free (program->_priv);
    program->_priv = NULL;
  
    if (G_OBJECT_CLASS (parent_class)->finalize)
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static gpointer
gnome_module_info_init (void)
{
    return g_new0 (GnomeModuleInfo, 1);
}

static gpointer
gnome_module_info_copy (gpointer boxed)
{
    return g_memdup (boxed, sizeof (GnomeModuleInfo));
}

static void
gnome_module_info_free (gpointer boxed)
{
    g_free (boxed);
}

GType
gnome_module_info_get_type (void)
{
    static GType module_info_type = 0;

    if (!module_info_type)
	module_info_type = g_boxed_type_register_static
	    ("GnomeModuleInfo", gnome_module_info_init,
	     gnome_module_info_copy, gnome_module_info_free,
	     FALSE);

    return module_info_type;
}

/**
 * gnome_program_get:
 *
 * Returns an object that stores information on the GNOME application's state.
 * If the object does not exist, it is created.
 *
 * Other functions assume that this will always return an appobject
 * with state > APP_UNINIT
 */

GnomeProgram *
gnome_program_get (void)
{
    return global_program;
}

/**
 * gnome_program_get_name
 * @program: The program object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as an identifier. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: application ID string.
 */
const char *
gnome_program_get_name (GnomeProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    return program->_priv->app_id;
}

/**
 * gnome_program_get_version
 * @app: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as a version number. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: application version string.
 */
const char *
gnome_program_get_version (GnomeProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    return program->_priv->app_version;
}

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
const char *
gnome_program_get_human_readable_name (GnomeProgram *program)
{
    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);

    return program->_priv->prop_human_readable_name;
}

guint
gnome_program_install_property (GnomeProgramClass *pclass,
				GObjectGetPropertyFunc get_fn,
				GObjectSetPropertyFunc set_fn,
				GParamSpec *pspec)
{
    g_return_val_if_fail (pclass != NULL, -1);
    g_return_val_if_fail (GNOME_IS_PROGRAM_CLASS (pclass), -1);
    g_return_val_if_fail (pspec != NULL, -1);

    g_param_spec_set_qdata (pspec, quark_get_prop, (gpointer)get_fn);
    g_param_spec_set_qdata (pspec, quark_set_prop, (gpointer)set_fn);

    g_object_class_install_property (G_OBJECT_CLASS (pclass),
				     last_property_id, pspec);

    return last_property_id++;
}

/**
 * gnome_program_locate_file:
 * @domain: A domain (see GnomeFileDomain in gnome-program.h).
 * @filename: A file name or path inside the 'domain' to find.
 * @only_if_exists: Only return a full pathname if the specified file
 *                  actually exists
 * @ret_locations: If this is not NULL, a list of all the possible locations
 *                 of the file will be returned.
 *
 * This function finds the full path to a file located in the specified
 * "domain". A domain is a name for a collection of related files.
 * For example, common domains are "libdir", "pixmap", and "config".
 *
 *   NOTE: This function is to locate system-wide files, such as files which
 *         have been installed by libgnomeui-2 or another platform library.
 *
 *         Do *NOT* use it to locate files which are installed by your own
 *         application; if you have an application `foo' and you want to
 *         load a pixmap `foo.png' which it installs, define FOOPIXMAPDIR
 *         in your app's Makefile.am and use FOOPIXMAPDIR "/foo.png".
 *
 * The ret_locations list and its contents should be freed by the caller.
 *
 * Returns: The full path to the file (if it exists or only_if_exists is
 *          FALSE) or NULL.
 */
gchar *
gnome_program_locate_file (GnomeProgram *program, GnomeFileDomain domain,
			   const gchar *file_name, gboolean only_if_exists,
			   GSList **ret_locations)
{
    gchar *prefix_rel = NULL, *attr_name = NULL, *attr_rel = NULL;
    gchar fnbuf [PATH_MAX], *retval = NULL, *lastval = NULL, **ptr;
    gboolean append_app_id = FALSE;
    GValue value = { 0, };

    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_PROGRAM (program), NULL);
    g_return_val_if_fail (program->_priv->state >= APP_PREINIT_DONE, NULL);
    g_return_val_if_fail (file_name != NULL, NULL);

#define ADD_FILENAME(x) { \
lastval = (x); \
if(lastval) { if(ret_locations) *ret_locations = g_slist_append(*ret_locations, lastval); \
if(!retval && !ret_locations) retval = lastval; } \
}

    switch (domain) {
    case GNOME_FILE_DOMAIN_LIBDIR:
	prefix_rel = "/lib";
	attr_name = GNOME_PARAM_APP_LIBDIR;
	attr_rel = "";
	break;
    case GNOME_FILE_DOMAIN_DATADIR:
	prefix_rel = "/share";
	attr_name = GNOME_PARAM_APP_DATADIR;
	attr_rel = "";
	break;
    case GNOME_FILE_DOMAIN_SOUND:
	prefix_rel = "/share/sounds";
	attr_name = GNOME_PARAM_APP_DATADIR;
	attr_rel = "/sounds";
	break;
    case GNOME_FILE_DOMAIN_PIXMAP:
	prefix_rel = "/share/pixmaps";
	attr_name = GNOME_PARAM_APP_DATADIR;
	attr_rel = "/pixmaps";
	break;
    case GNOME_FILE_DOMAIN_CONFIG:
	prefix_rel = "/etc";
	attr_name = GNOME_PARAM_APP_SYSCONFDIR;
	attr_rel = "";
	break;
    case GNOME_FILE_DOMAIN_HELP:
	prefix_rel = "/share/gnome/help";
	attr_name = GNOME_PARAM_APP_DATADIR;
	attr_rel = "/gnome/help";
	break;
    case GNOME_FILE_DOMAIN_APP_HELP:
	prefix_rel = "/share/gnome/help";
	attr_name = GNOME_PARAM_APP_DATADIR;
	attr_rel = "/gnome/help";
	append_app_id = TRUE;
	break;
    default:
	g_warning (G_STRLOC ": unknown file domain %d", domain);
	return NULL;
    }

    if (attr_name) {
	const gchar *dir;

	g_value_init (&value, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (program), attr_name, &value);
	dir = g_value_get_string (&value);

	if (dir) {
	    if (append_app_id)
		g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s/%s",
			    dir, attr_rel, program->_priv->app_id, file_name);
	    else
		g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s",
			    dir, attr_rel, file_name);

	    if (!only_if_exists || g_file_test (fnbuf, G_FILE_TEST_EXISTS))
		ADD_FILENAME (g_strdup (fnbuf));
	}

	g_value_unset (&value);
    }
    if (retval && !ret_locations)
	goto out;

    /* Now check the GNOME_PATH. */
    for (ptr = program->_priv->gnome_path; ptr && *ptr; ptr++) {
	if (append_app_id)
	    g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s/%s",
			*ptr, prefix_rel, program->_priv->app_id, file_name);
	else
	    g_snprintf (fnbuf, sizeof (fnbuf), "%s%s/%s",
			*ptr, prefix_rel, file_name);

	if (!only_if_exists || g_file_test (fnbuf, G_FILE_TEST_EXISTS))
	    ADD_FILENAME (g_strdup (fnbuf));
    }
    if (retval && !ret_locations)
	goto out;

 out:
    return retval;
}

/******** modules *******/

/* Stolen verbatim from rpm/lib/misc.c 
   RPM is Copyright (c) 1998 by Red Hat Software, Inc.,
   and may be distributed under the terms of the GPL and LGPL.
*/
/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
static int rpmvercmp(const char * a, const char * b) {
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;
    
    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    str1 = g_alloca(strlen(a) + 1);
    str2 = g_alloca(strlen(b) + 1);

    strcpy(str1, a);
    strcpy(str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one && *two) {
	while (*one && !isalnum(*one)) one++;
	while (*two && !isalnum(*two)) two++;

	str1 = one;
	str2 = two;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (isdigit(*str1)) {
	    while (*str1 && isdigit(*str1)) str1++;
	    while (*str2 && isdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && isalpha(*str1)) str1++;
	    while (*str2 && isalpha(*str2)) str2++;
	    isnum = 0;
	}
		
	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';

	/* take care of the case where the two version segments are */
	/* different types: one numeric and one alpha */
	if (one == str1) return -1;	/* arbitrary */
	if (two == str2) return -1;

	if (isnum) {
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */
	  
	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    if (strlen(one) > strlen(two)) return 1;
	    if (strlen(two) > strlen(one)) return -1;
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) return rc;
	
	/* restore character that was replaced by null above */
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}

static gboolean
gnome_program_version_check (const char *required_version,
			     const char *provided_version)
{
    if (required_version && provided_version)
	return (rpmvercmp (provided_version, required_version) >= 0);
    else
	return TRUE;
}

/**
 * gnome_program_module_registered:
 * @module_info: A pointer to a GnomeModuleInfo structure describing the module
 *               to be queried
 *
 * Description: This method checks to see whether a specific module has been
 *              initialized in the specified program.
 *
 * Returns: A value indicating whether the specified module has been
 *          registered/initialized in the current program
 */
gboolean
gnome_program_module_registered (const GnomeModuleInfo *module_info)
{
    int i;
    GnomeModuleInfo *curmod;

    g_return_val_if_fail (module_info, FALSE);

    if (!program_modules)
	program_modules = g_ptr_array_new ();

    for(i = 0; i < program_modules->len; i++) {
	curmod = g_ptr_array_index (program_modules, i);
	if(curmod == module_info
	   || !strcmp (curmod->name, module_info->name))
	    return TRUE;
    }

    return FALSE;
}

/**
 * gnome_program_module_register:
 * @module_info: A pointer to a GnomeModuleInfo structure describing the module
 *               to be initialized
 *
 * Description:
 * This function is used to register a module to be initialized by the
 * GNOME library framework. The memory pointed to by 'module_info' must be
 * valid during the whole application initialization process, and the module
 * described by 'module_info' must only use the 'module_info' pointer to
 * register itself.
 *
 */
void
gnome_program_module_register (const GnomeModuleInfo *module_info)
{
    int i;

    g_return_if_fail (module_info);

    if (program_initialized) {
	g_warning (G_STRLOC ": cannot load modules after program is initialized");
	return;
    }

    if (!program_modules)
	program_modules = g_ptr_array_new();

    /* Check that it's not already registered. */

    if (gnome_program_module_registered (module_info))
	return;

    g_ptr_array_add (program_modules, (GnomeModuleInfo *)module_info);

    /* We register requirements *after* the module itself to avoid loops.
       Initialization order gets sorted out later on. */
    if (module_info->requirements) {
	for(i = 0; module_info->requirements[i].required_version; i++) {
	    GnomeModuleInfo *dep_mod;

	    dep_mod = module_info->requirements[i].module_info;
	    if (gnome_program_version_check (module_info->requirements[i].required_version,
					     dep_mod->version))
		gnome_program_module_register (dep_mod);
	    else
		/* The required version is not installed */
		/* I18N needed */
		g_error ("Module '%s' requires version '%s' of module '%s' "
			 "to be installed, and you only have version '%s' of '%s'. "
			 "Aborting application.",
			 module_info->name,
			 module_info->requirements[i].required_version,
			 dep_mod->name,
			 dep_mod->version,
			 dep_mod->name);
	}
    }
}

/**
 * gnome_program_preinit:
 * @program: Application object
 * @app_id: application ID string
 * @app_version: application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 *
 * Description:
 * This function performs the portion of application initialization that
 * needs to be done prior to command line argument parsing. The poptContext
 * returned can be used for getopt()-style option processing.
 *
 * Returns: A poptContext representing the argument parsing state.
 */
poptContext
gnome_program_preinit (GnomeProgram *program,
		       const char *app_id, const char *app_version,
		       int argc, char **argv)
{
    GnomeModuleInfo *a_module;
    poptContext argctx;
    int i;

    g_return_val_if_fail (program != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_PROGRAM (program), NULL);

    if (program->_priv->state != APP_CREATE_DONE)
	return NULL;

    /* On non-glibc systems, this is not set up for us.  */
    if (!program_invocation_name) {
	program_invocation_name = argv[0];
	program_invocation_short_name = g_path_get_basename (program_invocation_name);
    }

    /* 0. Misc setup */
    if (program->_priv->app_id)
	g_free (program->_priv->app_id);
    program->_priv->app_id = g_strdup (app_id);
    g_set_prgname (app_id);
    if (program->_priv->app_version)
	g_free (program->_priv->app_version);
    program->_priv->app_version = g_strdup (app_version);
    program->_priv->argc = argc;
    program->_priv->argv = argv;

    if (!program_modules)
	program_modules = g_ptr_array_new();

    /* Major steps in this function:
       1. Process all framework attributes in 'attrs'
       2. Order the module list for dependencies
       3. Call the preinit functions for the modules
       4. Process other attributes 
       5. Create a top-level 'struct poptOption *' for use in arg-parsing.
       6. Create a poptContext
       7. Cleanup/return
    */

    /* 3. call the pre-init functions */
    for (i = 0; (a_module = g_ptr_array_index (program_modules, i)); i++) {
	if (a_module->pre_args_parse)
	    a_module->pre_args_parse (program, a_module);
    }

    /* 5. Create a top-level 'struct poptOption *' for use in arg-parsing. */
    {
	struct poptOption includer = {NULL, '\0', POPT_ARG_INCLUDE_TABLE,
				      NULL, 0, NULL, NULL};

	program->_priv->top_options_table = g_array_new
	    (TRUE, TRUE, sizeof (struct poptOption));

	/* Put the special popt table in first */
	includer.arg = poptHelpOptions;
	includer.descrip = N_("Help options");
	g_array_append_val (program->_priv->top_options_table, includer);

	if (program->_priv->prop_popt_table) {
	    includer.arg = program->_priv->prop_popt_table;
	    includer.descrip = N_("Application options");
	    g_array_append_val (program->_priv->top_options_table,
				includer);
	}

	for (i = 0; (a_module = g_ptr_array_index(program_modules, i)); i++) {
	    if (a_module->options) {
		includer.arg = a_module->options;
		includer.descrip = (char *)a_module->description;

		g_array_append_val (program->_priv->top_options_table, includer);
	    }
	}

	includer.longName = "load-modules";
	includer.argInfo = POPT_ARG_STRING;
	includer.descrip = N_("Dynamic modules to load");
	includer.argDescrip = N_("MODULE1,MODULE2,...");
	g_array_append_val (program->_priv->top_options_table, includer);
    }

    argctx = program->_priv->arg_context = poptGetContext
	(program->_priv->app_id, argc, (const char **) argv,
	 (struct poptOption *) program->_priv->top_options_table->data,
	 program->_priv->prop_popt_flags);
  
    /* 7. Cleanup/return */
    program->_priv->state = APP_PREINIT_DONE;

    return argctx;
}

/**
 * gnome_program_module_load:
 * @program: Application object
 * @mod_name: module name
 *
 * Loads a shared library that contains a
 * 'GnomeModuleInfo dynamic_module_info' structure.
 */
const GnomeModuleInfo *
gnome_program_module_load (const char *mod_name)
{
    GModule *mh;
    const GnomeModuleInfo *gmi;
    char tbuf[1024];

    g_return_val_if_fail (mod_name != NULL, NULL);

    g_snprintf (tbuf, sizeof(tbuf), "lib%s.so.0", mod_name);

    mh = g_module_open (mod_name, G_MODULE_BIND_LAZY);
    if(!mh) {
	g_snprintf (tbuf, sizeof(tbuf), "lib%s.so", mod_name);

	mh = g_module_open (mod_name, G_MODULE_BIND_LAZY);
    }

    if (!mh)
	return NULL;

    if (g_module_symbol (mh, "dynamic_module_info", (gpointer *)&gmi)) {
	gnome_program_module_register (gmi);
	g_module_make_resident (mh);
	return gmi;
    } else {
	g_module_close (mh);
	return NULL;
    }
}

/**
 * gnome_program_parse_args:
 * @app: Application object
 *
 * Description: Parses the command line arguments for the application
 */
void
gnome_program_parse_args (GnomeProgram *program)
{
    int nextopt;
    poptContext ctx;

    g_return_if_fail (program != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (program));

    if (program->_priv->state != APP_PREINIT_DONE)
	return;

    ctx = program->_priv->arg_context;
    while ((nextopt = poptGetNextOpt (ctx)) > 0 || nextopt == POPT_ERROR_BADOPT)
	/* do nothing */ ;

    if (nextopt != -1) {
	g_print ("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n",
		 poptBadOption (ctx, 0),
		 poptStrerror (nextopt),
		 program->_priv->argv[0]);
	exit (1);
    }
}

/**
 * gnome_program_postinit:
 * @program: Application object
 *
 * Description: Called after gnome_program_parse_args(), this function
 * takes care of post-parse initialization and cleanup
 */
void
gnome_program_postinit (GnomeProgram *program)
{
    int i;
    GnomeModuleInfo *a_module;

    g_return_if_fail (program != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (program));

    if (program->_priv->state != APP_PREINIT_DONE)
	return;

    /* Call post-parse functions */
    for (i = 0; (a_module = g_ptr_array_index(program_modules, i)); i++) {
	if (a_module->post_args_parse)
	    a_module->post_args_parse (program, a_module);
    }

    /* Free up stuff we don't need to keep around for the lifetime of the app */
#if 0
    g_ptr_array_free (program_modules, TRUE);
    program_modules = NULL;
#endif

    g_array_free (program->_priv->top_options_table, TRUE);
    program->_priv->top_options_table = NULL;

    g_blow_chunks(); /* Try to compact memory usage a bit */

    program->_priv->state = APP_POSTINIT_DONE;
}

/**
 * gnome_program_init:
 * @program_id: Application ID string
 * @app_version: Application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 * @...: a NULL-terminated list of attribute name/value pairs
 *
 * Description:
 * Performs application initialization.
 */
GnomeProgram *
gnome_program_init (const char *app_id, const char *app_version,
		    const GnomeModuleInfo *module_info,
		    int argc, char **argv,
		    const char *first_property_name, ...)
{
    GnomeProgram *program;
    va_list args;

    g_type_init ();

    va_start(args, first_property_name);
    program = gnome_program_initv (GNOME_TYPE_PROGRAM,
				   app_id, app_version, module_info,
				   argc, argv, first_property_name, args);
    va_end(args);

    return program;
}

GnomeProgram *
gnome_program_initv (GType type,
		     const char *app_id, const char *app_version,
		     const GnomeModuleInfo *module_info,
		     int argc, char **argv,
		     const char *first_property_name, va_list args)
{
    GnomeProgram *program;
    int i;

    g_type_init ();

    if (!program_initialized) {
	GnomeProgramClass *klass;
	const char *ctmp;

	klass = g_type_class_ref (type);

	program_module_list = g_ptr_array_new ();
	program_modules = g_ptr_array_new ();

	/* Always register libgnome. */
	gnome_program_module_register (&libgnome_module_info);

	/* Register the requested modules. */
	gnome_program_module_register (module_info);

	/* We have to handle --load-modules=foo,bar,baz specially */
	for (i = 0; i < argc; i++) {
	    if (!strncmp (argv[i], "--load-modules=", strlen ("--load-modules=")))
		add_to_module_list (program_module_list, argv[i] + strlen("--load-modules="));
	}

	if ((ctmp = g_getenv ("GNOME_MODULES")))
	    add_to_module_list (program_module_list, ctmp);

	/*
	 * Load all the modules.
	 */

	for (i = 0; i < program_module_list->len; i++) {
	    gchar *modname = g_ptr_array_index (program_module_list, i);

	    gnome_program_module_load (modname);
	}

	for (i = 0; i < program_modules->len; i++) {
	    GnomeModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->init_pass)
		a_module->init_pass (a_module);
	}

	/* Make sure the array is NULL-terminated */
	g_ptr_array_add (program_modules, NULL);

	/* Order the module list for dependencies */
	gnome_program_module_list_order ();

	for (i = 0; i < program_modules->len; i++) {
	    GnomeModuleInfo *a_module = g_ptr_array_index (program_modules, i);

	    if (a_module && a_module->class_init)
		a_module->class_init (klass, a_module);
	}
    }

    program = g_object_new_valist (GNOME_TYPE_PROGRAM,
				   first_property_name, args);

    if (!program_initialized) {
	global_program = program;
	g_object_ref (G_OBJECT (global_program));

	program_initialized = TRUE;
    }

    gnome_program_preinit (program, app_id, app_version, argc, argv);
    gnome_program_parse_args (program);
    gnome_program_postinit (program);

    return program;
}
