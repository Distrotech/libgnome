/* -*- Mode: C; tab-width: 8 -*-
   gnome-dl.c --- dynamic loading for gnome.
   Copyright (C) 1998 The Free Software Foundation, all rights reserved.
   Created: Chris Toshok <toshok@hungry.com>, 24-Jan-1998.
 */

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#include "glib.h"
#include "gnome-defs.h"
#include "gnome-util.h"
#include "gnome-dl.h"

/* this stuff should be autoconf'ed */
#ifdef __FreeBSD__
/* what about -current and ELF? */
# define NEED_UNDERSCORE
#endif

#ifdef hpux
# define LIB_SUFFIX ".sl"
#else
# define LIB_SUFFIX ".so"
#endif

#if defined HAVE_DLFCN_H
# include <dlfcn.h>
# ifdef RTLD_LAZY
#  define LAZY_BIND_MODE RTLD_LAZY
# else
#  define LAZY_BIND_MODE 1
# endif
# ifdef RTLD_NOW
#  define NOW_BIND_MODE RTLD_NOW
# else
#  error what is RTLD_NOW supposed to be?
# endif
#elif defined HAVE_DL_H
# include <dl.h>
# define LAZY_BIND_MODE (BIND_DEFERRED | BIND_NONFATAL)
# define NOW_BIND_MODE BIND_IMMEDIATE 
#endif

struct GnomeLibHandle {
  char *full_path;

  int ref_count;

  void *native_handle;
};

/* a cache of loaded libraries */
static GList *_libs = NULL;

/* the error returned by the last gnome_dl_* call.  equivalent to dlerror on platforms that
   have it. */
static char *last_error;

/* a temporary error buffer used through the file to format error messages. */
static char err_buf[256];

static void
fill_in_last_error(char *error)
{
#ifdef HAVE_DLERROR
  last_error = dlerror();
#else
  last_error = error;
#endif
}

static void
clear_last_error()
{
  last_error = NULL;
}

static GnomeLibHandle*
gnome_dl_find_lib(char *libpath)
{
  GList *cur;

  for (cur = _libs;
       cur && cur->data;
       cur = g_list_next(cur))
    {
      GnomeLibHandle *handle = (GnomeLibHandle*)cur->data;

      if (!strcmp(libpath, handle->full_path))
	{
	  return handle;
	}
    }

  return NULL;
}

GnomeLibHandle*
gnome_dl_load(char *libpath, int bind_lazy)
{
  GnomeLibHandle *handle = gnome_dl_find_lib(libpath);

  if (handle == NULL)
    {
      void *native_handle;

#if defined( HAVE_DLFCN_H )
      native_handle = dlopen(libpath, bind_lazy ? LAZY_BIND_MODE : NOW_BIND_MODE);
#elif defined( HAVE_DL_H )
      native_handle = shl_load(lib_path, bind_lazy ? LAZY_BIND_MODE : NOW_BIND_MODE, 0L);
#endif

      if (native_handle == NULL)
	{
#ifdef HAVE_ERRNO_H
	  g_snprintf(err_buf, 256, "unable to load library: %s (err = %d)", libpath, errno);
#else
	  g_snprintf(err_buf, 256, "unable to load library: %s", libpath);
#endif

	  fill_in_last_error(err_buf);
	  return NULL;
	}
      else
	{
	  clear_last_error();
	}

      handle = g_new(struct GnomeLibHandle, 1);
      handle->full_path = g_strdup(libpath);
      handle->ref_count = 0;
      handle->native_handle = native_handle;

      if (_libs == NULL) _libs = g_list_alloc();
      g_list_append(_libs, handle);
    }

  handle->ref_count++;

  return handle;
}

GnomeLibHandle*
gnome_dl_exec_handle()
{
  GnomeLibHandle *handle = gnome_dl_find_lib("<executable>");
  void *native_handle;

  if (handle == NULL)
    {
      void *native_handle = NULL;
      GnomeLibHandle *handle;

#if defined( HAVE_DLFCN_H )

      native_handle = dlopen(NULL, LAZY_BIND_MODE);

#elif defined( HAVE_DL_H )

      native_handle = PROG_HANDLE;

#endif /* HAVE_DL_H */

      if (native_handle == NULL)
	{
#ifdef HAVE_ERRNO_H
	  g_snprintf(err_buf, 256, "unable to get dl-handle for executable. (err = %d)", errno);
#else
	  g_snprintf(err_buf, 256, "unable to get dl-handle for executable.");
#endif

	  fill_in_last_error(err_buf);

	  return NULL;
	}
      else
	{
	  clear_last_error();
	}

      handle = g_new(struct GnomeLibHandle, 1);
      handle->full_path = g_strdup("<exec>");
      handle->ref_count = 0;
      handle->native_handle = native_handle;

      if (_libs == NULL) _libs = g_list_alloc();
      g_list_append(_libs,
		    handle);
    }

  handle++;

  return handle;
}

