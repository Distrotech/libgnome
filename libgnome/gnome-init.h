/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef LIBGNOMEINIT_H
#define LIBGNOMEINIT_H

#include <libgnomebase/gnome-defs.h>
#include <libgnomebase/gnome-program.h>

#include <gconf/gconf-client.h>

G_BEGIN_DECLS

GConfClient *
gnome_program_get_gconf_client (GnomeProgram *program);

#define GNOME_PARAM_GCONF_CLIENT "gconf-client"

extern GnomeModuleInfo gnome_oaf_module_info;
extern GnomeModuleInfo gnome_gconf_module_info;
extern GnomeModuleInfo gnome_vfs_module_info;
extern GnomeModuleInfo libgnome_module_info;

G_END_DECLS

#endif /* LIBGNOMEINIT_H */
