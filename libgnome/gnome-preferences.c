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
#include <unistd.h>

static void
do_warning (void)
{
	g_warning ("gnome_preferences_* are deprecated and will be removed before GNOME 2.0.\n");
}

#define DEFINE_DESKTOP_PROP_BOOLEAN(c_name, prop_name)  \
gboolean                                                \
gnome_preferences_get_ ## c_name (void)                 \
{                                                       \
	do_warning ();					\
	return FALSE;					\
}                                                       \
void                                                    \
gnome_preferences_set_ ## c_name (gboolean value)       \
{                                                       \
	do_warning ();					\
}

/**
 * Description:
 * Determine whether or not the statusbar is a dialog.
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_dialog, "statusbar-dialog");

/**
 * Description:
 * Determine whether or not the statusbar is interactive.
 *
 * replaced by /desktop/gnome/interface/statusbar-interactive gconf key.
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_interactive, "statusbar-interactive");

/**
 * Description:
 * Determine whether or not the statusbar's meter is on the right-hand side. 
 *
 * replaced by /desktop/gnome/interface/statusbar-meter-on-right gconf key.
 **/
DEFINE_DESKTOP_PROP_BOOLEAN (statusbar_meter_on_right, "statusbar-meter-on-right");

/**
 * Description:
 * Determine whether or not a menu bar is, by default,
 * detachable from its parent frame.
 *
 * replaced by /desktop/gnome/interface/menubar-detachable gconf key.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_detachable, "menubar-detachable");

/**
 * Description:
 *
 * replaced by /desktop/gnome/interface/menubar-relief gconf key.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menubar_relief, "menubar-relief");

/**
 * Description:
 *
 * replaced by /desktop/gnome/interface/toolbar-detachable gconf key.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_detachable, "toolbar-detachable");

/**
 * Description:
 *
 * replaced by /desktop/gnome/interface/toolbar-relief gconf key.
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
 *
 * replaced by /desktop/gnome/interface/toolbar-labels gconf key.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (toolbar_labels, "toolbar-labels");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (dialog_centered, "dialog-centered");

/**
 * Description:
 *
 * replaced by /desktop/gnome/interface/menus-have-tearoff gconf key.
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_tearoff, "menus-have-tearoff");

/**
 * Description:
 */
DEFINE_DESKTOP_PROP_BOOLEAN (menus_have_icons, "menus-have-icons");
