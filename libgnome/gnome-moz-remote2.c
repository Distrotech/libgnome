/*
 * Copyright (C) 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the Gnome Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/******
 * gnome-moz-remote2.c:
 *   A netscape remote control. Written by Elliot Lee, with lots of code stolen from the original gnome-moz-remote
 *   (by jamesh, I think).
 *   My changes are under the GPL, Copyright (C) 1999 Red Hat Software.
 ******/
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
#include "libgnome.h"
#include "gnome-program.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* vroot.h is a header file which lets a client get along with `virtual root'
   window managers like swm, tvtwm, olvwm, etc.  If you don't have this header
   file, you can find it at "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers, you can
   comment this line out.
 */
#include "vroot.h"

#define CMD_TIMEOUT 10000

typedef struct {
  Window cmd_window, result_window;

  Display *dpy;
  GMainLoop *ml;
  enum { LOCK_WAIT, STARTING_PROCESS, SENT_COMMAND, PROCESSING, FINISHED } state;

  guint timeout_tag;
  gboolean got_response : 1;

} AppContext;

static AppContext global_ctx = {0};
static const char *dpy_name;
static int dpy_sync = 0, win_noraise=0, win_new=0, win_ids_print=-1, win_close = 0;
static char *win_id = NULL;

static struct poptOption options[] = {
  {"display", '\0', POPT_ARG_STRING, &dpy_name, 0, N_("Specify X display to use."), "DISPLAY"},
  {"sync", '\0', POPT_ARG_NONE, &dpy_sync, 0, N_("Put the X connection in synchronous mode."), NULL},
  {"id", '\0', POPT_ARG_STRING, &win_id, 0, N_("Specify the X window ID of Netscape."), "WIN-ID"},
  
  {"noraise", '\0', POPT_ARG_NONE, &win_noraise, 0, N_("Don't raise the Netscape window."), NULL},
  {"newwin", '\0', POPT_ARG_NONE, &win_new, 0, N_("Show the given URL in a new window."), NULL},
  {"closewin", '\0', POPT_ARG_NONE, &win_close, 0, N_("Close the specified window."), NULL},
  {"print-id", '\0', POPT_ARG_INT, &win_ids_print, 0, N_("Print the X window ID of the utilized window."), NULL},
  {NULL}
};

static Window mozilla_remote_find_window (Display *dpy);
static void mozilla_remote_init_atoms (Display *dpy);
static gboolean mozilla_handle_xevents(GIOChannel *source, GIOCondition condition, gpointer data);
static gboolean mozilla_remote_check_window (Display *dpy, Window window);
static void mozilla_remote_obtain_lock (Display *dpy, Window window);
static void mozilla_remote_free_lock (Display *dpy, Window window);
static Window mozilla_remote_command (Display *dpy, Window window, const char *command, Bool raise_p);
static int handle_xerror(Display *dpy, XErrorEvent *ev);

static gboolean
do_main_quit(gpointer data)
{
  AppContext *ctx = (AppContext *) data;
  gboolean retval;

  g_main_quit(ctx->ml);

  retval = FALSE;
  ctx->timeout_tag = 0;

  return retval;
}

static Window
mozilla_loop_run(AppContext *ctx, Window cmd_window, Window result_window, int state)
{
  Window retval;

  ctx->state = state;
  ctx->result_window = result_window;
  ctx->cmd_window = cmd_window;
  ctx->got_response = FALSE;
  if(!ctx->ml)
    ctx->ml = g_main_new(FALSE);

  if(cmd_window)
    XSelectInput(ctx->dpy, cmd_window, (PropertyChangeMask|StructureNotifyMask));
  XSelectInput(ctx->dpy, DefaultRootWindow(ctx->dpy), (PropertyChangeMask|SubstructureNotifyMask));
  XFlush(ctx->dpy);

  if(!ctx->timeout_tag)
    ctx->timeout_tag = g_timeout_add(CMD_TIMEOUT, do_main_quit, ctx);

  g_main_run(ctx->ml);

  if(ctx->timeout_tag)
    {
      g_source_remove(ctx->timeout_tag); ctx->timeout_tag = 0;
    }

  if(global_ctx.result_window == -1)
    retval = 0;
  else
    {
      retval = global_ctx.result_window;
      if(retval)
	XSelectInput(ctx->dpy, retval, 0);
    }

  return retval;
}

