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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib.h>
#include <libgnome/gnome-sound.h>

#include <csl/csl.h>
#include <csl/cslsample.h>

typedef struct _GnomeSoundDriverCSL GnomeSoundDriverCSL;

struct _GnomeSoundDriverCSL {
    GnomeSoundDriver driver;

    GPtrArray *active_samples;
    CslDriver *csl_driver;
};

struct _GnomeSoundSample {
    CslSample *sample;
};

GType type_csl_driver = 0;

static void csl_class_init (GnomeSoundDriverClass *klass);
static void csl_init (GnomeSoundDriverCSL *driver);

static void
csl_register_type (GTypeModule *module)
{
    static const GTypeInfo object_info = {
	sizeof (GnomeSoundDriverClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) csl_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (GnomeSoundDriverCSL),
	0,
	(GInstanceInitFunc) csl_init,
    };

    type_csl_driver =  g_type_module_register_type
	(module, GNOME_TYPE_SOUND_DRIVER, "GnomeSoundDriverCSL",
	 &object_info, 0);
}

#define GNOME_TYPE_SOUND_DRIVER_CSL              (type_csl_driver)
#define GNOME_SOUND_DRIVER_CSL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SOUND_DRIVER_CSL, GnomeSoundDriverCSL))
#define GNOME_SOUND_DRIVER_CSL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SOUND_DRIVER_CSL, GnomeSoundDriverClass))
#define GNOME_IS_SOUND_DRIVER_CSL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_SOUND_DRIVER_CSL))
#define GNOME_IS_SOUND_DRIVER_CSL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SOUND_DRIVER_CSL))
#define GNOME_SOUND_DRIVER_CSL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_SOUND_DRIVER_CSL, GnomeSoundDriverClass))


static gboolean
gnome_sound_csl_init (GnomeSoundDriver *driver, const gchar *backend_args, GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);
    const gchar *name = "arts";
    CslErrorType err;

    if (csl->csl_driver) {
	g_warning ("Sound system already initialized!");
	return TRUE;
    }

    err = csl_driver_init_mutex (name,
				 CSL_DRIVER_CAP_PCM|CSL_DRIVER_CAP_SAMPLE,
				 NULL, &csl->csl_driver);

    if (err) {
	g_warning ("Failed to initialize %s sound driver: %s (%d)",
		   name ? name : "auto-selected", csl_strerror (err), err);
	return FALSE;
    }

    csl->active_samples = g_ptr_array_new ();
    return TRUE;
}

static void
gnome_sound_csl_play_file (GnomeSoundDriver *driver,
			   const char *filename, GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);
    CslErrorType err;
    CslSample *sample;

    if (!csl->csl_driver)
	return;

    err = csl_sample_new_from_file (csl->csl_driver, filename,
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
gnome_sound_csl_sample_new_from_file (GnomeSoundDriver *driver,
				      const char *name, const char *filename,
				      GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);
    GnomeSoundSample *retval;
    CslErrorType err;
    CslSample *sample;
    int fd;

    if (!csl->csl_driver)
	return NULL;

    err = csl_sample_new (csl->csl_driver, name,
			  "gnome_sound_csl_sample_new", NULL,
			  &sample);
    if (err) {
	csl_warning ("unable to create sample '%s': %s",
		     name, csl_strerror (err));
	return NULL;
    }

    fd = open (filename, O_RDONLY);

    if (fd >= 0) {
	char buffer[4096];
	int len;		

	do {
	    do
		len = read (fd, buffer, sizeof(buffer));
	    while (len < 0 && errno == EINTR);
	      
	    if (len > 0)
		csl_sample_write (sample, len, buffer);
	}
	while (len > 0);
	close (fd);

	csl_sample_write_done (sample);
    } else {
	csl_sample_release (sample);
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (csl->active_samples, retval);

    return retval;
}

static GnomeSoundSample *
gnome_sound_csl_sample_new_from_cache (GnomeSoundDriver *driver,
				       const char *name, GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);
    GnomeSoundSample *retval;
    CslErrorType err;
    CslSample *sample;

    if (!csl->csl_driver)
	return NULL;

    err = csl_sample_new_from_cache (csl->csl_driver, name,
				    "gnome_sound_csl_sample_new", NULL,
				    &sample);
    if (err) {
	csl_warning ("unable to create sample from cache '%s': %s",
		     name, csl_strerror (err));
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (csl->active_samples, retval);

    return retval;
}

static GnomeSoundSample *
gnome_sound_csl_sample_new (GnomeSoundDriver *driver,
			    const char *sample_name, GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);
    GnomeSoundSample *retval;
    CslErrorType err;
    CslSample *sample;

    if (!csl->csl_driver)
	return NULL;

    err = csl_sample_new (csl->csl_driver, sample_name,
			  "gnome_sound_csl_play", NULL,
			  &sample);
    if (err) {
	csl_warning ("unable to create sample '%s': %s",
		     sample_name, csl_strerror (err));
	return NULL;
    }

    retval = g_new0 (GnomeSoundSample, 1);
    retval->sample = sample;

    g_ptr_array_add (csl->active_samples, retval);

    return retval;
}

