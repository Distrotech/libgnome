#ifndef __GNOME_SOUND_H__
#define __GNOME_SOUND_H__ 1

#include "gnome-defs.h"
#include <glib.h>
BEGIN_GNOME_DECLS

/*
 * Copyright (C) 1998 Stuart Parmenter and Elliot Lee
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option) 
 * any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

/* Use this with the Esound functions */
extern int gnome_sound_connection;

/* Initialize esd connection */
void gnome_sound_init(const char *hostname);

/* Closes esd connection */
void gnome_sound_shutdown(void);

/* Returns the Esound sample ID for the sample */
int gnome_sound_sample_load(const char *sample_name, const char *filename);

/* Loads sample, plays sample, frees sample */
void gnome_sound_play (const char * filename);

END_GNOME_DECLS

#endif /* __GNOME_SOUND_H__ */
