/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-gconf.h
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

#include <gconf/gconf-value.h>
#include <gconf/gconf-client.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <libgnome/gnome-program.h>
#include "gnome-color-picker.h"
/* GTK Widgets */
GConfValue *gnome_gconf_gtk_entry_get          (GtkEntry         *entry,
						GConfValueType    type);
void        gnome_gconf_gtk_entry_set          (GtkEntry         *entry,
						GConfValue       *value);
GConfValue *gnome_gconf_spin_button_get        (GtkSpinButton    *spin_button,
						GConfValueType    type);
void        gnome_gconf_spin_button_set        (GtkSpinButton   *spin_button,
						GConfValue      *value);
GConfValue *gnome_gconf_gtk_radio_button_get   (GtkRadioButton   *radio,
						GConfValueType    type);
void        gnome_gconf_gtk_radio_button_set   (GtkRadioButton   *radio,
						GConfValue       *value);
GConfValue *gnome_gconf_gtk_range_get          (GtkRange         *range,
						GConfValueType    type);
void        gnome_gconf_gtk_range_set          (GtkRange         *range,
						GConfValue       *value);
GConfValue *gnome_gconf_gtk_toggle_button_get  (GtkToggleButton  *toggle,
						GConfValueType    type);
void        gnome_gconf_gtk_toggle_button_set  (GtkToggleButton  *toggle,
						GConfValue       *value);


/* GNOME Widgets */
GConfValue *gnome_gconf_gnome_color_picker_get (GnomeColorPicker *picker,
						GConfValueType    type);
void        gnome_gconf_gnome_color_picker_set (GnomeColorPicker *picker,
						GConfValue       *value);
/* Get keys relative to the gnome-libs internal per-app directory and the
   application author per-app directory */
gchar      *gnome_gconf_get_gnome_libs_settings_relative (const gchar *subkey);
gchar      *gnome_gconf_get_app_settings_relative        (const gchar *subkey);

/* GNOME GConf module; basically what this does is
   create a global GConfClient for a GNOME application; it's used
   by libgnomeui, and applications can either use it or create
   their own. However note that signals will be emitted for
   libgnomeui settings and errors! Also the module inits
   GConf
*/

GConfClient *gnome_get_gconf_client (void);

extern GnomeModuleInfo gnome_gconf_module_info;
#define GNOME_GCONF_INIT GNOME_PARAM_MODULE,&gnome_gconf_module_info

#endif



