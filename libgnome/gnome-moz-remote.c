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


/* vroot.h is a header file which lets a client get along with `virtual root'
   window managers like swm, tvtwm, olvwm, etc.  If you don't have this header
   file, you can find it at "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers, you can
   comment this line out.
 */
#include "vroot.h"

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

/* these are needed for the machine check and client list check */
#define WM_STATE_PROP                  "WM_STATE"
#define GNOME_SUPPORTING_WM_CHECK_PROP "_WIN_SUPPORTING_WM_CHECK"
#define GNOME_PROTOCOLS_PROP           "_WIN_PROTOCOLS"
#define GNOME_CLIENT_LIST_PROP         "_WIN_CLIENT_LIST"
static Atom XA_WM_STATE                = 0;
static Atom XA_WIN_SUPPORTING_WM_CHECK = 0;
static Atom XA_WIN_PROTOCOLS           = 0;
static Atom XA_WIN_CLIENT_LIST         = 0;

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

  if (! XA_WM_STATE)
    XA_WM_STATE = XInternAtom (dpy, WM_STATE_PROP, False);
  if (! XA_WIN_SUPPORTING_WM_CHECK)
    XA_WIN_SUPPORTING_WM_CHECK = XInternAtom (dpy, GNOME_SUPPORTING_WM_CHECK_PROP, False);
  if (! XA_WIN_PROTOCOLS)
    XA_WIN_PROTOCOLS = XInternAtom (dpy, GNOME_PROTOCOLS_PROP, False);
  if (! XA_WIN_CLIENT_LIST)
    XA_WIN_CLIENT_LIST = XInternAtom (dpy, GNOME_CLIENT_LIST_PROP, False);
}

/* returns true if the window manager supports the _WIN_CLIENT_LIST
 * gnome wm hint */
static Bool
gnomewm_check_wm(Display *dpy)
{
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));
  Atom r_type;
  int r_format;
  unsigned long count, i;
  unsigned long bytes_remain;
  unsigned char *prop, *prop2, *prop3;

  if (XGetWindowProperty(dpy, root,
                         XA_WIN_SUPPORTING_WM_CHECK, 0, 1, False, XA_CARDINAL,
                         &r_type, &r_format,
                         &count, &bytes_remain, &prop) == Success && prop) {
    if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
      Window n = *(long *)prop;
      if (XGetWindowProperty(dpy, n,
			     XA_WIN_SUPPORTING_WM_CHECK, 0, 1, False, 
			     XA_CARDINAL,
			     &r_type, &r_format, &count, &bytes_remain, 
			     &prop2) == Success && prop2) {
	if (r_type == XA_CARDINAL && r_format == 32 && count == 1) {
	  if (XGetWindowProperty(dpy, root,
				 XA_WIN_PROTOCOLS, 0,0x7fffffff,False, XA_ATOM,
				 &r_type, &r_format, &count, &bytes_remain,
				 &prop3) == Success && prop3) {
	    if (r_type == XA_ATOM && r_format == 32) {
	      /* I heard George saying format==32 requires this type */
	      unsigned long *list = (unsigned long *)prop3;
	      XFree(prop);
	      XFree(prop2);

	      for (i = 0; i < count; i++)
		if (list[i] == XA_WIN_CLIENT_LIST) {
		  XFree(prop3);
		  return TRUE;
		}
	    }
	    XFree(prop3);
	  }
	}
	XFree(prop2);
      }       
    }
    XFree(prop);
  }
  return FALSE;
}

static char localhostname[1024] = "\0";

/* this function is called on each client window to deduce whether it is
 * a mozilla window (by checking the _MOZILLA_VERSION property) and, if
 * isLocal is set, it is a local copy of netscape (by checking if the
 * WM_CLIENT_MACHINE property is set to the local hostname).
 */
static Bool
mozilla_remote_test_window(Display *dpy, Window win, Bool isLocal)
{
  Atom type;
  int format, status;
  unsigned long nitems, bytesafter;
  unsigned char *version = 0, *hostname = 0;

  g_assert(win != 0);
  status = XGetWindowProperty (dpy, win, XA_MOZILLA_VERSION,
			       0, (65536 / sizeof (long)),
			       False, XA_STRING,
			       &type, &format, &nitems, &bytesafter,
			       &version);
  if (! version)
    return False;
  XFree (version);
  if (status != Success || type == None)
    return False;
  if (!isLocal) {
    /* for non-local URLs, any netscape prog is good enough */
    return True;
  }
  status = XGetWindowProperty (dpy, win, XA_WM_CLIENT_MACHINE,
			       0, 0x7fffffff, False, XA_STRING,
			       &type, &format, &nitems, &bytesafter,
			       &hostname);
  if (! hostname || status != Success || type == None) {
    if (hostname) XFree(hostname);
    return False;
  }
  if (! g_strcasecmp(hostname, localhostname)) {
    XFree(hostname);
    return True;
  }
  XFree(hostname);
  return False;
}

