/* GNOME GUI Library: gnome-preferences.c
 * Copyright (C) 1998 Free Software Foundation
 * Author: Havoc Pennington
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

#include <config.h>
#include <libgnome/gnome-preferences.h>
#include <libgnome/gnome-init.h>
#include <bonobo/bonobo-property-bag-client.h>
#include <string.h>

static gboolean
desktop_property_get_boolean (const gchar *name)
{
    Bonobo_ConfigDatabase cb;

    cb = gnome_program_get_desktop_config_database (gnome_program_get ());
    if (cb == CORBA_OBJECT_NIL)
	return FALSE;

    return bonobo_pbclient_get_boolean (cb, name, NULL);
}

static void
desktop_property_set_boolean (const gchar *name, gboolean value)
{
    Bonobo_ConfigDatabase cb;

    cb = gnome_program_get_desktop_config_database (gnome_program_get ());
    if (cb == CORBA_OBJECT_NIL)
	return;

    bonobo_pbclient_set_boolean (cb, name, value, NULL);
}

#define DEFINE_DESKTOP_PROP_BOOLEAN(c_name, prop_name)  \
gboolean                                                \
gnome_preferences_get_ ## c_name (void)                 \
{                                                       \
    return desktop_property_get_boolean (prop_name);    \
}                                                       \
void                                                    \
gnome_preferences_set_ ## c_name (gboolean value)       \
{                                                       \
    desktop_property_set_boolean (prop_name, value);    \
}

/**
 * Description:
 * Determine whether or not the statusbar is a dialog.
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_dialog, "statusbar-dialog");

/**
 * Description:
 * Determine whether or not the statusbar is interactive.
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_interactive, "statusbar-interactive");

/**
 * Description:
 * Determine whether or not the statusbar's meter is on the right-hand side. 
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_meter_on_right, "statusbar-meter-on-right");

/**
 * Description:
 * Determine whether or not a menu bar is, by default,
 * detachable from its parent frame.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_detachable, "menubar-detachable");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_relief, "menubar-relief");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_detachable, "toolbar-detachable");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_relief, "toolbar-relief");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_relief_btn, "toolbar-relief-btn");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_lines, "toolbar-lines");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_labels, "toolbar-labels");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (dialog_centered, "dialog-centered");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_tearoff, "menus-have-tearoff");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_icons, "menus-have-icons");
