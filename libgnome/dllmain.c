/* dllmain.c: DLL entry point for libgnome on Win32
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

#include <config.h>
#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include <glib.h>

const char *_libgnome_prefix;
const char *_libgnome_libdir;
const char *_libgnome_datadir;
const char *_libgnome_bindir;
const char *_libgnome_localstatedir;
const char *_libgnome_sysconfdir;
const char *_libgnome_localedir;

static char *
_libgnome_replace_prefix (const char *configure_time_path)
{
  if (strncmp (configure_time_path, LIBGNOME_PREFIX "/", strlen (LIBGNOME_PREFIX) + 1) == 0)
    {
      return g_strconcat (_libgnome_prefix,
			  configure_time_path + strlen (LIBGNOME_PREFIX),
			  NULL);
    }
  else
    return g_strdup (configure_time_path);
}

/* DllMain function needed to fetch the DLL name and deduce the
 * installation directory from that, and then form the pathnames for
 * various directories relative to the installation directory.
 */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  wchar_t wcbfr[1000];
  char cpbfr[1000];
  char *dll_name = NULL;
  
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    /* GLib 2.6 uses UTF-8 file names */
    if (GetVersion () < 0x80000000) {
      /* NT-based Windows has wide char API */
      if (GetModuleFileNameW ((HMODULE) hinstDLL,
			      wcbfr, G_N_ELEMENTS (wcbfr)))
	dll_name = g_utf16_to_utf8 (wcbfr, -1,
				    NULL, NULL, NULL);
    } else {
      /* Win9x, yecch */
      if (GetModuleFileNameA ((HMODULE) hinstDLL,
			      cpbfr, G_N_ELEMENTS (cpbfr)))
	dll_name = g_locale_to_utf8 (cpbfr, -1,
				     NULL, NULL, NULL);
    }

    if (dll_name) {
      gchar *p = strrchr (dll_name, '\\');
      
      if (p != NULL)
	*p = '\0';
      
      p = strrchr (dll_name, '\\');
      if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
	*p = '\0';
      
      _libgnome_prefix = dll_name;
    } else {
      _libgnome_prefix = g_strdup ("");
    }

    _libgnome_libdir = _libgnome_replace_prefix (LIBGNOME_LIBDIR);
    _libgnome_datadir = _libgnome_replace_prefix (LIBGNOME_DATADIR);
    _libgnome_bindir = _libgnome_replace_prefix (LIBGNOME_BINDIR);
    _libgnome_localstatedir = _libgnome_replace_prefix (LIBGNOME_LOCALSTATEDIR);
    _libgnome_sysconfdir = _libgnome_replace_prefix (LIBGNOME_SYSCONFDIR);
    _libgnome_localedir = _libgnome_replace_prefix (LIBGNOME_LOCALEDIR);
    break;
  }

  return TRUE;
}
