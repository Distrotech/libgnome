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

BEGIN_GNOME_DECLS

extern GnomeModuleInfo libgnome_module_info;
#define LIBGNOME_INIT GNOME_PARAM_MODULE,&libgnome_module_info

extern const char libgnome_param_create_directories[], libgnome_param_espeaker[], libgnome_param_enable_sound[],
  libgnome_param_file_locator[], libgnome_param_app_prefix[], libgnome_param_app_sysconfdir[], libgnome_param_app_datadir[],
  libgnome_param_app_libdir[], libgnome_param_human_readable_name[];
#define LIBGNOME_PARAM_CREATE_DIRECTORIES libgnome_param_create_directories
#define LIBGNOME_PARAM_ESPEAKER libgnome_param_espeaker
#define LIBGNOME_PARAM_ENABLE_SOUND libgnome_param_enable_sound
#define LIBGNOME_PARAM_FILE_LOCATOR libgnome_param_file_locator
#define LIBGNOME_PARAM_APP_PREFIX libgnome_param_app_prefix
#define LIBGNOME_PARAM_APP_SYSCONFDIR libgnome_param_app_sysconfdir
#define LIBGNOME_PARAM_APP_DATADIR libgnome_param_app_datadir
#define LIBGNOME_PARAM_APP_LIBDIR libgnome_param_app_libdir
#define LIBGNOME_PARAM_HUMAN_READABLE_NAME libgnome_param_human_readable_name 

typedef char * (*GnomeFileLocatorFunc)(const char *domain, const char *filename, gboolean only_if_exists);

extern char *gnome_user_home_dir;
extern char *gnome_user_dir;
extern char *gnome_user_private_dir;
extern char *gnome_user_accels_dir;

/* Backwards compat */
#define gnome_app_id gnome_program_get_name(gnome_program_get())
#define gnome_app_version gnome_program_get_version(gnome_program_get())

const char* gnome_program_get_human_readable_name(const GnomeProgram *app);


END_GNOME_DECLS

#endif /* GNOMELIB_INIT_H */