static gboolean
mozilla_loop_quit(AppContext *ctx)
{
  switch(ctx->state)
    {
    case STARTING_PROCESS:
      if(ctx->result_window != 0)
	g_main_quit(ctx->ml);
      break;
    case SENT_COMMAND:
      if(ctx->result_window && ctx->got_response)
	{
	  g_main_quit(ctx->ml);
	}
      break;
    case LOCK_WAIT:
      if(ctx->got_response)
	{
	  ctx->result_window = ctx->cmd_window;
	  g_main_quit(ctx->ml);
	}
      break;
    case PROCESSING:
    case FINISHED:
      g_warning("Main loop running in illegal state");
      g_main_quit(ctx->ml);
      break;
    }

  return FALSE;
}

static gboolean
mozilla_process_start(gpointer data)
{
  char *argv[2];

  argv[0] = gnome_config_get_string("/gnome-moz-remote/Mozilla/filename=netscape");
  argv[1] = NULL;

  gnome_execute_async(NULL, 1, argv);

  g_free(argv[0]);

  return FALSE;
}

static Window
mozilla_window_spawn(Display *dpy)
{
  g_idle_add(mozilla_process_start, &global_ctx);
  return mozilla_loop_run(&global_ctx, 0, 0, STARTING_PROCESS);
}

/* We need to escape all ',' characters in the URL */
static char *
escape_url (const char *url) 
{
  gint n_escaped = 0;
  char *escaped_url;
  const char *p;
  char *q;
  
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

  return escaped_url;
}

static Window
mozilla_window_goto(Display *dpy, Window winid, const char *url)
{
  char command[2048], *escaped_url;
  if(!winid)
    winid = mozilla_remote_find_window(dpy);
  if(!winid)
    {
      winid = mozilla_window_spawn(dpy);
      win_new = 0;
    }
  if(!winid)
    return 0;

  escaped_url = escape_url(url);
  g_snprintf(command, sizeof(command), "openURL(%s%s%s)", escaped_url, win_new?", new-window":"", win_noraise?", noraise":"");
  winid = mozilla_remote_command(dpy, winid, command, !win_noraise);
  g_free(escaped_url);

  return winid;
}

int main(int argc, char **argv)
{
  const char **args;
  poptContext ctx;
  int i;
  Display *dpy;
  GnomeProgram *program;

  dpy_name = g_getenv("DISPLAY");

  program = gnome_program_get();
  g_object_set(G_OBJECT(program), "popt_table", options, LIBGNOME_INIT, NULL);
  ctx = gnome_program_preinit(program, "gnome_moz_remote", VERSION, argc, argv);
  args = poptGetArgs(ctx);

  dpy = XOpenDisplay(dpy_name);
  XSetErrorHandler(handle_xerror);

  if (!dpy)
    return 1;

  g_io_add_watch(g_io_channel_unix_new(ConnectionNumber(dpy)), G_IO_IN, mozilla_handle_xevents, &global_ctx);

  mozilla_remote_init_atoms(dpy);

  if(dpy_sync)
    XSynchronize (dpy, True);

  global_ctx.dpy = dpy;

  if(args && args[1])
    {
      g_print("%s: Only one URL on the cmdline\n", argv[0]);
      goto out;
    }
  if((!args || !args[0]) && !win_close)
    return 0; /* Nothing to do */

  if(win_new)
    win_noraise = TRUE; /* This makes the new window wind up on the current screen, at least with NSCP 4.7 */

  mozilla_remote_obtain_lock(dpy, DefaultRootWindow(dpy));

  if(win_close)
    {
      Window utwin;

      if(!win_id)
	goto out;

      utwin = strtol(win_id, NULL, 0);

      mozilla_remote_command(dpy, utwin, "close", FALSE);
    }
  else
    {
      if(win_id)
	{
	  global_ctx.cmd_window = strtol(win_id, NULL, 0);
	  if(!mozilla_remote_check_window(dpy, global_ctx.cmd_window))
	    global_ctx.cmd_window = 0;
	}
      else
	global_ctx.cmd_window = mozilla_remote_find_window(dpy);

      if (!global_ctx.cmd_window)
	global_ctx.cmd_window = mozilla_window_spawn(dpy);

      for(i = 0; args[i]; i++)
	{
	  Window utwin;

	  utwin = mozilla_window_goto(dpy, global_ctx.cmd_window, args[i]);
	  if((win_ids_print >= 0) && utwin)
	    {
	      static FILE *fh = NULL;

	      if(!fh)
		fh = fdopen(win_ids_print, "w");
	      fprintf(fh, "%#lx\n", utwin);
	    }

	  if(!utwin)
	    break;
	}
    }

 out:
  mozilla_remote_free_lock(dpy, DefaultRootWindow(dpy));

  return 0;
}

