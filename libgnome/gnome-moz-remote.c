/* -*- Mode:C; tab-width: 8 -*- */
/* gnome-moz-remote.c -- open a URL in a current netscape, or start a new
 *    copy of netscape, pointing at the given page.
 * This is a modified version of http://home.netscape.com/newsref/std/remote.c
 * You can find documentation of --remote commands at
 *   http://home.netscape.com/newsref/std/x-remote.html
 * My changes fall under the GPL2 (where the previous license allows).  The
 * original copyright message follows:
 */
/* remote.c --- remote control of Netscape Navigator for Unix.
 * version 1.1.3, for Netscape Navigator 1.1 and newer.
 *
 * Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * To compile:
 *
 *    cc -o netscape-remote remote.c -DSTANDALONE -lXmu -lX11
 *
 * To use:
 *
 *    netscape-remote -help
 *
 * Documentation for the protocol which this code implements may be found at:
 *
 *    http://home.netscape.com/newsref/std/x-remote.html
 *
 * Bugs and commentary to x_cbug@netscape.com.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "gnomesupport.h"
#include <libgnome/libgnome.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>	/* for XmuClientWindow() */


/* vroot.h is a header file which lets a client get along with `virtual root'
   window managers like swm, tvtwm, olvwm, etc.  If you don't have this header
   file, you can find it at "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers, you can
   comment this line out.
 */
#include "vroot.h"

static const char *progname = 0;

static void
parseAnArg(poptContext ctx,
	   enum poptCallbackReason reason,
	   const struct poptOption *opt,
	   const char *arg, void *data);

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
static Atom XA_MOZILLA_VERSION  = 0;
static Atom XA_MOZILLA_LOCK     = 0;
static Atom XA_MOZILLA_COMMAND  = 0;
static Atom XA_MOZILLA_RESPONSE = 0;

static void
mozilla_remote_init_atoms (Display *dpy)
{
  if (! XA_MOZILLA_VERSION)
    XA_MOZILLA_VERSION = XInternAtom (dpy, MOZILLA_VERSION_PROP, False);
  if (! XA_MOZILLA_LOCK)
    XA_MOZILLA_LOCK = XInternAtom (dpy, MOZILLA_LOCK_PROP, False);
  if (! XA_MOZILLA_COMMAND)
    XA_MOZILLA_COMMAND = XInternAtom (dpy, MOZILLA_COMMAND_PROP, False);
  if (! XA_MOZILLA_RESPONSE)
    XA_MOZILLA_RESPONSE = XInternAtom (dpy, MOZILLA_RESPONSE_PROP, False);
}

static Window
mozilla_remote_find_window (Display *dpy)
{
  int i;
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));
  Window root2, parent, *kids;
  unsigned int nkids;
  Window result = 0;

  if (! XQueryTree (dpy, root, &root2, &parent, &kids, &nkids))
    return 0;

  /* root != root2 is possible with virtual root WMs. */

  if (! (kids && nkids))
    return 0;

  for (i = nkids-1; i >= 0; i--)
    {
      Atom type;
      int format, status;
      unsigned long nitems, bytesafter;
      unsigned char *version = 0;
      Window w = XmuClientWindow (dpy, kids[i]);
      g_assert(w != 0);
      status = XGetWindowProperty (dpy, w, XA_MOZILLA_VERSION,
				       0, (65536 / sizeof (long)),
				       False, XA_STRING,
				       &type, &format, &nitems, &bytesafter,
				       &version);
      if (! version)
	continue;
      XFree (version);
      if (status == Success && type != None)
	{
	  result = w;
	  break;
	}
    }

  return result;
}

static void
mozilla_remote_check_window (Display *dpy, Window window)
{
  Atom type;
  int format, status;
  unsigned long nitems, bytesafter;
  unsigned char *version = 0;

  g_return_if_fail(window != 0);
  status = XGetWindowProperty (dpy, window, XA_MOZILLA_VERSION,
				   0, (65536 / sizeof (long)),
				   False, XA_STRING,
				   &type, &format, &nitems, &bytesafter,
				   &version);
  if (status != Success || !version)
    {
      fprintf (stderr, "%s: window 0x%x is not a Netscape window.\n",
	       progname, (unsigned int) window);
      exit (6);
    }
  XFree (version);
}