GnomeLibHandle*
gnome_dl_find(char *lib_name, int bind_lazy)
{
  g_warning("gnome_dl_find() not implemented yet.  return NULL\n");

  /* we want to force the last error to be this value, not (possibly) consult dl_error. */
  last_error = "gnome_dl_find() not implemented yet.";

  return NULL;
}

int
gnome_dl_unload(GnomeLibHandle *handle)
{
  int ret_val = -1;
  GList *list;

  handle->ref_count --;

  if (handle->ref_count == 0 && handle->native_handle)
    {
#if defined( HAVE_DLFCN_H )
      ret_val = dlclose(handle->native_handle);
#elif defined( HAVE_DL_H )
      ret_val = shl_unload(handle->native_handle);
#endif

      if (ret_val == -1)
	g_snprintf(err_buf, 256, "can't unload library %s", handle->full_path);
    }
  
  if (!g_list_remove(_libs, handle))
    {
      ret_val = -1;

      /* force this message, regardless of whether or not they have dlerror() */
      g_snprintf(err_buf, 256, "error removing handle from cache for library %s.", handle->full_path);
      last_error = err_buf;
      return ret_val;
    }
  else
    {
      g_free(handle->full_path);
      g_free(handle);
    }

  if (ret_val == -1)
    {
      fill_in_last_error(err_buf);
    }
  else
    clear_last_error();

  return ret_val;
}

static GnomeFuncHandle*
gnome_dl_raw_findsym(char *symbol_name,
		     GnomeLibHandle* handle)
{
  void *symbol = NULL;

#if defined( HAVE_DLFCN_H )
  symbol = dlsym(handle->native_handle, symbol_name);
#elif defined( HAVE_DL_H )
  /* returns an int value, 0 if successful, -1 if not */
  if (shl_findsym(handle->native_handle, symbol_name, TYPE_UNDEFINED, symbol) == -1)
    symbol = NULL;
#endif

  return symbol;
}

GnomeFuncHandle*
gnome_dl_findsym(char *symbol_name,
		 GnomeLibHandle* handle)
{
  void *symbol = NULL;
  char *lookup;

  if (handle == NULL) return NULL;

#if defined( NEED_UNDERSCORE )
  lookup = g_copy_strings("_", symbol_name, NULL);
#else
  lookup = g_strdup(symbol_name);
#endif

  symbol = gnome_dl_raw_findsym(lookup, handle);

  g_free(lookup);

  if (symbol == NULL)
    {
      g_snprintf(err_buf, 256, "couldn't find symbol '%s' in library %s", symbol_name, handle->full_path);

      fill_in_last_error(err_buf);
    }
  else
    clear_last_error();

  return symbol;
}

GnomeFuncHandle*
gnome_dl_findsym_and_lib(char *symbol_name,
			 GnomeLibHandle **handle)
{
  void *symbol = NULL;
  char *lookup;
  GList *cur;

#ifdef NEED_UNDERSCORE
  lookup = g_copy_strings("_", symbol_name, NULL);
#else
  lookup = g_strdup(symbol_name);
#endif

  for (cur = _libs;
       cur && cur->data;
       cur = g_list_next(cur))
    {
      GnomeLibHandle *h = (GnomeLibHandle*)cur->data;

      symbol = gnome_dl_findsym(symbol_name,
				h);

      if (symbol != NULL)
	{
	  if (handle != NULL) *handle = h;
	  break;
	}
    }

  g_free(lookup);

  if (symbol == NULL)
    {
      g_snprintf(err_buf, 256, "couldn't find symbol '%s'", symbol_name);
      
      fill_in_last_error(err_buf);
    }
  else
    clear_last_error();

  return symbol;
}

const char*
gnome_dl_error()
{
  char *error = last_error;

  clear_last_error();

  return error;
}

int
gnome_dl_is_library_filename(char *lib_path)
{
  int suffix_length = strlen(LIB_SUFFIX);
  int lib_length = strlen(lib_path);

  if (strlen(lib_path) > suffix_length
      && !strcmp(lib_path + lib_length - suffix_length, LIB_SUFFIX))
    return TRUE;
  else
    return FALSE;
}

void
gnome_dl_init()
{
}
