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

static const GnomeSoundPlugin *sound_plugin = NULL;
static GModule *sound_plugin_module = NULL;

static void
_gnome_sound_error_nodriver (GError **error)
{
    g_set_error (error, GNOME_SOUND_ERROR,
		 GNOME_SOUND_ERROR_NODRIVER,
		 _("No sound driver loaded"));
}

void 
gnome_sound_play (const char *filename, GError **error)
{
    if (sound_plugin)
	sound_plugin->play_file (filename, error);
    else
	_gnome_sound_error_nodriver (error);
}

void 
gnome_sound_cache_add_sample (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->cache_add_sample (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

void 
gnome_sound_cache_remove_sample (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->cache_remove_sample (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

GnomeSoundSample *
gnome_sound_sample_new_from_file (const gchar *name, const char *filename,
				  GError **error)
{
    if (sound_plugin)
	return sound_plugin->sample_new_from_file (name, filename, error);
    else {
	_gnome_sound_error_nodriver (error);
	return NULL;
    }
}

GnomeSoundSample *
gnome_sound_sample_new_from_cache (const char *name, GError **error)
{
    if (sound_plugin)
	return sound_plugin->sample_new_from_cache (name, error);
    else {
	_gnome_sound_error_nodriver (error);
	return NULL;
    }
}

GnomeSoundSample *
gnome_sound_sample_new (const char *sample_name, GError **error)
{
    if (sound_plugin)
	return sound_plugin->sample_new (sample_name, error);
    else {
	_gnome_sound_error_nodriver (error);
	return NULL;
    }
}

int
gnome_sound_sample_write (GnomeSoundSample *gs,
			  guint n_bytes, gpointer bytes,
			  GError **error)
{
    if (sound_plugin)
	return sound_plugin->sample_write (gs, n_bytes, bytes, error);
    else {
	_gnome_sound_error_nodriver (error);
	return -1;
    }
}

void
gnome_sound_sample_write_done (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->sample_write_done (gs, error);
    else
    _gnome_sound_error_nodriver (error);
}

void
gnome_sound_sample_play (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->sample_play (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

gboolean
gnome_sound_sample_is_playing (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	return sound_plugin->sample_is_playing (gs, error);
    else {
	_gnome_sound_error_nodriver (error);
	return FALSE;
    }
}

void
gnome_sound_sample_stop (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->sample_stop (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

void
gnome_sound_sample_wait_done (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->sample_wait_done (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

void
gnome_sound_sample_release (GnomeSoundSample *gs, GError **error)
{
    if (sound_plugin)
	sound_plugin->sample_release (gs, error);
    else
	_gnome_sound_error_nodriver (error);
}

GQuark
gnome_sound_error_quark (void)
{
    static GQuark q = 0;

    if (q == 0)
	q = g_quark_from_static_string ("gnome-sound-error-quark");

    return q;
}

void
gnome_sound_init (const gchar *name, const gchar *backend_args,
		  const GnomeSoundPlugin *plugin, GError **error)
{
    if (sound_plugin)
	return;

    if (plugin)
	sound_plugin = plugin;
    else {
	gchar *module_name;
	gpointer plugin_ptr = NULL;
	GModule *module;

	module_name = g_strdup_printf (LIBGNOME_SOUNDPLUGINDIR
				       "/gnomesoundplugin_%s",
				       name);
	module = g_module_open (module_name, G_MODULE_BIND_LAZY);
	if (!module) {
	    g_warning (G_STRLOC ": Can't open sound plugin `%s': %s",
		       name, g_module_error ());
	    return;
	}

	if (!g_module_symbol (module, "gnome_sound_plugin", &plugin_ptr)) {
	    g_warning (G_STRLOC ": Not a valid sound plugin: `%s'", name);
	    g_module_close (module);
	    return;
	}

	sound_plugin = plugin_ptr;
	sound_plugin_module = module;
    }

    if (sound_plugin->version != GNOME_PLUGIN_SERIAL) {
	g_warning (G_STRLOC ": Sound plugin `%s' has invalid version %ld",
		   name, sound_plugin->version);
	sound_plugin = NULL;

	if (sound_plugin_module) {
	    g_module_close (sound_plugin_module);
	    sound_plugin_module = NULL;
	}
    }

    if (sound_plugin)
	sound_plugin->init (backend_args, error);
    else
	_gnome_sound_error_nodriver (error);
}

void
gnome_sound_shutdown (GError **error)
{
    if (sound_plugin)
	sound_plugin->shutdown (error);
    else
	_gnome_sound_error_nodriver (error);

    sound_plugin = NULL;
    if (sound_plugin_module) {
	g_module_close (sound_plugin_module);
	sound_plugin_module = NULL;
    }
}

gboolean
gnome_sound_enabled (void)
{
    return sound_plugin != NULL;
}
