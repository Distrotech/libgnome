/* -*- Mode: C; tab-width: 8 -*-
   gnome-dl.h --- dynamic loading for gnome.
   Copyright (C) 1998 The Free Software Foundation, all rights reserved.
   Created: Chris Toshok <toshok@hungry.com>, 24-Jan-1998.
 */

#ifndef __GNOME_DL_H__
#define __GNOME_DL_H__

BEGIN_GNOME_DECLS

typedef struct GnomeLibHandle GnomeLibHandle;
typedef void* GnomeFuncHandle;

#define GNOME_DL_BIND_NOW 0
#define GNOME_DL_BIND_LAZY 1

/* one time initialization */
void			gnome_dl_init(void);

/* returns the shared library descriptor for the executable. */
GnomeLibHandle*		gnome_dl_exec_handle(void);

/* loads a shared library given an explicit pathname */
GnomeLibHandle*		gnome_dl_load(char *lib_path, int bind_lazy);

/* find a shared library based on a name.  this function searches the LD_LIBRARY_PATH
   (and other gnome specific places.)  The LIB_NAME parameter for a library libpthread.so
   would be 'pthread'. */
GnomeLibHandle*		gnome_dl_find(char *lib_name, int bind_lazy);

/* unloads a library.  returns 0 on success, and -1 otherwise. */
int			gnome_dl_unload(GnomeLibHandle *handle);

/* find a symbol in a specific shared library. */
GnomeFuncHandle*	gnome_dl_findsym(char *symbol_name, GnomeLibHandle* handle);

/* search down the list of libraries, in the order in which they were loaded. returns both
   a handle to the function and the library it was found in.  If you're not interested in 
   the library handle, pass NULL for it. */
GnomeFuncHandle*	gnome_dl_findsym_and_lib(char *symbol_name, GnomeLibHandle **handle);

/* returns the error returned by the last gnome_dl_* call.  equivalent to dlerror() on
   platforms that have it. */
const char*		gnome_dl_error(void);

END_GNOME_DECLS

#endif
