/*
 * Copyright (C) 2001 SuSE Linux AG
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
#include "gnome-sound.h"

#include <csl/csl.h>
#include <csl/cslsample.h>

struct _GnomeSoundSample {
    CslSample *sample;
};

static GPtrArray *active_samples = NULL;
static CslDriver *gnome_sound_csl_driver = NULL;

static void
gnome_sound_csl_play_file (const char *filename, GError **error)
{
    CslErrorType err;
    CslSample *sample;

    if (!gnome_sound_csl_driver)
	return;

    err = csl_sample_new_from_file (gnome_sound_csl_driver, filename,
				    "gnome_sound_csl_play", NULL,
				    &sample);
    if (err) {
	csl_warning ("unable to create sample from file '%s': %s",
		     filename, csl_strerror (err));
	return;
    }

    csl_sample_play (sample);
    csl_sample_release (sample);
}

static GnomeSoundSample *
gnome_sound_csl_sample_new_from_file (const char *filename, GError **error)
{
    CslErrorType err;
    CslSample *sample;
    GnomeSoundSample *retval;

    if (!gnome_sound_csl_driver)
	return NULL;

    err = csl_sample_new_from_file (gnome_sound_csl_driver, filename,
				    "gnome_sound_csl_sample_new", NULL,
				    &sample);
    if (err) {
	csl_warning ("unable to create sample from file '%s': %s",
		     filename, csl_strerror (err));
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (active_samples, retval);

    return retval;
}

static GnomeSoundSample *
gnome_sound_csl_sample_new_from_cache (const char *name, GError **error)
{
    CslErrorType err;
    CslSample *sample;
    GnomeSoundSample *retval;

    if (!gnome_sound_csl_driver)
	return NULL;

    err = csl_sample_new_from_cache (gnome_sound_csl_driver, name,
				    "gnome_sound_csl_sample_new", NULL,
				    &sample);
    if (err) {
	csl_warning ("unable to create sample from cache '%s': %s",
		     name, csl_strerror (err));
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (active_samples, retval);

    return retval;
}

static GnomeSoundSample *
gnome_sound_csl_sample_new (const char *sample_name, GError **error)
{
    CslErrorType err;
    CslSample *sample;
    GnomeSoundSample *retval;

    if (!gnome_sound_csl_driver)
	return NULL;

    err = csl_sample_new (gnome_sound_csl_driver, sample_name,
			  "gnome_sound_csl_play", NULL,
			  &sample);
    if (err) {
	csl_warning ("unable to create sample '%s': %s",
		     sample_name, csl_strerror (err));
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (active_samples, retval);

    return retval;
}

static void
gnome_sound_csl_cache_add_sample (GnomeSoundSample *gs, GError **error)
{
    csl_sample_cache_add (gs->sample);
}

static void
gnome_sound_csl_cache_remove_sample (GnomeSoundSample *gs, GError **error)
{
    csl_sample_cache_remove (gs->sample);
}

static int
gnome_sound_csl_sample_write (GnomeSoundSample *gs,
			      guint n_bytes, gpointer bytes,
			      GError **error)
{
    return csl_sample_write (gs->sample, n_bytes, bytes);
}

static void
gnome_sound_csl_sample_write_done (GnomeSoundSample *gs, GError **error)
{
    csl_sample_write_done (gs->sample);
}

static void
gnome_sound_csl_sample_play (GnomeSoundSample *gs, GError **error)
{
    csl_sample_play (gs->sample);
}

static gboolean
gnome_sound_csl_sample_is_playing (GnomeSoundSample *gs, GError **error)
{
    return csl_sample_is_playing (gs->sample);
}

static void
gnome_sound_csl_sample_stop (GnomeSoundSample *gs, GError **error)
{
    csl_sample_stop (gs->sample);
}

static void
gnome_sound_csl_sample_wait_done (GnomeSoundSample *gs, GError **error)
{
    csl_sample_wait_done (gs->sample);
}

static void
gnome_sound_csl_sample_release (GnomeSoundSample *gs, GError **error)
{
    csl_sample_release (gs->sample);
    g_ptr_array_remove (active_samples, gs);
    g_free (gs);
}

static gboolean
gnome_sound_csl_init (const gchar *backend_args, GError **error)
{
    CslErrorType err;
    const gchar *name = "arts";

    if (gnome_sound_csl_driver) {
	g_warning ("Sound system already initialized!");
	return TRUE;
    }

    err = csl_driver_init_mutex (name,
				 CSL_DRIVER_CAP_PCM|CSL_DRIVER_CAP_SAMPLE,
				 NULL, &gnome_sound_csl_driver);

    if (err) {
	g_warning ("Failed to initialize %s sound driver: %s (%d)",
		   name ? name : "auto-selected", csl_strerror (err), err);
	return FALSE;
    }

    active_samples = g_ptr_array_new ();
    return TRUE;
}

static void
gnome_sound_csl_shutdown (GError **error)
{
    if (gnome_sound_csl_driver) {

	while (active_samples->len) {
	    GnomeSoundSample *gs = g_ptr_array_index (active_samples, 0);

	    gnome_sound_csl_sample_stop (gs, NULL);
	    gnome_sound_csl_sample_release (gs, NULL);
	}

	csl_driver_shutdown (gnome_sound_csl_driver);
	gnome_sound_csl_driver = NULL;
    }
}

GnomeSoundPlugin gnome_sound_plugin = {
    GNOME_PLUGIN_SERIAL,
    "gnome-sound-csl",
    gnome_sound_csl_init,
    gnome_sound_csl_shutdown,
    gnome_sound_csl_play_file,
    gnome_sound_csl_sample_new,
    gnome_sound_csl_sample_write,
    gnome_sound_csl_sample_write_done,
    gnome_sound_csl_cache_add_sample,
    gnome_sound_csl_cache_remove_sample,
    gnome_sound_csl_sample_new_from_file,
    gnome_sound_csl_sample_new_from_cache,
    gnome_sound_csl_sample_play,
    gnome_sound_csl_sample_is_playing,
    gnome_sound_csl_sample_stop,
    gnome_sound_csl_sample_wait_done,
    gnome_sound_csl_sample_release
};