static char *lock_data = 0;

static void
mozilla_remote_obtain_lock (Display *dpy, Window window)
{
  Bool locked = False;
  Bool waited = False;

  g_return_if_fail(window != 0);
  if (! lock_data)
    {
      lock_data = (char *) malloc (255);
      sprintf (lock_data, "pid%d@", getpid ());
      if (gethostname (lock_data + strlen (lock_data), 100))
	{
	  perror ("gethostname");
	  exit (-1);
	}
    }

  do
    {
      int result;
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;

      XGrabServer (dpy);   /* ################################# DANGER! */

      result = XGetWindowProperty (dpy, window, XA_MOZILLA_LOCK,
				   0, (65536 / sizeof (long)),
				   False, /* don't delete */
				   XA_STRING,
				   &actual_type, &actual_format,
				   &nitems, &bytes_after,
				   &data);
      if (result != Success || actual_type == None)
	{
	  /* It's not now locked - lock it. */
#ifdef DEBUG_PROPS
	  fprintf (stderr, "%s: (writing " MOZILLA_LOCK_PROP
		   " \"%s\" to 0x%x)\n",
		   progname, lock_data, (unsigned int) window);
#endif
	  XChangeProperty (dpy, window, XA_MOZILLA_LOCK, XA_STRING, 8,
			   PropModeReplace, (unsigned char *) lock_data,
			   strlen (lock_data));
	  locked = True;
	}

      XUngrabServer (dpy); /* ################################# danger over */
      XSync (dpy, False);

      if (! locked)
	{
	  /* We tried to grab the lock this time, and failed because someone
	     else is holding it already.  So, wait for a PropertyDelete event
	     to come in, and try again. */

	  fprintf (stderr, "%s: window 0x%x is locked by %s; waiting...\n",
		   progname, (unsigned int) window, data);
	  waited = True;

	  while (1)
	    {
	      XEvent event;
	      XNextEvent (dpy, &event);
	      if (event.xany.type == DestroyNotify &&
		  event.xdestroywindow.window == window)
		{
		  fprintf (stderr, "%s: window 0x%x unexpectedly destroyed.\n",
			   progname, (unsigned int) window);
		  exit (6);
		}
	      else if (event.xany.type == PropertyNotify &&
		       event.xproperty.state == PropertyDelete &&
		       event.xproperty.window == window &&
		       event.xproperty.atom == XA_MOZILLA_LOCK)
		{
		  /* Ok!  Someone deleted their lock, so now we can try
		     again. */
#ifdef DEBUG_PROPS
		  fprintf (stderr, "%s: (0x%x unlocked, trying again...)\n",
			   progname, (unsigned int) window);
#endif
		  break;
		}
	    }
	}
      if (data)
	XFree (data);
    }
  while (! locked);

  if (waited)
    fprintf (stderr, "%s: obtained lock.\n", progname);
}


static void
mozilla_remote_free_lock (Display *dpy, Window window)
{
  int result;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = 0;

  g_return_if_fail(window != 0);
#ifdef DEBUG_PROPS
	  fprintf (stderr, "%s: (deleting " MOZILLA_LOCK_PROP
		   " \"%s\" from 0x%x)\n",
		   progname, lock_data, (unsigned int) window);
#endif

  result = XGetWindowProperty (dpy, window, XA_MOZILLA_LOCK,
			       0, (65536 / sizeof (long)),
			       True, /* atomic delete after */
			       XA_STRING,
			       &actual_type, &actual_format,
			       &nitems, &bytes_after,
			       &data);
  if (result != Success)
    {
      fprintf (stderr, "%s: unable to read and delete " MOZILLA_LOCK_PROP
	       " property\n",
	       progname);
      return;
    }
  else if (!data || !*data)
    {
      fprintf (stderr, "%s: invalid data on " MOZILLA_LOCK_PROP
	       " of window 0x%x.\n",
	       progname, (unsigned int) window);
      return;
    }
  else if (strcmp ((char *) data, lock_data))
    {
      fprintf (stderr, "%s: " MOZILLA_LOCK_PROP
	       " was stolen!  Expected \"%s\", saw \"%s\"!\n",
	       progname, lock_data, data);
      return;
    }

  if (data)
    XFree (data);
}


