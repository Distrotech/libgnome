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

#ifndef __GNOME_SOUND_H__
#define __GNOME_SOUND_H__ 1

#include <glib/gmacros.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

typedef struct _GnomeSoundSample GnomeSoundSample;
typedef struct _GnomeSoundPlugin GnomeSoundPlugin;

struct _GnomeSoundPlugin {
    gulong version;
    const gchar *name;

    gboolean (*init) (const gchar *backend_args,
		      GError **error);
    void (*shutdown) (GError **error);

    void (*play_file) (const gchar *filename,
		       GError **error);

    GnomeSoundSample * (*sample_new) (const gchar *name,
				      GError **error);
    int (*sample_write) (GnomeSoundSample *gs,
			 guint n_bytes, gpointer bytes,
			 GError **error);
    void (*sample_write_done) (GnomeSoundSample *gs,
			       GError **error);

    void (*cache_add_sample) (GnomeSoundSample *sample,
			      GError **error);
    void (*cache_remove_sample) (GnomeSoundSample *sample,
				 GError **error);

    GnomeSoundSample * (*sample_new_from_file) (const gchar *name,
						const gchar *filename,
						GError **error);
    GnomeSoundSample * (*sample_new_from_cache) (const gchar *name,
						 GError **error);

    void (*sample_play) (GnomeSoundSample *gs,
			 GError **error);
    gboolean (*sample_is_playing) (GnomeSoundSample *gs,
				   GError **error);
    void (*sample_stop) (GnomeSoundSample *gs,
			 GError **error);
    void (*sample_wait_done) (GnomeSoundSample *gs,
			      GError **error);
    void (*sample_release) (GnomeSoundSample *gs,
			    GError **error);
};

typedef enum
{
    /* no sound driver */
    GNOME_SOUND_ERROR_NODRIVER,
    /* not implemented */
    GNOME_SOUND_ERROR_NOT_IMPLEMENTED,
    /* device busy */
    GNOME_SOUND_ERROR_DEVICE_BUSY,
    /* I/O eror */
    GNOME_SOUND_ERROR_IO,
    /* Insufficient permissions */
    GNOME_SOUND_ERROR_PERMS,
    /* misc error */
    GNOME_SOUND_ERROR_MISC
} GnomeSoundError;

#define GNOME_SOUND_ERROR gnome_sound_error_quark ()
GQuark gnome_sound_error_quark (void) G_GNUC_CONST;

void gnome_sound_init (const gchar *driver_name,
		       const gchar *backend_args,
		       const GnomeSoundPlugin *opt_plugin,
		       GError **error);

void gnome_sound_shutdown (GError **error);

gboolean gnome_sound_enabled (void);

void gnome_sound_play (const char *filename,
		       GError **error);

void
gnome_sound_cache_add_sample (GnomeSoundSample *sample, GError **error);

void
gnome_sound_cache_remove_sample (GnomeSoundSample *sample, GError **error);

GnomeSoundSample *
gnome_sound_sample_new_from_file (const char *name,
				  const char *filename,
				  GError **error);

GnomeSoundSample *
gnome_sound_sample_new_from_cache (const char *name,
				   GError **error);

GnomeSoundSample *
gnome_sound_sample_new (const char *sample_name,
			GError **error);

int
gnome_sound_sample_write (GnomeSoundSample *gs,
			  guint n_bytes, gpointer bytes,
			  GError **error);

void
gnome_sound_sample_write_done (GnomeSoundSample *gs,
			       GError **error);

void
gnome_sound_sample_play (GnomeSoundSample *gs,
			 GError **error);

gboolean
gnome_sound_sample_is_playing (GnomeSoundSample *gs,
			       GError **error);

void
gnome_sound_sample_stop (GnomeSoundSample *gs,
			 GError **error);

void
gnome_sound_sample_wait_done (GnomeSoundSample *gs,
			      GError **error);

void
gnome_sound_sample_release (GnomeSoundSample *gs,
			    GError **error);

G_END_DECLS

#endif /* __GNOME_SOUND_H__ */
