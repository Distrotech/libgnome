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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

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
#include <string.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

/**
 * gnome_execute_async_with_env_fds:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * @close_fds: If TRUE will close all fds but 0,1, and 2
 * 
 * Like gnome_execute_async_with_env() but has a flag to decide whether or not  * to close fd's
 * 
 * Return value: the process id, or %-1 on error.
 **/
int
gnome_execute_async_with_env_fds (const char *dir, int argc, 
				       char * const argv[], int envc, 
				       char * const envv[], 
				       gboolean close_fds)
{
  int comm_pipes[2];
  int child_errno, itmp, i, open_max;
  char **cpargv;
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

      cpargv = alloca((argc + 1) * sizeof(char *));
      memcpy(cpargv, argv, argc * sizeof(char *));
      cpargv[argc] = NULL;

      if(close_fds) {
	/* Close all file descriptors but stdin stdout and stderr */
	open_max = sysconf (_SC_OPEN_MAX);
	for (i = 3; i < open_max; i++){
	  close (i);
	}
      }
      /* doit */
      execvp(cpargv[0], cpargv);

      /* failed */
      itmp = errno;
      write(comm_pipes[1], &itmp, sizeof(itmp));
      _exit(1); break;      /* END PROCESS 2 */
    }
    break;

  default: /* parent process */
    /* do nothing */
    break;
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

/**
 * gnome_execute_async_with_env:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @envc: Number of environment slots
 * @envv: Environment vector
 * 
 * This function forks and executes some program in the background.
 * On error, returns %-1; in this case, errno should hold a useful
 * value.  Searches the path to find the child.  Environment settings
 * in @envv are added to the existing environment -- they do not
 * completely replace it.  This function closes all fds besides 0, 1,
 * and 2 for the child
 * 
 * Return value: the process id, or %-1 on error.
 **/
int
gnome_execute_async_with_env (const char *dir, int argc, char * const argv[], 
			      int envc, char * const envv[])
{
  return gnome_execute_async_with_env_fds(dir,argc,argv,envc,envv,TRUE);
}


/**
 * gnome_execute_async:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * 
 * Like gnome_execute_async_with_env(), but doesn't add anything to
 * child's environment.
 * 
 * Return value: process id of child, or %-1 on error.
 **/
int
gnome_execute_async (const char *dir, int argc, char * const argv[])
{
  return gnome_execute_async_with_env (dir, argc, argv, 0, NULL);
}

/**
 * gnome_execute_async_fds:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @argc: Number of arguments
 * @argv: Argument vector to exec child
 * @close_fds: 
 * Like gnome_execute_async_with_env_fds(), but doesn't add anything to
 * child's environment.
 * 
 * Return value: process id of child, or %-1 on error.
 **/
int
gnome_execute_async_fds (const char *dir, int argc, 
				 char * const argv[], gboolean close_fds)
{
  return gnome_execute_async_with_env_fds (dir, argc, argv, 0, NULL, 
					   close_fds);
}

/**
 * gnome_execute_shell_fds:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @commandline: Shell command to execute
 * @close_fds: Like close_fds in gnome_execute_async_with_env_fds()
 * Like gnome_execute_async_with_env_fds(), but uses the user's shell to
 * run the desired program.  Note that the pid of the shell is
 * returned, not the pid of the user's program.
 * 
 * Return value: process id of shell, or %-1 on error.
 **/
int
gnome_execute_shell_fds (const char *dir, const char *commandline,
			      gboolean close_fds)
{
  char * argv[4];
  int r;

  g_return_val_if_fail(commandline != NULL, -1);

  argv[0] = gnome_util_user_shell ();
  argv[1] = "-c";
  /* This cast is safe.  It sucks that we need it, though.  */
  argv[2] = (char *) commandline;
  argv[3] = NULL;

  r = gnome_execute_async_with_env_fds (dir, 4, argv, 0, NULL, close_fds);
  free (argv[0]);
  return r;
}

/**
 * gnome_execute_shell:
 * @dir: Directory in which child should be execd, or NULL for current
 *       directory
 * @commandline: Shell command to execute
 * 
 * Like gnome_execute_async_with_env(), but uses the user's shell to
 * run the desired program.  Note that the pid of the shell is
 * returned, not the pid of the user's program.
 * 
 * Return value: process id of shell, or %-1 on error.
 **/
int
gnome_execute_shell (const char *dir, const char *commandline)
{
  return gnome_execute_shell_fds(dir, commandline, TRUE);
}







