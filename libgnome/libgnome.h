#ifndef LIBGNOME_H
#define LIBGNOME_H

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-dentry.h"
#include "libgnome/gnome-dl.h"
#include "libgnome/gnome-dns.h"
#include "libgnome/gnome-hook.h"
#include "libgnome/gnome-i18n.h"
#include "libgnome/gnome-score.h"
#include "libgnome/gnome-string.h"
#include "libgnome/gnome-triggers.h"
#include "libgnome/gnome-util.h"

extern char *gnome_user_home_dir;
extern char *gnome_user_dir;

void gnomelib_init (int *argc, char ***argv);

#endif
