/* gnome-exec.c - Execute some command.

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

#include "gnome-defs.h"
#include "gnome-exec.h"
#include "gnome-util.h"
#include <glib.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

static void
report_errno (int fd)
{
  int n = errno;

  /* fprintf (stderr, "failure %d\n", errno); */
  /* We don't really care if the write fails.  There's nothing we can
     do about it.  */
  write (fd, &n, sizeof n);
  _exit (1);
}

/* Fork and execute some program in the background.  Returns -1 on
 * error.  Returns PID on success.  Should correctly report errno returns
 * from a failing child invocation.  DIR is the directory in which to
 * exec the child; if NULL the current directory is used.  Searches $PATH
 * to find the child.
 */
int
gnome_execute_async_with_env (const char *dir, int argc, char * const argv[],
			      int envc, char * const envv[])
{
  int pid;
  int status, count, dummy;
  int p[2];

  if (pipe (p) == -1)
    return -1;

  pid = fork ();
  if (pid == (pid_t) -1)
    return -1;

  if (pid == 0)
    {
      /* Child.  Fork again so child won't become a zombie.  */
      close (p[0]);
      pid = fork ();
      if (pid == (pid_t) -1)
	report_errno (p[1]);

      if (pid != 0) {

	if(waitpid(pid, &status, WNOHANG) > 0) {
	  /* lame!!!! */
	  status *= -1;
	  write(p[1], &status, sizeof(status));
	} else
	  write(p[1], &pid, sizeof(pid));

	_exit (0);
      }

      /* Child of the child.  */
      fcntl (p[1], F_SETFD, 1);

      /* Try to go to the right directory.  If we fail, hopefully it
	 will still be ok.  */
      if (dir)
	chdir (dir);

      if (envv)
	{
	  int i;

	  for (i = 0; i < envc; ++i)
	    putenv (envv[i]);
	}

      execvp (argv[0], argv);
      _exit(errno); /* xxx lamehack */
    }

  close (p[1]);

  /* Ignore errors.  FIXME: maybe only ignore EOFs.  */
  count = read (p[0], &status, sizeof(status));

  /* Wait for the child, since we know it will exit shortly.  */
  if (waitpid (pid, &dummy, 0) == -1)
    return -1;

  if (count == sizeof(status))
    {

      if(status <= 0) {
	/* the child failed.  STATUS is the errno value.  */
	errno = status;
	return -1;
      } else
	return status;
    }
}

int
gnome_execute_async (const char *dir, int argc, char * const argv[])
{
  return gnome_execute_async_with_env (dir, argc, argv, 0, NULL);
}

int
gnome_execute_shell (const char *dir, const char *commandline)
{
  char * argv[4];
  int r;

  g_return_val_if_fail(commandline != NULL, -1);

  argv[0] = gnome_util_user_shell ();
  argv[1] = "-c";
  /* This cast is safe.  It sucks that we need it, though.  */
  argv[2] = (char *) commandline;
  argv[3] = NULL;

  r = gnome_execute_async (dir, 4, argv);
  free (argv[0]);
  return r;
}
