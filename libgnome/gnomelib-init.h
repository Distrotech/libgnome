/* gnomelib-init.h - Export libgnome module
   Copyright (C) 1997, 1998, 1999 Free Software Foundation
   Copyright (C) 1999, 2000 Red Hat, Inc.
   All rights reserved.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

#ifndef GNOMELIB_INIT_H
#define GNOMELIB_INIT_H 1

#include <glib.h>
#include "gnome-defs.h"
#include "gnome-program.h"

BEGIN_GNOME_DECLS

extern GnomeModuleInfo libgnome_module_info;
#define LIBGNOME_INIT GNOME_PARAM_MODULE_INFO,&libgnome_module_info

typedef char * (*GnomeFileLocatorFunc)(const char *domain, const char *filename, gboolean only_if_exists);

extern char *gnome_user_home_dir;
extern char *gnome_user_dir;
extern char *gnome_user_private_dir;
extern char *gnome_user_accels_dir;

/* Backwards compat */
#define gnome_app_id gnome_program_get_name (gnome_program_get())
#define gnome_app_version gnome_program_get_version (gnome_program_get())

END_GNOME_DECLS

#endif /* GNOMELIB_INIT_H */
