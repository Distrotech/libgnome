/* libgnome-private.h: Uninstalled private header for libgnome
 * Copyright (C) 2005 Novell, Inc
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __LIBGNOME_PRIVATE_H__
#define __LIBGNOME_PRIVATE_H__

#include <glib.h>

#ifdef G_OS_WIN32

const char *_gnome_get_prefix (void) G_GNUC_CONST;
const char *_gnome_get_localedir (void) G_GNUC_CONST;
const char *_gnome_get_libdir (void) G_GNUC_CONST;
const char *_gnome_get_datadir (void) G_GNUC_CONST;
const char *_gnome_get_localstatedir (void) G_GNUC_CONST;
const char *_gnome_get_sysconfdir (void) G_GNUC_CONST;

#undef LIBGNOME_PREFIX
#define LIBGNOME_PREFIX _gnome_get_prefix ()

#undef LIBGNOME_LOCALEDIR
#define LIBGNOME_LOCALEDIR _gnome_get_localedir ()

#undef LIBGNOME_LIBDIR
#define LIBGNOME_LIBDIR _gnome_get_libdir ()

#undef LIBGNOME_DATADIR
#define LIBGNOME_DATADIR _gnome_get_datadir ()

#undef LIBGNOME_LOCALSTATEDIR
#define LIBGNOME_LOCALSTATEDIR _gnome_get_localstatedir ()

#undef LIBGNOME_SYSCONFDIR
#define LIBGNOME_SYSCONFDIR _gnome_get_sysconfdir ()

#endif

#endif