static int
mozilla_remote_command (Display *dpy, Window window, const char *command,
			Bool raise_p)
{
  int result = 0;
  Bool done = False;
  char *new_command = 0;

  g_return_val_if_fail(window != 0, 1);
  /* The -noraise option is implemented by passing a "noraise" argument
     to each command to which it should apply.
   */
  if (! raise_p)
    {
      char *close;
      new_command = (char *) malloc (strlen (command) + 20);
      strcpy (new_command, command);
      close = strrchr (new_command, ')');
      if (close)
	strcpy (close, ", noraise)");
      else
	strcat (new_command, "(noraise)");
      command = new_command;
    }

#ifdef DEBUG_PROPS
  fprintf (stderr, "%s: (writing " MOZILLA_COMMAND_PROP " \"%s\" to 0x%x)\n",
	   progname, command, (unsigned int) window);
#endif

  XChangeProperty (dpy, window, XA_MOZILLA_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) command,
		   strlen (command));

  while (!done)
    {
      XEvent event;
      XNextEvent (dpy, &event);
      if (event.xany.type == DestroyNotify &&
	  event.xdestroywindow.window == window)
	{
	  /* Print to warn user...*/
	  fprintf (stderr, "%s: window 0x%x was destroyed.\n",
		   progname, (unsigned int) window);
	  result = 6;
	  goto DONE;
	}
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.state == PropertyNewValue &&
	       event.xproperty.window == window &&
	       event.xproperty.atom == XA_MOZILLA_RESPONSE)
	{
	  Atom actual_type;
	  int actual_format;
	  unsigned long nitems, bytes_after;
	  unsigned char *data = 0;

	  result = XGetWindowProperty (dpy, window, XA_MOZILLA_RESPONSE,
				       0, (65536 / sizeof (long)),
				       True, /* atomic delete after */
				       XA_STRING,
				       &actual_type, &actual_format,
				       &nitems, &bytes_after,
				       &data);
#ifdef DEBUG_PROPS
	  if (result == Success && data && *data)
	    {
	      fprintf (stderr, "%s: (server sent " MOZILLA_RESPONSE_PROP
		       " \"%s\" to 0x%x.)\n",
		       progname, data, (unsigned int) window);
	    }
#endif

	  if (result != Success)
	    {
	      fprintf (stderr, "%s: failed reading " MOZILLA_RESPONSE_PROP
		       " from window 0x%0x.\n",
		       progname, (unsigned int) window);
	      result = 6;
	      done = True;
	    }
	  else if (!data || strlen((char *) data) < 5)
	    {
	      fprintf (stderr, "%s: invalid data on " MOZILLA_RESPONSE_PROP
		       " property of window 0x%0x.\n",
		       progname, (unsigned int) window);
	      result = 6;
	      done = True;
	    }
	  else if (*data == '1')	/* positive preliminary reply */
	    {
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
	      /* keep going */
	      done = False;
	    }
#if 1
	  else if (!strncmp ((char *)data, "200", 3)) /* positive completion */
	    {
	      result = 0;
	      done = True;
	    }
#endif
	  else if (*data == '2')		/* positive completion */
	    {
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
	      result = 0;
	      done = True;
	    }
	  else if (*data == '3')	/* positive intermediate reply */
	    {
	      fprintf (stderr, "%s: internal error: "
		       "server wants more information?  (%s)\n",
		       progname, data);
	      result = 3;
	      done = True;
	    }
	  else if (*data == '4' ||	/* transient negative completion */
		   *data == '5')	/* permanent negative completion */
	    {
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
	      result = (*data - '0');
	      done = True;
	    }
	  else
	    {
	      fprintf (stderr,
		       "%s: unrecognised " MOZILLA_RESPONSE_PROP
		       " from window 0x%x: %s\n",
		       progname, (unsigned int) window, data);
	      result = 6;
	      done = True;
	    }

	  if (data)
	    XFree (data);
	}
