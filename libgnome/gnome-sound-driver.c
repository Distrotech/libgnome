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

#include <libgnome/gnome-sound-driver.h>
#include <libgnome/gnome-i18n.h>

#include <gmodule.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static GObjectClass *parent_class = NULL;

static void
gnome_sound_driver_finalize (GObject *object)
{
    parent_class->finalize (object);
}

static void
gnome_sound_driver_class_init (GnomeSoundDriverClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
  
    gobject_class->finalize = gnome_sound_driver_finalize;
}

GType
gnome_sound_driver_get_type (void)
{
    static GType sound_driver_type = 0;

    if (!sound_driver_type) {
	static const GTypeInfo sound_driver_info = {
	    sizeof (GnomeSoundDriverClass),
	    NULL,           /* base_init */
	    NULL,           /* base_finalize */
	    (GClassInitFunc) gnome_sound_driver_class_init,
	    NULL,           /* class_finalize */
	    NULL,           /* class_data */
	    sizeof (GnomeSoundDriver),
	    0,              /* n_preallocs */
	    NULL,           /* instance_init */
	};

	sound_driver_type = g_type_register_static (G_TYPE_OBJECT, "GnomeSoundDriver",
						    &sound_driver_info, 0);
    }
  
    return sound_driver_type;
}

static void
_gnome_sound_error_notimplemented (GError **error)
{
    g_set_error (error, GNOME_SOUND_ERROR,
		 GNOME_SOUND_ERROR_NOT_IMPLEMENTED,
		 _("Function not implemented"));
}

void 
gnome_sound_driver_play (GnomeSoundDriver *driver, const char *filename, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->play_file)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->play_file (driver, filename, error);
    else
	_gnome_sound_error_notimplemented (error);
}

void 
gnome_sound_driver_cache_add_sample (GnomeSoundDriver *driver,
				     GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->cache_add_sample)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->cache_add_sample (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}

void 
gnome_sound_driver_cache_remove_sample (GnomeSoundDriver *driver,
					GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->cache_remove_sample)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->cache_remove_sample (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}

GnomeSoundSample *
gnome_sound_driver_sample_new_from_file (GnomeSoundDriver *driver,
					 const gchar *name, const char *filename,
					 GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new_from_file)
	return GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new_from_file (driver, name,
									    filename, error);
    else {
	_gnome_sound_error_notimplemented (error);
	return NULL;
    }
}

GnomeSoundSample *
gnome_sound_driver_sample_new_from_cache (GnomeSoundDriver *driver,
					  const char *name, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new_from_cache)
	return GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new_from_cache (driver, name, error);
    else {
	_gnome_sound_error_notimplemented (error);
	return NULL;
    }
}

GnomeSoundSample *
gnome_sound_driver_sample_new (GnomeSoundDriver *driver,
			       const char *sample_name, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new)
	return GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_new (driver, sample_name, error);
    else {
	_gnome_sound_error_notimplemented (error);
	return NULL;
    }
}

int
gnome_sound_driver_sample_write (GnomeSoundDriver *driver, GnomeSoundSample *gs,
				 guint n_bytes, gpointer bytes,
				 GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_write)
	return GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_write (driver, gs, n_bytes, bytes, error);
    else {
	_gnome_sound_error_notimplemented (error);
	return -1;
    }
}

void
gnome_sound_driver_sample_write_done (GnomeSoundDriver *driver,
				      GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_write_done)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_write_done (driver, gs, error);
    else
    _gnome_sound_error_notimplemented (error);
}

void
gnome_sound_driver_sample_play (GnomeSoundDriver *driver,
				GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_play)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_play (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}

gboolean
gnome_sound_driver_sample_is_playing (GnomeSoundDriver *driver,
				      GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_is_playing)
	return GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_is_playing (driver, gs, error);
    else {
	_gnome_sound_error_notimplemented (error);
	return FALSE;
    }
}

void
gnome_sound_driver_sample_stop (GnomeSoundDriver *driver,
				GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_stop)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_stop (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}

void
gnome_sound_driver_sample_wait_done (GnomeSoundDriver *driver,
				     GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_wait_done)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_wait_done (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}

void
gnome_sound_driver_sample_release (GnomeSoundDriver *driver,
				   GnomeSoundSample *gs, GError **error)
{
    if (GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_release)
	GNOME_SOUND_DRIVER_GET_CLASS (driver)->sample_release (driver, gs, error);
    else
	_gnome_sound_error_notimplemented (error);
}
