/* gnome-url.h
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
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
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_URL_H
#define GNOME_URL_H
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* This function displays the given URL in the appropriate viewer.  The
 * Appropriate viewer is user definable, according to these rules:
 *  1) Extract the protocol from URL.  This is defined as everything before
 *     the first colon
 *  2) Check if the key /Gnome/URL Handlers/<protocol>-show exists in the
 *     gnome config database.  If it does, use use this as a command
 *     template.  If it doesn't, check for the key
 *     /Gnome/URL Handlers/default-show, and if that doesn't exist fallback
 *     on the compiled in default.
 *  3) substitute the %s in the template with the URL.
 *  4) call gnome_execute_shell, with this expanded command as the second
 *     argument.
 */
void gnome_url_show(const char *url);

END_GNOME_DECLS
#endif