/* This function uses the _WIN_CLIENT_LIST GNOME window manager hint to find
 * the list of client windows.  It then calls mozilla_remote_test_window on
 * each until it finds a matching mozilla window
 */
static Window
gnomewm_find_window(Display *dpy, Bool isLocal)
{
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));
  Atom r_type;
  int r_format;
  unsigned long count, i;
  unsigned long bytes_remain, *wins;
  unsigned char *prop;

  XGetWindowProperty(dpy, root, XA_WIN_CLIENT_LIST,
		     0, 0x7fffffff, False, XA_CARDINAL,
		     &r_type, &r_format, &count, &bytes_remain, &prop);
  if (!prop || count <= 0 || r_type != XA_CARDINAL || r_format != 32) {
    if (prop) XFree(prop);
    return 0;
  }
  wins = (unsigned long *)prop;
  for (i = 0; i < count; i++)
    if (mozilla_remote_test_window(dpy, wins[i], isLocal)) {
      Window ret = wins[i];
      XFree(prop);
      return ret;
    }
  XFree(prop);
  return 0;
}

/* this function recurses down the X window heirachy, finding windows with the
 * WM_STATE property set (which indicates that we have a client window), and
 * calls mozilla_remote_test_window on each until it finds a match
 */
static Window
mozilla_remote_find_window_recurse(Display *dpy, Window win,
				   Bool isLocal)
{
  Atom r_type;
  int r_format;
  unsigned long count, after;
  unsigned char *data;

  XGetWindowProperty(dpy, win, XA_WM_STATE, 0, 0, False, AnyPropertyType,
		     &r_type, &r_format, &count, &after, &data);
  if (r_type) {
    /* this is a client window */
    XFree(data);
    if (mozilla_remote_test_window(dpy, win, isLocal))
      return win;
    else
      return 0;
  } else {
    Window ret_root, ret_parent, *ret_children;
    unsigned int ret_nchildren, i;

    if (XQueryTree(dpy, win, &ret_root, &ret_parent,
		   &ret_children, &ret_nchildren) != True)
      return 0;
    for (i = 0; i < ret_nchildren; i++) {
      Window check = mozilla_remote_find_window_recurse(dpy, ret_children[i],
							isLocal);
      if (check) {
	XFree(ret_children);
	return check;
      }
    }
  }
  return 0;
}

/* this function finds the first matching mozilla window (using GNOME WM
 * hints or querying the X window heirachy) and returns it
 */
