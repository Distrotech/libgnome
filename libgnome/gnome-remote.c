/* gnome-remote.c - Handle remote execution configuration.

   Copyright (C) 1998 Tom Tromey

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

#include "libgnomeP.h"

/* Name of key where remote config stored.  */
#define BASE "/Gnome/Remote/"

/* Subkey where default command stored.  Try to pick something
   unlikely to be the name of an actual host.  */
#define DEFAULT "__default__"

/**
 * gnome_remote_get_command:
 * @host: Host where command should be run
 * @argcp: Result parameter; number of elements in rsh command
 * @argvp: Result parameter; the actual rsh command to use
 * 
 * Given a host, this function will return the user's desired
 * `rsh'-like command to use to contact that host.  This lets the user
 * choose how to log in on a host-by-host basis.  If @host is %NULL,
 * then the user's default remote shell command is chosen.
 **/
void
gnome_remote_get_command (const char *host, gint *argcp, char ***argvp)
{
  gboolean def = TRUE;

  if (host)
    {
      gchar *path = g_strconcat (BASE, host, NULL);
      gnome_config_get_vector_with_default (path, argcp, argvp, &def);
      g_free (path);
    }

  if (def)
    {
      /* No entry for this host, or we were requested to return the
	 default.  So try that.  */
      gnome_config_get_vector_with_default (BASE DEFAULT, argcp, argvp, &def);
      if (def)
	{
	  /* No default.  So use built-in default.  FIXME on some
	     machines this should be "remsh".  Enter configure... */
	  *argvp = (char **) g_malloc (2 * sizeof (char *));
	  (*argvp)[0] = g_strdup ("rsh");
	  (*argvp)[1] = NULL;
	  *argcp = 1;
	}
    }
}

/**
 * gnome_remote_set_command:
 * @host: Name of host
 * @argc: Number of elements in argument vector
 * @argv: Command line to use to contact host
 * 
 * This sets the appropriate options in the config database so that
 * subsequent calls to gnome_remote_get_command() will return @argv.
 * If @host is %NULL, then the user's default remote shell is set.
 * If @argc is %0, then this instance of the command is deleted.
 **/
void
gnome_remote_set_command (const char *host, gint argc, const char * const argv[])
{
  gchar *path;

  if (host)
    path = g_strconcat (BASE, host, NULL);
  else
    path = BASE DEFAULT;

  gnome_config_clean_key (path);

  if (argc)
    gnome_config_set_vector (path, argc, argv);

  if (host)
    g_free (path);
}
