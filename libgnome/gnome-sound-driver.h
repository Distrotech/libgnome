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

#ifndef __GNOME_SOUND_DRIVER_H__
#define __GNOME_SOUND_DRIVER_H__ 1

#include <glib/gmacros.h>
#include <glib/gerror.h>
#include <gobject/gobject.h>

G_BEGIN_DECLS

typedef struct _GnomeSoundDriver             GnomeSoundDriver;
typedef struct _GnomeSoundDriverClass        GnomeSoundDriverClass;

typedef struct _GnomeSoundSample             GnomeSoundSample;

#define GNOME_TYPE_SOUND_DRIVER              (gnome_sound_driver_get_type ())
#define GNOME_SOUND_DRIVER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_SOUND_DRIVER, GnomeSoundDriver))
#define GNOME_SOUND_DRIVER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_SOUND_DRIVER, GnomeSoundDriverClass))
#define GNOME_IS_SOUND_DRIVER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_SOUND_DRIVER))
#define GNOME_IS_SOUND_DRIVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_SOUND_DRIVER))
#define GNOME_SOUND_DRIVER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_SOUND_DRIVER, GnomeSoundDriverClass))

struct _GnomeSoundDriver {
    GObject object;
};

struct _GnomeSoundDriverClass {
    GObjectClass parent_class;

    gboolean (*init) (GnomeSoundDriver *driver,
		      const gchar *backend_args,
		      GError **error);

    void (*shutdown) (GnomeSoundDriver *driver);

    void (*play_file) (GnomeSoundDriver *driver,
		       const gchar *filename,
		       GError **error);

    GnomeSoundSample * (*sample_new) (GnomeSoundDriver *driver,
				      const gchar *name,
				      GError **error);
    int (*sample_write) (GnomeSoundDriver *driver,
			 GnomeSoundSample *gs,
			 guint n_bytes, gpointer bytes,
			 GError **error);
    void (*sample_write_done) (GnomeSoundDriver *driver,
			       GnomeSoundSample *gs,
			       GError **error);

    void (*cache_add_sample) (GnomeSoundDriver *driver,
			      GnomeSoundSample *sample,
			      GError **error);
    void (*cache_remove_sample) (GnomeSoundDriver *driver,
				 GnomeSoundSample *sample,
				 GError **error);

    GnomeSoundSample * (*sample_new_from_file) (GnomeSoundDriver *driver,
						const gchar *name,
						const gchar *filename,
						GError **error);
    GnomeSoundSample * (*sample_new_from_cache) (GnomeSoundDriver *driver,
						 const gchar *name,
						 GError **error);

    void (*sample_play) (GnomeSoundDriver *driver,
			 GnomeSoundSample *gs,
			 GError **error);
    gboolean (*sample_is_playing) (GnomeSoundDriver *driver,
				   GnomeSoundSample *gs,
				   GError **error);
    void (*sample_stop) (GnomeSoundDriver *driver,
			 GnomeSoundSample *gs,
			 GError **error);
    void (*sample_wait_done) (GnomeSoundDriver *driver,
			      GnomeSoundSample *gs,
			      GError **error);
    void (*sample_release) (GnomeSoundDriver *driver,
			    GnomeSoundSample *gs,
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

GType gnome_sound_driver_get_type (void) G_GNUC_CONST;

#define GNOME_SOUND_ERROR gnome_sound_error_quark ()
GQuark gnome_sound_error_quark (void) G_GNUC_CONST;

void
gnome_sound_driver_play (GnomeSoundDriver *driver,
			 const char *filename,
			 GError **error);

void
gnome_sound_driver_cache_add_sample (GnomeSoundDriver *driver,
				     GnomeSoundSample *sample, GError **error);

void
gnome_sound_driver_cache_remove_sample (GnomeSoundDriver *driver,
					GnomeSoundSample *sample, GError **error);

GnomeSoundSample *
gnome_sound_driver_sample_new_from_file (GnomeSoundDriver *driver,
					 const char *name,
					 const char *filename,
					 GError **error);

GnomeSoundSample *
gnome_sound_driver_sample_new_from_cache (GnomeSoundDriver *driver,
					  const char *name,
					  GError **error);

GnomeSoundSample *
gnome_sound_driver_sample_new (GnomeSoundDriver *driver,
			       const char *sample_name,
			       GError **error);

int
gnome_sound_driver_sample_write (GnomeSoundDriver *driver,
				 GnomeSoundSample *gs,
				 guint n_bytes, gpointer bytes,
				 GError **error);

void
gnome_sound_driver_sample_write_done (GnomeSoundDriver *driver,
				      GnomeSoundSample *gs,
				      GError **error);

void
gnome_sound_driver_sample_play (GnomeSoundDriver *driver,
				GnomeSoundSample *gs,
				GError **error);

gboolean
gnome_sound_driver_sample_is_playing (GnomeSoundDriver *driver,
				      GnomeSoundSample *gs,
				      GError **error);

void
gnome_sound_driver_sample_stop (GnomeSoundDriver *driver,
				GnomeSoundSample *gs,
				GError **error);

void
gnome_sound_driver_sample_wait_done (GnomeSoundDriver *driver,
				     GnomeSoundSample *gs,
				     GError **error);

void
gnome_sound_driver_sample_release (GnomeSoundDriver *driver,
				   GnomeSoundSample *gs,
				   GError **error);

G_END_DECLS

#endif /* __GNOME_SOUND_DRIVER_H__ */
