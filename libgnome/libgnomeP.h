/* libgnomeP.h - Private version of libgnome.h.
   Note: Keep this file in sync with libgnome.h.  */

#ifndef LIBGNOMEP_H
#define LIBGNOMEP_H

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-dentry.h"
#include "libgnome/gnome-dl.h"
#include "libgnome/gnome-exec.h"
#include "libgnome/gnome-hook.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-parse.h"
#include "libgnome/gnome-score.h"
#include "libgnome/gnome-string.h"
#include "libgnome/gnome-triggers.h"
#include "libgnome/gnome-util.h"

extern char *gnome_user_home_dir;
extern char *gnome_user_dir;
extern char *gnome_user_private_dir;
extern char *gnome_app_id;

void gnomelib_init (char *app_id);
void gnomelib_register_arguments (void);

#endif /* LIBGNOMEP_H */
