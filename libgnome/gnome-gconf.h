/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME Library - gnome-gconf.h
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef GNOME_GCONF_H
#define GNOME_GCONF_H

#include <gconf/gconf-client.h>

#include <libgnome/gnome-program.h>
#include <libgnome/gnome-init.h>

/* Get keys relative to the gnome-libs internal per-app directory and the
   application author per-app directory */
gchar      *gnome_gconf_get_gnome_libs_settings_relative (const gchar *subkey);
gchar      *gnome_gconf_get_app_settings_relative        (GnomeProgram *program,
							  const gchar *subkey);

/* GNOME GConf module; basically what this does is
   create a global GConfClient for a GNOME application; it's used
   by libgnome*, and applications can either use it or create
   their own. However note that signals will be emitted for
   libgnomeui settings and errors! Also the module inits
   GConf
*/

extern GnomeModuleInfo gnome_gconf_module_info;

#endif