static Window
mozilla_remote_find_window (Display *dpy, Bool isLocal)
{
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));

  if (isLocal && gethostname(localhostname, sizeof(localhostname)) < 0)
    return 0;

  if (gnomewm_check_wm(dpy))
    return gnomewm_find_window(dpy, isLocal);
  else
    return mozilla_remote_find_window_recurse(dpy, root, isLocal);
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
      g_printerr("gnome-moz-remote: window 0x%x is not a Netscape window.\n",
		 (unsigned int) window);
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
      g_snprintf (lock_data, 255, "pid%d@", getpid ());

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
	  g_printerr("gnome-moz-remote: (writing " MOZILLA_LOCK_PROP
		     " \"%s\" to 0x%x)\n"
		     lock_data, (unsigned int) window);
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

	  g_printerr("gnome-moz-remote: window 0x%x is locked by %s; waiting...\n",
		     (unsigned int) window, data);
	  waited = True;

	  while (1)
	    {
	      XEvent event;
	      XNextEvent (dpy, &event);
	      if (event.xany.type == DestroyNotify &&
		  event.xdestroywindow.window == window)
		{
		  g_printerr("gnome-moz-remote: window 0x%x unexpectedly destroyed.\n",
			      (unsigned int) window);
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
		  g_printerr("gnome-moz-remote: (0x%x unlocked, trying again...)\n",
			     (unsigned int) window);
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
    g_printerr("gnome-moz-remote: obtained lock.\n");
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
  g_printerr("gnome-moz-remote: (deleting " MOZILLA_LOCK_PROP
	     " \"%s\" from 0x%x)\n",
	     lock_data, (unsigned int) window);
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
      g_printerr("gnome-moz-remote: unable to read and delete "
		 MOZILLA_LOCK_PROP " property\n");
      return;
    }
  else if (!data || !*data)
    {
      g_printerr("gnome-moz-remote: invalid data on " MOZILLA_LOCK_PROP
		 " of window 0x%x.\n",
		 (unsigned int) window);
      return;
    }
  else if (strcmp ((char *) data, lock_data))
    {
      g_printerr("gnome-moz-remote: " MOZILLA_LOCK_PROP
		 " was stolen!  Expected \"%s\", saw \"%s\"!\n",
		 lock_data, data);
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
  g_printerr("gnome-moz-remote: (writing " MOZILLA_COMMAND_PROP
	     " \"%s\" to 0x%x)\n", command, (unsigned int) window);
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
	  g_printerr("gnome-moz-remote: window 0x%x was destroyed.\n",
		     (unsigned int) window);
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
	      g_printerr("gnome-moz-remote: (server sent "MOZILLA_RESPONSE_PROP
			 " \"%s\" to 0x%x.)\n",
			 data, (unsigned int) window);
	    }
#endif

	  if (result != Success)
	    {
	      g_printerr("gnome-moz-remote: failed reading "
			 MOZILLA_RESPONSE_PROP " from window 0x%0x.\n",
			 (unsigned int) window);
	      result = 6;
	      done = True;
	    }
	  else if (!data || strlen((char *) data) < 5)
	    {
	      g_printerr("gnome-moz-remote: invalid data on "
			 MOZILLA_RESPONSE_PROP " property of window 0x%0x.\n",
			 (unsigned int) window);
	      result = 6;
	      done = True;
	    }
	  else if (*data == '1')	/* positive preliminary reply */
	    {
	      g_printerr("gnome-moz-remote: %s\n", data + 4);
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
	      g_printerr("gnome-moz-remote: %s\n", data + 4);
	      result = 0;
	      done = True;
	    }
	  else if (*data == '3')	/* positive intermediate reply */
	    {
	      g_printerr("gnome-moz-remote: internal error: "
			 "server wants more information?  (%s)\n",
			 data);
	      result = 3;
	      done = True;
	    }
	  else if (*data == '4' ||	/* transient negative completion */
		   *data == '5')	/* permanent negative completion */
	    {
	      g_printerr("gnome-moz-remote: %s\n", data + 4);
	      result = (*data - '0');
	      done = True;
	    }
	  else
	    {
	      g_printerr(
		       "gnome-moz-remote: unrecognised " MOZILLA_RESPONSE_PROP
		       " from window 0x%x: %s\n",
		       (unsigned int) window, data);
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
	  g_printerr("gnome-moz-remote: (server 0x%x has accepted "
		   MOZILLA_COMMAND_PROP ".)\n",
		   (unsigned int) window);
	}
#endif /* DEBUG_PROPS */
    }

 DONE:

  if (new_command)
    free (new_command);

  return result;
}

