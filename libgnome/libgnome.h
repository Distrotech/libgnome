/* libgnome.h - Public header that includes all libgnome headers.

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

/* Note: Keep this file in sync with libgnomeP.h.  */

#ifndef LIBGNOME_H
#define LIBGNOME_H

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-dentry.h"
#include "libgnome/gnome-exec.h"
#include "libgnome/gnome-help.h"
#include "libgnome/gnome-i18n.h"
#include "libgnome/gnome-metadata.h"
#include "libgnome/gnome-mime.h"
#include "libgnome/gnome-mime-info.h"
/* Should this be in gnome-print? */
#include "libgnome/gnome-paper.h"
#include "libgnome/gnome-popt.h"
#include "libgnome/gnome-remote.h"
#include "libgnome/gnome-score.h"
#include "libgnome/gnome-sound.h"
#include "libgnome/gnome-triggers.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-url.h"

extern char *gnome_user_home_dir;
extern char *gnome_user_dir;
extern char *gnome_user_private_dir;
extern char *gnome_user_accels_dir;
extern char *gnome_app_id, *gnome_app_version;
extern char gnome_do_not_create_directories;

BEGIN_GNOME_DECLS

void gnomelib_init (const char *app_id, const char *app_version);

END_GNOME_DECLS

#endif /* LIBGNOME_H */





