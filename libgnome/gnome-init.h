/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 *               2001 SuSE Linux AG
 * All rights reserved.
 *
 * This file is part of GNOME 2.0.
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

#include <libgnome/gnome-program.h>

#include <bonobo/Bonobo.h>

G_BEGIN_DECLS

/*
 * Bonobo::ConfigDatabase for desktop-wide settings.
 */
Bonobo_ConfigDatabase
gnome_program_get_desktop_config_database (GnomeProgram *program);

#define GNOME_PARAM_DESKTOP_CONFIG_DATABASE "desktop-config-database"
#define GNOME_PARAM_DESKTOP_CONFIG_MONIKER  "desktop-config-moniker"

/*
 * Bonobo::ConfigDatabase for per-application settings.
 */
Bonobo_ConfigDatabase
gnome_program_get_config_database (GnomeProgram *program);

/*
 * Shortcut for gnome_program_get_config_database (gnome_program_get ()).
 */
Bonobo_ConfigDatabase
gnome_get_config_database (void);

#define GNOME_PARAM_CONFIG_DATABASE "config-database"
#define GNOME_PARAM_CONFIG_MONIKER  "config-moniker"

extern GnomeModuleInfo gnome_bonobo_activation_module_info;
extern GnomeModuleInfo gnome_vfs_module_info;
extern GnomeModuleInfo libgnome_module_info;

G_END_DECLS

#endif /* LIBGNOMEINIT_H */
