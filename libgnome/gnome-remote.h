/* gnome-remote.h - Handle remote execution configuration.

   Copyright (C) 1998 Tom Tromey

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#ifndef GNOME_REMOTE_H
#define GNOME_REMOTE_H

BEGIN_GNOME_DECLS

/* Return a command which should be used to run a program on HOST.
   Typically this command will be something like "rsh".  If HOST is
   NULL, this returns the default command that should be used; typical
   programs should not use this information.  */
void gnome_remote_get_command (const char *host, gint *argcp, char ***argvp);

/* Set the command which should be used to run a program on HOST.  If
   ARGC is 0, then any current binding for HOST is removed.  If HOST
   is NULL, the default command is set.  */
void gnome_remote_set_command (const char *host, gint argc, char **argv);

END_GNOME_DECLS

#endif /* GNOME_REMOTE_H */