#ifdef DEBUG_PROPS
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.window == window &&
	       event.xproperty.state == PropertyDelete &&
	       event.xproperty.atom == XA_MOZILLA_COMMAND)
	{
	  fprintf (stderr, "%s: (server 0x%x has accepted "
		   MOZILLA_COMMAND_PROP ".)\n",
		   progname, (unsigned int) window);
	}
#endif /* DEBUG_PROPS */
    }

 DONE:

  if (new_command)
    free (new_command);

  return result;
}

static int
mozilla_remote_commands (Display *dpy, Window window, char **commands)
{
  Bool raise_p = True;
  int status = 0;
  mozilla_remote_init_atoms (dpy);

  if (window == 0)
    window = mozilla_remote_find_window (dpy);
  else
    mozilla_remote_check_window (dpy, window);
  g_return_val_if_fail(window != 0, 1);

  XSelectInput (dpy, window, (PropertyChangeMask|StructureNotifyMask));

  mozilla_remote_obtain_lock (dpy, window);

  while (*commands)
    {
      if (!strcmp (*commands, "-raise"))
	raise_p = True;
      else if (!strcmp (*commands, "-noraise"))
	raise_p = False;
      else
	status = mozilla_remote_command (dpy, window, *commands, raise_p);

      if (status != 0)
	break;
      commands++;
    }

  /* When status = 6, it means the window has been destroyed */
  /* It is invalid to free the lock when window is destroyed. */

  if ( status != 6 )
  mozilla_remote_free_lock (dpy, window);

  return status;
}


static int
mozilla_remote_cmd(Display *dpy, Window window, char *command, Bool raise_p)
{
  int status = 0;

  mozilla_remote_init_atoms(dpy);
  if (window == 0)
    window = mozilla_remote_find_window(dpy);
  else
    mozilla_remote_check_window(dpy, window);
  if (window == 0)
    return 1;
  XSelectInput(dpy, window, (PropertyChangeMask|StructureNotifyMask));
  mozilla_remote_obtain_lock(dpy, window);
  status = mozilla_remote_command(dpy, window, command, raise_p);
  if (status != 6)
    mozilla_remote_free_lock(dpy, window);
  return status;
}

static struct poptOption x_options[] = {
  {"display", '\0', POPT_ARG_STRING, NULL, -5, "Specify X display to use.", "DISPLAY"},
  {"id", '\0', POPT_ARG_STRING, NULL, -6, "Specify the X window ID of Netscape.", "WIN-ID"},
  {"sync", '\0', POPT_ARG_NONE, NULL, -7, "Put X connection in synchronous mode.", NULL},
  {NULL, '\0', 0, NULL, 0}
};

static struct poptOption options[] = {
  { NULL, '\0', POPT_ARG_CALLBACK, parseAnArg, 0, NULL, NULL},
  { "remote", '\0', POPT_ARG_STRING, NULL, -1, "Execute a command inside Netscape", "CMD"},
  {"raise", '\0', POPT_ARG_NONE, NULL, -2, "Raise the Netscape window after commands.", NULL},
  {"noraise", '\0', POPT_ARG_NONE, NULL, -3, "Don't raise the Netscape window.", NULL},
  {"newwin", '\0', POPT_ARG_NONE, NULL, -4, "Show the given URL in a new window", NULL},
  {NULL, '\0', POPT_ARG_INCLUDE_TABLE, x_options, 0, "X Options", NULL},
  {NULL, '\0', 0, NULL, 0}
};

