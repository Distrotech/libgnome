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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __LIBGNOME_PRIVATE_H__
#define __LIBGNOME_PRIVATE_H__

#include <glib.h>

#ifdef G_OS_WIN32

#undef LIBGNOME_PREFIX
extern const char *_libgnome_prefix;
#define LIBGNOME_PREFIX _libgnome_prefix

#undef LIBGNOME_LIBDIR
extern const char *_libgnome_libdir;
#define LIBGNOME_LIBDIR _libgnome_libdir

#undef LIBGNOME_DATADIR
extern const char *_libgnome_datadir;
#define LIBGNOME_DATADIR _libgnome_datadir

#undef LIBGNOME_BINDIR
extern const char *_libgnome_bindir;
#define LIBGNOME_BINDIR _libgnome_bindir

#undef LIBGNOME_LOCALSTATEDIR
extern const char *_libgnome_localstatedir;
#define LIBGNOME_LOCALSTATEDIR _libgnome_localstatedir

#undef LIBGNOME_SYSCONFDIR
extern const char *_libgnome_sysconfdir;
#define LIBGNOME_SYSCONFDIR _libgnome_sysconfdir

#undef LIBGNOME_LOCALEDIR
extern const char *_libgnome_localedir;
#define LIBGNOME_LOCALEDIR _libgnome_localedir

#endif

#endif
