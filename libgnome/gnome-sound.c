/*
 * Copyright (C) 1997-1998 Stuart Parmenter and Elliot Lee
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

#include "config.h"

#include <libgnome/gnome-sound.h>
#include <libgnome/gnome-i18n.h>

#include <gmodule.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct _GnomeSoundModule            GnomeSoundModule;
typedef struct _GnomeSoundModuleClass       GnomeSoundModuleClass;

#define GNOME_TYPE_SOUND_MODULE             (gnome_sound_module_get_type ())
#define GNOME_SOUND_MODULE(sound_module)    (G_TYPE_CHECK_INSTANCE_CAST ((sound_module), GNOME_TYPE_SOUND_MODULE, GnomeSoundModule))
#define GNOME_IS_SOUND_MODULE(sound_module) (G_TYPE_CHECK_INSTANCE_TYPE ((sound_module), GNOME_TYPE_SOUND_MODULE))

struct _GnomeSoundModule
{
    GTypeModule parent_instance;
    GModule *library;
    gchar *path;

    GnomeSoundPlugin *plugin;
};

struct _GnomeSoundModuleClass 
{
    GTypeModuleClass parent_class;
};

static GObjectClass *module_parent_class = NULL;
static GnomeSoundModule *sound_module = NULL;
static GnomeSoundDriver *sound_driver = NULL;

GType gnome_sound_module_get_type (void);

static gboolean
gnome_sound_module_load (GTypeModule *module)
{
    GnomeSoundModule *sound_module = GNOME_SOUND_MODULE (module);
  
    sound_module->library = g_module_open (sound_module->path, 0);
    if (!sound_module->library) {
	g_warning (g_module_error ());
	return FALSE;
    }
  
    /* extract symbols from the lib */
    if (!g_module_symbol (sound_module->library, "gnome_sound_plugin",
			  (gpointer *)&sound_module->plugin)) {
	g_warning (g_module_error ());
	g_module_close (sound_module->library);
      
	return FALSE;
    }

    return TRUE;
}

static void
gnome_sound_module_unload (GTypeModule *module)
{
    GnomeSoundModule *sound_module = GNOME_SOUND_MODULE (module);

#if 0
    sound_module->exit();
#endif

    g_module_close (sound_module->library);
    sound_module->library = NULL;
}

/* This only will ever be called if an error occurs during
 * initialization
 */
static void
gnome_sound_module_finalize (GObject *object)
{
    GnomeSoundModule *module = GNOME_SOUND_MODULE (object);

    g_free (module->path);

    module_parent_class->finalize (object);
}

static void
gnome_sound_module_class_init (GnomeSoundModuleClass *klass)
{
    GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    module_parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
  
    module_class->load = gnome_sound_module_load;
    module_class->unload = gnome_sound_module_unload;

    gobject_class->finalize = gnome_sound_module_finalize;
}

GType
gnome_sound_module_get_type (void)
{
    static GType sound_module_type = 0;

    if (!sound_module_type) {
	static const GTypeInfo sound_module_info = {
	    sizeof (GnomeSoundModuleClass),
	    NULL,           /* base_init */
	    NULL,           /* base_finalize */
	    (GClassInitFunc) gnome_sound_module_class_init,
	    NULL,           /* class_finalize */
	    NULL,           /* class_data */
	    sizeof (GnomeSoundModule),
	    0,              /* n_preallocs */
	    NULL,           /* instance_init */
	};

	sound_module_type = g_type_register_static (G_TYPE_TYPE_MODULE, "GnomeSoundModule",
						    &sound_module_info, 0);
    }
  
    return sound_module_type;
}

static void
_gnome_sound_error_nodriver (GError **error)
{
    g_set_error (error, GNOME_SOUND_ERROR,
		 GNOME_SOUND_ERROR_NODRIVER,
		 _("No sound driver loaded"));
}

GQuark
gnome_sound_error_quark (void)
{
    static GQuark q = 0;

    if (q == 0)
	q = g_quark_from_static_string ("gnome-sound-error-quark");

    return q;
}

GnomeSoundDriver *
gnome_sound_get_default_driver (void)
{
    return g_object_ref (G_OBJECT (sound_driver));
}

GnomeSoundDriver *
gnome_sound_init (const gchar *name, const gchar *backend_args, GError **error)
{
    GnomeSoundDriver *driver = NULL;
    GnomeSoundModule *module;

    module = g_object_new (GNOME_TYPE_SOUND_MODULE, NULL);
    module->path = g_strdup_printf (LIBGNOME_SOUNDPLUGINDIR
				    "/gnomesoundplugin_%s",
				    name);
    g_type_module_set_name (G_TYPE_MODULE (module), module->path);

    if (g_type_module_use (G_TYPE_MODULE (module))) {
	driver = module->plugin->init (G_TYPE_MODULE (module),
				       backend_args, error);
	g_type_module_unuse (G_TYPE_MODULE (module));
    } else {
	g_warning (G_STRLOC ": Loading sound module '%s' failed", name);
	_gnome_sound_error_nodriver (error);
    }

    return driver;
}

gboolean
gnome_sound_enabled (void)
{
    return sound_module != NULL;
}
