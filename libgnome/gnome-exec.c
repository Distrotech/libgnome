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
  int comm_pipes[2];
  int child_errno, itmp;
  pid_t child_pid, immediate_child_pid; /* XXX this routine assumes
					   pid_t is signed */

  if(pipe(comm_pipes))
    return -1;

  child_pid = immediate_child_pid = fork();

  switch(child_pid) {
  case -1:
    return -1;

  case 0: /* START PROCESS 1: child */
    close(comm_pipes[0]);
    child_pid = fork();

    switch(child_pid) {
    case -1:
      itmp = errno;
      child_pid = -1; /* simplify parent code */
      write(comm_pipes[1], &child_pid, sizeof(child_pid));
      write(comm_pipes[1], &itmp, sizeof(itmp));

    default:
      _exit(0); break;      /* END PROCESS 1: monkey in the middle dies */

    case 0:                 /* START PROCESS 2: child of child */
      /* pre-exec setup */
      fcntl(comm_pipes[1], F_SETFD, 1); /* Make the FD close if
					   executing the program succeeds */

      child_pid = getpid();
      write(comm_pipes[1], &child_pid, sizeof(child_pid));

      if(envv) {
	for(itmp = 0; itmp < envc; itmp++)
	  putenv(envv[itmp]);
      }

      if(dir) chdir(dir);

      /* doit */
      execvp(argv[0], argv);

      /* failed */
      itmp = errno;
      write(comm_pipes[1], &itmp, sizeof(itmp));
      _exit(1); break;      /* END PROCESS 2 */
    }
    break;

  default: /* parent process */
  }

  close(comm_pipes[1]);

  if(read(comm_pipes[0], &child_pid, sizeof(child_pid)) != sizeof(child_pid))
    return -1; /* really weird things happened */
  if(read(comm_pipes[0], &child_errno, sizeof(child_errno))
     == sizeof(child_errno))
    errno = child_errno;

  /* do this after the read's in case some OS's handle blocking on pipe writes
     differently */
  waitpid(immediate_child_pid, &itmp, 0); /* eat zombies */

  return child_pid;
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
