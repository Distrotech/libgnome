/******
 *
 ******/

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

typedef struct {
  Window cmd_window, result_window;

  Display *dpy;
  GMainLoop *ml;
  enum { STARTING_PROCESS, SENT_COMMAND, PROCESSING, FINISHED } state;
  gboolean got_response;
} AppContext;

static AppContext global_ctx = {0};
static char *dpy_name;
static int dpy_sync = 0, win_noraise=0, win_new=0, win_ids_print;
static char *win_id = NULL;

static struct poptOption options[] = {
  {"display", '\0', POPT_ARG_STRING, &dpy_name, 0, N_("Specify X display to use."), "DISPLAY"},
  {"sync", '\0', POPT_ARG_NONE, &dpy_sync, 0, N_("Put the X connection in synchronous mode."), NULL},
  {"id", '\0', POPT_ARG_STRING, &win_id, 0, N_("Specify the X window ID of Netscape."), "WIN-ID"},
  
  {"noraise", '\0', POPT_ARG_NONE, &win_noraise, 0, N_("Don't raise the Netscape window."), NULL},
  {"newwin", '\0', POPT_ARG_NONE, &win_new, 0, N_("Show the given URL in a new window"), NULL},
  {"print-id", '\0', POPT_ARG_NONE, &win_ids_print, 0, N_("Print the X window ID of the utilized window"), NULL},
  {NULL}
};

static void mozilla_remote_check_window (Display *dpy, Window window);
static Window mozilla_remote_find_window (Display *dpy);
static void mozilla_remote_init_atoms (Display *dpy);
static gboolean mozilla_handle_xevents(GIOChannel *source, GIOCondition condition, gpointer data);
static int mozilla_remote_check_window (Display *dpy, Window window);

static Window
mozilla_loop_run(AppContext *ctx, Window cmd_window, int state)
{
  ctx->state = STARTING_PROCESS;
  ctx->result_window = 0;
  ctx->cmd_window = 0;
  ctx->got_response = FALSE;
  g_main_run(ctx->ml);

  return global_ctx.result_window;
}

static void
mozilla_loop_quit(AppContext *ctx)
{
  /* Do "check if we should quit main loop" stuff here */
}

static gboolean
mozilla_process_start(AppContext *ctx)
{
  /* XXX exec mozilla here */

  return FALSE;
}

static Window
mozilla_window_spawn(Display *dpy)
{
  g_idle_add(mozilla_process_start, &global_ctx);
  return mozilla_loop_run(&global_ctx, 0, STARTING_PROCESS);
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

static Window
mozilla_window_goto(Display *dpy, Window winid, const char *url)
{
  char command[2048], *escaped_url;
  if(!winid)
    winid = mozilla_window_find(dpy);
  if(!winid)
    winid = mozilla_window_spawn(dpy);
  if(!winid)
    return 0;

  escaped_url = escaped_url(url);
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

  dpy_name = getenv("DISPLAY");

  gnome_program_init("gnome_moz_remote", VERSION, argc, argv, GNOME_PARAM_POPT_TABLE, options, NULL);

  gnome_attributes_get(gnome_program_get(), GNOME_PARAM_POPT_CONTEXT, &ctx, NULL);

  args = poptGetArgs(ctx);
  if(!args || !args[0])
    return 0; /* Nothing to do */

  dpy = XOpenDisplay(dpy_name);

  if (!dpy)
    return 1;

  mozilla_remote_init_atoms(dpy);

  if(dpy_sync)
    XSynchronize (dpy, True);

  g_io_add_watch(g_io_channel_unix_new(ConnectionNumber(dpy)), G_IO_IN, mozilla_handle_xevents, dpy);

  global_ctx.dpy = dpy;

  if(args[1] && !win_new)
    {
      g_print("%s: You must specify --newwin on the command line when passing multiple URLs\n", argv[0]);
      return 1;
    }

  if(win_id)
    {
      global_ctx.cmd_window = strtol(win_id, NULL, 0);
      if(!mozilla_remote_check_window(dpy, global_ctx.cmd_window))
	global_ctx.cmd_window = 0;
    }
  else
    global_ctx.cmd_window = mozilla_remote_find_window(dpy);

  if (!global_ctx.cmd_window);

  for(i = 0; args[i]; i++)
    {
      Window utwin;


      utwin = mozilla_window_goto(dpy, utwin, args[i]);
      if(win_ids_print && utwin)
	{
	  g_print("%#x\n", utwin);
	}
      if(!utwin)
	break;
    }

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

static int
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
      return 1;
    }
  XFree (version);
  return 0;
}

static Window
mozilla_remote_command (Display *dpy, Window window, const char *command,
			Bool raise_p)
{
  int result = 0;
  Bool done = False;
  char *new_command = NULL;

  g_return_val_if_fail(window != 0, 0);

#ifdef DEBUG_PROPS
  g_printerr("gnome-moz-remote: (writing " MOZILLA_COMMAND_PROP
	     " \"%s\" to 0x%x)\n", command, (unsigned int) window);
#endif

  XChangeProperty (dpy, window, XA_MOZILLA_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) command,
		   strlen (command));
  global_ctx.cmd_window = window;
  global_ctx.result_window = 0;
  global_ctx.state = SENT_COMMAND;

  g_main_run(global_ctx.ml);

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

static gboolean
mozilla_handle_xevents(GIOChannel *source, GIOCondition condition, gpointer data)
{
  XEvent ev;
  AppContext *ctx = (AppContext *)data;

  XNextEvent(ctx->dpy, &ev);

  switch(ev.xany.type)
    {
    case CreateNotify:
      break;
    case DestroyNotify:
      
      break;
    case PropertyNotify:
      if (global_ctx.state != SENT_COMMAND)
	break;
      if(event.xproperty.atom != XA_MOZILLA_COMMAND)
	break;
      switch(event.xproperty.state)
	{
	case PropertyNewValue:
	  break;
	case PropertyDelete:
	  ctx->got_response = TRUE;
	  mozilla_loop_quit(ctx);
	  break;
	}      
      break;
    }
}