static void
gnome_sound_csl_cache_add_sample (GnomeSoundDriver *driver,
				  GnomeSoundSample *gs, GError **error)
{
    csl_sample_cache_add (gs->sample);
}

static void
gnome_sound_csl_cache_remove_sample (GnomeSoundDriver *driver,
				     GnomeSoundSample *gs, GError **error)
{
    csl_sample_cache_remove (gs->sample);
}

static int
gnome_sound_csl_sample_write (GnomeSoundDriver *driver,
			      GnomeSoundSample *gs,
			      guint n_bytes, gpointer bytes,
			      GError **error)
{
    return csl_sample_write (gs->sample, n_bytes, bytes);
}

static void
gnome_sound_csl_sample_write_done (GnomeSoundDriver *driver,
				   GnomeSoundSample *gs, GError **error)
{
    csl_sample_write_done (gs->sample);
}

static void
gnome_sound_csl_sample_play (GnomeSoundDriver *driver,
			     GnomeSoundSample *gs, GError **error)
{
    csl_sample_play (gs->sample);
}

static gboolean
gnome_sound_csl_sample_is_playing (GnomeSoundDriver *driver,
				   GnomeSoundSample *gs, GError **error)
{
    return csl_sample_is_playing (gs->sample);
}

static void
gnome_sound_csl_sample_stop (GnomeSoundDriver *driver,
			     GnomeSoundSample *gs, GError **error)
{
    csl_sample_stop (gs->sample);
}

static void
gnome_sound_csl_sample_wait_done (GnomeSoundDriver *driver,
				  GnomeSoundSample *gs, GError **error)
{
    csl_sample_wait_done (gs->sample);
}

static void
gnome_sound_csl_sample_release (GnomeSoundDriver *driver,
				GnomeSoundSample *gs, GError **error)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);

    csl_sample_release (gs->sample);
    g_ptr_array_remove (csl->active_samples, gs);
    g_free (gs);
}

static void
gnome_sound_csl_shutdown (GnomeSoundDriver *driver)
{
    GnomeSoundDriverCSL *csl = GNOME_SOUND_DRIVER_CSL (driver);

    while (csl->active_samples->len) {
	GnomeSoundSample *gs = g_ptr_array_index (csl->active_samples, 0);

	    gnome_sound_csl_sample_stop (driver, gs, NULL);
	    gnome_sound_csl_sample_release (driver, gs, NULL);
    }

    csl_driver_shutdown (csl->csl_driver);
    csl->csl_driver = NULL;
}

static void
csl_class_init (GnomeSoundDriverClass *klass)
{
    klass->init = gnome_sound_csl_init;
    klass->shutdown = gnome_sound_csl_shutdown;

    klass->play_file = gnome_sound_csl_play_file;
    klass->sample_new = gnome_sound_csl_sample_new;
    klass->sample_write = gnome_sound_csl_sample_write;
    klass->sample_write_done = gnome_sound_csl_sample_write_done;
    klass->cache_add_sample = gnome_sound_csl_cache_add_sample;
    klass->cache_remove_sample = gnome_sound_csl_cache_remove_sample;
    klass->sample_new_from_file = gnome_sound_csl_sample_new_from_file;
    klass->sample_new_from_cache = gnome_sound_csl_sample_new_from_cache;
    klass->sample_play = gnome_sound_csl_sample_play;
    klass->sample_is_playing = gnome_sound_csl_sample_is_playing;
    klass->sample_stop = gnome_sound_csl_sample_stop;
    klass->sample_wait_done = gnome_sound_csl_sample_wait_done;
    klass->sample_release = gnome_sound_csl_sample_release;
}

static void
csl_init (GnomeSoundDriverCSL *driver)
{
}

static GnomeSoundDriver *
gnome_sound_module_csl_init (GTypeModule *module, const gchar *backend_args, GError **error)
{
    GnomeSoundDriver *driver;
#if 0
    CslErrorType err;
    const gchar *name = "arts";
#endif

    csl_register_type (module);

    driver = g_object_new (type_csl_driver, NULL);
    if (!GNOME_SOUND_DRIVER_GET_CLASS (driver)->init (driver, backend_args, error)) {
	g_object_unref (G_OBJECT (driver));
	return NULL;
    }

    return driver;

#if 0
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
#endif
}

static void
gnome_sound_module_csl_shutdown (void)
{
}

GnomeSoundPlugin gnome_sound_plugin = {
    GNOME_PLUGIN_SERIAL,
    "gnome-sound-csl",
    gnome_sound_module_csl_init,
    gnome_sound_module_csl_shutdown
};