/****************************************** find/check a window, any window :) *********************************/
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
  Window root = DefaultRootWindow(dpy);
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

/* This function uses the _WIN_CLIENT_LIST GNOME window manager hint to find
 * the list of client windows.  It then calls mozilla_remote_test_window on
 * each until it finds a matching mozilla window
 */
static Window
gnomewm_find_window(Display *dpy)
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
    if (mozilla_remote_check_window(dpy, wins[i])) {
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
mozilla_remote_find_window_recurse(Display *dpy, Window win)
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

    if (mozilla_remote_check_window(dpy, win))
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
      Window check = mozilla_remote_find_window_recurse(dpy, ret_children[i]);
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
mozilla_remote_find_window (Display *dpy)
{
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));

  if (gnomewm_check_wm(dpy))
    return gnomewm_find_window(dpy);
  else
    return mozilla_remote_find_window_recurse(dpy, root);
}

static gboolean
mozilla_remote_check_window (Display *dpy, Window window)
{
  Atom type;
  int format, status;
  unsigned long nitems, bytesafter;
  unsigned char *version = 0;

  g_return_val_if_fail(window != 0, 0);
  status = XGetWindowProperty (dpy, window, XA_MOZILLA_VERSION,
				   0, (65536 / sizeof (long)),
				   False, XA_STRING,
				   &type, &format, &nitems, &bytesafter,
				   &version);
  if (status != Success || !version)
    return FALSE;

  XFree (version);

  return TRUE;
}

static Window
mozilla_remote_command (Display *dpy, Window window, const char *command,
			Bool raise_p)
{
  g_return_val_if_fail(window != 0, 0);

#ifdef DEBUG_PROPS
  g_printerr("gnome-moz-remote: (writing " MOZILLA_COMMAND_PROP
	     " \"%s\" to 0x%x)\n", command, (unsigned int) window);
#endif

  XChangeProperty (dpy, window, XA_MOZILLA_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) command,
		   strlen (command));

  return mozilla_loop_run(&global_ctx, window, win_new?0:window, SENT_COMMAND);
}