static int
mozilla_remote_commands (Display *dpy, Window window,
			 char **commands, Bool isLocal)
{
  Bool raise_p = True;
  int status = 0;
  mozilla_remote_init_atoms (dpy);

  if (window == 0)
    window = mozilla_remote_find_window (dpy, isLocal);
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
mozilla_remote_cmd(Display *dpy, Window window, char *command,
		   Bool raise_p, Bool isLocal)
{
  int status = 0;

  mozilla_remote_init_atoms(dpy);
  if (window == 0)
    window = mozilla_remote_find_window(dpy, isLocal);
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
  { NULL, '\0', POPT_ARG_CALLBACK, parseAnArg, 0, NULL, NULL},
  {"display", '\0', POPT_ARG_STRING, NULL, -6, "Specify X display to use.", "DISPLAY"},
  {"id", '\0', POPT_ARG_STRING, NULL, -7, "Specify the X window ID of Netscape.", "WIN-ID"},
  {"sync", '\0', POPT_ARG_NONE, NULL, -8, "Put X connection in synchronous mode.", NULL},
  {NULL, '\0', 0, NULL, 0}
};

static struct poptOption options[] = {
  { NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_POST, parseAnArg, 0, NULL, NULL},
  { "version", 'V', POPT_ARG_NONE, NULL, -42, "Display version", NULL },
  { "remote", '\0', POPT_ARG_STRING, NULL, -1, "Execute a command inside Netscape", "CMD"},
  {"raise", '\0', POPT_ARG_NONE, NULL, -2, "Raise the Netscape window after commands.", NULL},
  {"noraise", '\0', POPT_ARG_NONE, NULL, -3, "Don't raise the Netscape window.", NULL},
  {"newwin", '\0', POPT_ARG_NONE, NULL, -4, "Show the given URL in a new window", NULL},
  {"local", '\0', POPT_ARG_NONE, NULL, -5, "Copy of netscape must be local", NULL},
  {NULL, '\0', POPT_ARG_INCLUDE_TABLE, x_options, 0, "X Options", NULL},
  {NULL, '\0', 0, NULL, 0}
};

char *dpy_string = NULL, *url_string = NULL;
char **remote_commands = NULL;
int remote_command_count = 0;
int remote_command_size = 0;
unsigned long remote_window = 0;
Bool sync_p = False, raise_p = False, isLocal = False;
gboolean new_window = FALSE;
int i;

static void
parseAnArg(poptContext ctx,
	   enum poptCallbackReason reason,
	   const struct poptOption *opt,
	   const char *arg, void *data)
{
  char c;

  if (reason == POPT_CALLBACK_REASON_POST) {
    url_string = poptGetArg(ctx);
    return;
  }
  /* reason == POPT_CALLBACK_REASON_OPTION */
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
  case -5: /* --local */
    isLocal = TRUE;
    break;
  case -6: /* --display */
    dpy_string = (char *)arg;
    break;
  case -7: /* --id */
    if (remote_command_count > 0) {
      g_printerr("gnome-moz-remote: '--id' option must precede '--remote' options.\n");
      poptPrintUsage(ctx, stdout, 0);
      exit(1);
    }
    if (sscanf(arg, " %ld %c", &remote_window, &c))
      ;
    else if (sscanf(arg, "0x%lx %c", &remote_window, &c))
      ;
    else {
      g_printerr("gnome-moz-remote: invalid '--id' option \"%s\"\n", arg);
      poptPrintUsage(ctx, stdout, 0);
      exit(1);
    }
    break;
  case -8: /* --sync */
    sync_p = True;
    break;
  case -42:
    g_print("Gnome mozilla-remote %s\n", VERSION);
    exit(0);
    break;
  }
}

/* We need to escape all ',' characters in the URL */
static char *
escape_url (char *url) 
{
  gint n_escaped = 0;
  char *escaped_url, *p, *q;
  
  p = url;
  while ((p = strchr (p, ','))) {
    n_escaped++;
    p++;
  }
  
  escaped_url = g_new (char, strlen(url) + 2 * n_escaped + 1);
  
  p = url;
  q = escaped_url;
  while ((p = strchr (p, ','))) {
    strncpy (q, url, p-url);
    q += p-url;
    strcpy (q, "%2c");
    q += 3;

    p++;
    url = p;
  }
  strcpy (q, url);

  if(!strncmp("ghelp:", escaped_url, strlen("ghelp:"))) {
    /* Try to do some fixups so we can display ghelp: URL's */
    char *retval;

    /* This is really bad-hacky, and won't work for a bunch of ghelp:
       URL's (e.g. the ones that don't have absolute filenames). Oh
       well */
    retval = g_strdup_printf("file:%s", escaped_url + 6);

    g_free(escaped_url);

    return retval;
  } else
    return escaped_url;
}

int
main (int argc, char **argv)
{
  Display *dpy;
  char *def;

  /* Parse arguments ... */
  gnomelib_init("gnome-moz-remote", VERSION);
  gnomelib_register_popt_table(options, "gnome-moz-remote options");
  poptFreeContext(gnomelib_parse_args(argc, argv, 0));

  dpy = XOpenDisplay (dpy_string);
  if (! dpy)
    return (-1);

  if (sync_p)
    XSynchronize (dpy, True);

  if (url_string) {
    char *argv[3];
    char *buf, *escaped_url;

    escaped_url = escape_url (url_string);

    if (!g_strncasecmp(escaped_url, "file:", 5))
      isLocal = True;;
    buf = g_strdup_printf("openURL(%s%s)",
			  escaped_url, new_window ? ",new-window" : "");
    if (!mozilla_remote_cmd(dpy, (Window) remote_window, buf,
			    raise_p, isLocal))
      exit(0);

    if (gnome_is_program_in_path ("mozilla"))
	    def = "/gnome-moz-remote/Mozilla/filename=mozilla";
    else
	    def = "/gnome-moz-remote/Mozilla/filename=netscape";
    
    argv[0] = gnome_config_get_string(def);
    argv[1] = escaped_url;
    argv[2] = NULL;
    return (gnome_execute_async(NULL, 2, argv) >= 0);
  } else
    if (remote_commands)
        return (mozilla_remote_commands (dpy, (Window) remote_window,
					 remote_commands, isLocal));
  return 0;
}