char *dpy_string = NULL, *url_string = NULL;
char **remote_commands = NULL;
int remote_command_count = 0;
int remote_command_size = 0;
unsigned long remote_window = 0;
Bool sync_p = False, raise_p = False;
gboolean new_window = FALSE;
int i;

static void
parseAnArg(poptContext ctx,
	   enum poptCallbackReason reason,
	   const struct poptOption *opt,
	   const char *arg, void *data)
{
  char c;

  switch (opt->val) {
  case -1: /* --remote */
    if (remote_command_count == remote_command_size) {
      remote_command_size += 20;
      remote_commands =
	(remote_commands
	 ? realloc(remote_commands, remote_command_size * sizeof(char *))
	 : calloc(remote_command_size, sizeof(char *)));
    }
    remote_commands[remote_command_count++] = (char *)arg;
    break;
  case -2: /* --raise */
    if (remote_command_count == remote_command_size) {
      remote_command_size += 20;
      remote_commands =
	(remote_commands
	 ? realloc(remote_commands, remote_command_size * sizeof(char *))
	 : calloc(remote_command_size, sizeof(char *)));
    }
    remote_commands[remote_command_count++] = "-raise";
    raise_p = True;
    break;
  case -3: /* --noraise */
    if (remote_command_count == remote_command_size) {
      remote_command_size += 20;
      remote_commands =
	(remote_commands
	 ? realloc(remote_commands, remote_command_size * sizeof(char *))
	 : calloc(remote_command_size, sizeof(char *)));
    }
    remote_commands[remote_command_count++] = "-noraise";
    raise_p = False;
    break;
  case -4: /* --newwin */
    new_window = TRUE;
    break;
  case -5: /* --display */
    dpy_string = (char *)arg;
    break;
  case -6: /* --id */
    if (remote_command_count > 0) {
      fprintf(stderr, "%s: '--id' option must precede '--remote' options.\n",
	      progname);
      poptPrintUsage(ctx, stdout, 0);
      exit(1);
    }
    if (sscanf(arg, " %ld %c", &remote_window, &c))
      ;
    else if (sscanf(arg, "0x%lx %c", &remote_window, &c))
      ;
    else {
      fprintf(stderr, "%s: invalid '--id' option \"%s\"\n", progname, arg);
      poptPrintUsage(ctx, stdout, 0);
      exit(1);
    }
    break;
  case -7: /* --sync */
    sync_p = True;
    break;
  }
};

int
main (int argc, char **argv)
{
  Display *dpy;
  char *tmp, **catmp;
  poptContext ctx;

  progname = argv[0];
  tmp = strrchr(argv[0], '/');
  progname = tmp? (tmp + 1) : progname;

  /* Parse arguments ... */
  gnomelib_init("gnome-moz-remote", VERSION);
  gnomelib_register_popt_table(options, "gnome-moz-remote options");
  ctx = gnomelib_parse_args(argc, argv, 0);
  while(poptGetNextOpt(ctx) > 0) /* */;

  catmp = poptGetArgs(ctx);
  if(catmp && catmp[0])
    url_string = catmp[0];

  dpy = XOpenDisplay (dpy_string);
  if (! dpy)
    return (-1);

  if (sync_p)
    XSynchronize (dpy, True);

  if (url_string) {
    char buf[512], *argv[3];

    g_snprintf(buf, 512, "openURL(%s%s)", url_string,
	       new_window ? ",new-window" : "");
    if (!mozilla_remote_cmd(dpy, (Window) remote_window, buf, raise_p))
      exit(0);

    argv[0] = gnome_config_get_string("/gnome-moz-remote/Mozilla/filename=netscape");
    argv[1] = url_string;
    argv[2] = NULL;
    return (gnome_execute_async(NULL, 2, argv));
  } else
    return (mozilla_remote_commands (dpy, (Window) remote_window,
				     remote_commands));

}