static gboolean
mozilla_handle_xevents(GIOChannel *source, GIOCondition condition, gpointer data)
{
  XEvent ev;
  AppContext *ctx = (AppContext *)data;
  Window tmpwin;

  XNextEvent(ctx->dpy, &ev);

  switch(ev.xany.type)
    {
    case MapNotify:
      if ((ctx->state != SENT_COMMAND || ctx->result_window)
	  && ctx->state != STARTING_PROCESS)
	break;

      tmpwin = mozilla_remote_find_window_recurse(ctx->dpy, ev.xmap.window);
      if(tmpwin)
	{
	  ctx->result_window = tmpwin;
	  mozilla_loop_quit(ctx);
	}
      break;
    case DestroyNotify:
      if(ev.xdestroywindow.window != ctx->cmd_window)
	break;

      ctx->got_response = TRUE;
      ctx->result_window = -1;
      mozilla_loop_quit(ctx);
      break;
    case PropertyNotify:
      if (ctx->state != SENT_COMMAND && ctx->state != LOCK_WAIT)
	break;
      if(ev.xproperty.atom != XA_MOZILLA_COMMAND
	 && ev.xproperty.atom != XA_MOZILLA_LOCK)
	break;
      if(ev.xproperty.window != ctx->cmd_window)
	break;
      switch(ev.xproperty.state)
	{
	case PropertyNewValue:
	  if (ev.xproperty.atom == XA_MOZILLA_COMMAND) {
	    Atom actual_type;
	    int actual_format;
	    unsigned long nitems, bytes_after;
	    unsigned char *data = 0;
	    int result;

	    result = XGetWindowProperty (ctx->dpy, ctx->cmd_window, XA_MOZILLA_RESPONSE,
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
			   (unsigned int) ctx->cmd_window);
		result = 6;
		ctx->got_response = TRUE;
	      }
	    else if (!data || strlen((char *) data) < 5)
	      {
		g_printerr("gnome-moz-remote: invalid data on "
			   MOZILLA_RESPONSE_PROP " property of window 0x%0x.\n",
			   (unsigned int) ctx->cmd_window);
		ctx->result_window = -1;
		ctx->got_response = TRUE;
	      }
	    else if (*data == '1')	/* positive preliminary reply */
	      {
		g_printerr("gnome-moz-remote: %s\n", data + 4);
	      }
	    else if (!strncmp ((char *)data, "200", 3)) /* positive completion */
	      {
		result = 0;
		ctx->got_response = TRUE;
	      }
	    else if (*data == '2')		/* positive completion */
	      {
		g_printerr("gnome-moz-remote: %s\n", data + 4);
		ctx->got_response = TRUE;
	      }
	    else if (*data == '3')	/* positive intermediate reply */
	      {
		g_printerr("gnome-moz-remote: internal error: "
			   "server wants more information?  (%s)\n",
			   data);
		ctx->got_response = TRUE;
	      }
	    else if (*data == '4' ||	/* transient negative completion */
		     *data == '5')	/* permanent negative completion */
	      {
		g_printerr("gnome-moz-remote: %s\n", data + 4);
		result = (*data - '0');
		ctx->got_response = TRUE;
		ctx->result_window = -1;
	      }
	    else
	      {
		g_printerr(
			   "gnome-moz-remote: unrecognised " MOZILLA_RESPONSE_PROP
			   " from window 0x%x: %s\n",
			   (unsigned int) ctx->cmd_window, data);
		ctx->got_response = TRUE;
		ctx->result_window = -1;
	      }

	    if (data)
	      XFree (data);

	    mozilla_loop_quit(ctx);
	  }
	  break;
	case PropertyDelete:
	  if(ev.xproperty.atom == XA_MOZILLA_COMMAND)
	    {
	      ctx->got_response = TRUE;
	      mozilla_loop_quit(ctx);
	    }
	  else if(ctx->state == LOCK_WAIT
		  && ev.xproperty.atom == XA_MOZILLA_LOCK)
	    {
	      ctx->got_response = TRUE;
	      mozilla_loop_quit(ctx);
	    }
	  break;
	}      
      break;
    }

  return TRUE;
}

/****** locking *******/
static char *lock_data = NULL;

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
	  mozilla_remote_free_lock(dpy, window);
	  exit(1);
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

	  mozilla_loop_run(&global_ctx, window, window, LOCK_WAIT);
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
handle_xerror(Display *dpy, XErrorEvent *ev)
{
  mozilla_remote_free_lock(dpy, DefaultRootWindow(dpy));

  exit(1);
}
