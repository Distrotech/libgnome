/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-url.c
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <bonobo/bonobo-property-bag-client.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-util.h>
#include <libgnome/libgnome-init.h>
#include <libgnome/gnome-i18n.h>
#include "gnome-url.h"
#include <popt.h>

#define DEFAULT_HANDLER "gnome-moz-remote \"%s\""
#define INFO_HANDLER  "gnome-help-browser \"%s\""
#define MAN_HANDLER   "gnome-help-browser \"%s\""
#define GHELP_HANDLER "gnome-help-browser \"%s\""

static GSList *free_atexit = NULL;
GnomeURLHistoryCallback gnome_url_history_callback = NULL;

struct _GnomeURLDisplayContext {
  char *command;
  char *id;
};

static gchar *
gnome_url_default_handler (void)
{
	static gchar *default_handler = NULL;
	
	if (!default_handler) {
		gchar *str, *app;
		gboolean def;
		Bonobo_ConfigDatabase cdb;

		cdb = gnome_get_config_database ();

		str = bonobo_pbclient_get_string_with_default (cdb, "/Gnome/URL Handlers/default-show",
							       NULL, &def);
		g_message (G_STRLOC ": %d - `%s'", def, str);
		if (def) {
			app = gnome_is_program_in_path ("nautilus");
			if (app) {
				g_free (app);
				app = "nautilus \"%s\"";
			} else
				app = "gnome-help-browser \"%s\"";

			/* first time gnome_url_show is run -- set up some useful defaults */
			default_handler = DEFAULT_HANDLER;
			bonobo_pbclient_set_string (cdb, "/Gnome/URL Handlers/default-show",
						    default_handler, NULL);

			g_free (bonobo_pbclient_get_string_with_default(
				cdb, "/Gnome/URL Handlers/info-show", NULL, &def));
			if (def)
				bonobo_pbclient_set_string (cdb, "/Gnome/URL Handlers/info-show",
							    app, NULL);
			g_free (bonobo_pbclient_get_string_with_default(
				cdb, "/Gnome/URL Handlers/man-show", NULL, &def));
			if (def)
				bonobo_pbclient_set_string (cdb, "/Gnome/URL Handlers/man-show",
							    app, NULL);
			g_free (bonobo_pbclient_get_string_with_default(
				cdb, "/Gnome/URL Handlers/ghelp-show", NULL, &def));
			if (def)
				bonobo_pbclient_set_string (cdb, "/Gnome/URL Handlers/ghelp-show",
							    app, NULL);
		} else
			default_handler = str;
	}
	return default_handler;
}

/* returns FALSE and sets error if an error was encountered */
static gboolean
create_cmd(GnomeURLDisplayContext *rdc, const char *template,
	   const char *url, GnomeURLDisplayFlags flags, int pipe,
	   char ***cmd, int *argc,
	   GError **error)
{
	int temp_argc;
	const char **temp_argv;
	int i;

	/* we use a popt function as it does exactly what we want to do and
	   gnome already uses popt */
	if(poptParseArgvString(template, &temp_argc, &temp_argv) != 0) {
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_PARSE,
			     "Error parsing '%s'",
			     template);
		return FALSE;
	}
	if(temp_argc == 0) {
		free(temp_argv);
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_PARSE,
			     _("Error parsing '%s' (empty result)"),
			     template);
		return FALSE;
	}

	/* sort of a hack, if the command is gnome-moz-remote, first look
	 * if mozilla is in path */
	if(strcmp(temp_argv[0], "gnome-moz-remote") == 0) {
		Bonobo_ConfigDatabase cdb = gnome_get_config_database ();
		char *moz = bonobo_pbclient_get_string_with_default(cdb,
								    "/gnome-moz-remote/Mozilla/filename",
								    "netscape",
								    NULL);
		char *foo;

		foo = gnome_is_program_in_path(moz);
		if (foo == NULL) {
			g_set_error (error,
				     GNOME_URL_ERROR,
				     GNOME_URL_ERROR_NO_MOZILLA,
				     _("Mozilla not found as '%s'"),
				     moz);
			g_free(moz);
			return FALSE;
		}
		g_free(moz);
		g_free(foo);
	}
		

	*argc = temp_argc;

	if(flags & GNOME_URL_DISPLAY_NEWWIN)
		(*argc) ++;

	if(rdc && rdc->id)
		(*argc) ++;

	if(flags & GNOME_URL_DISPLAY_CLOSE)
		(*argc) ++;

	if(!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT))
		(*argc) ++;

	*cmd = g_new(char *, *argc + 1);

	for(i = 0; i < temp_argc; i++) {
		if(strcmp(temp_argv[i], "%s") == 0)
			(*cmd)[i] = g_strdup(url);
		else
			(*cmd)[i] = g_strdup(temp_argv[i]);
	}

	/* this must be free and not g_free because of how poptParseArgvString
	 * works.  This also frees all the strings inside the array */
	free(temp_argv);

	if(flags & GNOME_URL_DISPLAY_NEWWIN)
		(*cmd)[i++] = g_strdup("--newwin");

	if(rdc && rdc->id)
		(*cmd)[i++] = g_strdup_printf("--id=%s", rdc->id);

	if(flags & GNOME_URL_DISPLAY_CLOSE)
		(*cmd)[i++] = g_strdup("--closewin");

	if(!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT))
		(*cmd)[i++] = g_strdup_printf("--print-id=%d", pipe);

	(*cmd)[i++] = NULL;

	return TRUE;
}

/**
 * gnome_url_show_full
 * @display_context: An existing GnomeURLDisplayContext to use for displaying this URL in. May be NULL.
 * @url: URL to show
 * @url_type: The type of the URL (e.g. "help")
 * @flags: Flags changing the way the URL is displayed or handled.
 * @error: if an error happens GError will be set with domain of GNOME_URL_ERROR and code one of the GnomeURLError enum values.  May be NULL.
 *
 * Description:
 * Loads the given URL in an appropriate viewer.  The viewer is deduced from
 * the url_type and the protocol part of the URL.  That is all that the caller should know
 * about the function.
 *
 * 'display_context' is not presently used.
 *
 *
 * Internally, the handler for a given URL is deduced by looking in the
 * /Gnome/URL Handlers/<protocol>-show key in the user's configuration
 * files.  The key is a string that will be passed to gnome_execute_shell(),
 * after the %s is replaced with with the URL.  If that key can't be found,
 * it falls back to /Gnome/URL Handlers/default-show, and if that isn't
 * found, uses the contents of the DEFAULT_HANDLER macro in this file
 * (libgnome/gnome-url.c).
 *
 * If no /Gnome/URL Handlers keys are set, some sensible defaults are added
 * to the user's configuration files.
 *
 **/
GnomeURLDisplayContext *
gnome_url_show_full(GnomeURLDisplayContext *display_context, const char *url,
		    const char *url_type, GnomeURLDisplayFlags flags,
		    GError **error)
{
  char *retid = NULL;
  char *pos, *template = NULL;
  char path[PATH_MAX];
  gboolean def, free_template = FALSE;
  GnomeURLDisplayContext *rdc = display_context;
  Bonobo_ConfigDatabase cdb;

  g_return_val_if_fail (!(flags & GNOME_URL_DISPLAY_CLOSE) || display_context, display_context);

  cdb = gnome_get_config_database();

  if(flags & GNOME_URL_DISPLAY_CLOSE)
    url = url_type = ""; /* Stick in a dummy URL to make code happy */

  g_return_val_if_fail (url != NULL, display_context);

  if (gnome_url_history_callback && !(flags & GNOME_URL_DISPLAY_NO_HISTORY))
    {
      if(!gnome_url_history_callback(display_context, url, url_type, flags))
	return display_context;
    }

  if(!rdc)
    {
      if (url_type)
	{
	  g_snprintf(path, sizeof(path), "/Gnome/URL Handlers/%s-show", url_type);
	  template = bonobo_pbclient_get_string_with_default (cdb, path, NULL, &def);
	  if(def)
	    {
	      g_free(template);
	      template = NULL;
	    }
	  else
	    free_template = TRUE;
	}

      if (!template && (pos = strstr (url, "://")))
	{
	  char protocol[64];

	  g_snprintf(protocol, sizeof(protocol), "%.*s", (int)(pos - url - 3), url);
	  g_strdown (protocol);

	  g_snprintf (path, sizeof(path), "/Gnome/URL Handlers/%.*s-show", (int)(pos - url - 3), url);

	  template = bonobo_pbclient_get_string_with_default (cdb, path, NULL, &def);

	  if (def) {
	    g_free(template);
	    template = gnome_url_default_handler ();
	  } else
	    free_template = TRUE;
	}
      else
	/* no : ? -- this shouldn't happen.  Use default handler */
	template = gnome_url_default_handler ();
    }
  else
    {
      template = rdc->command;
    }


  if(!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT))
    {
      int pipes[2];
      char aline[LINE_MAX];
      FILE *fh;
      char **argv = NULL;
      int argc;
      int pid;

      if(pipe(pipes) != 0) {
	      g_warning(_("Cannot open a pipe: %s"), g_strerror(errno));
	      g_set_error (error,
			   GNOME_URL_ERROR,
			   GNOME_URL_ERROR_PIPE,
			   _("Cannot open a pipe: %s"),
			   g_strerror(errno));
	      flags |= GNOME_URL_DISPLAY_NO_RETURN_CONTEXT;
	      goto about_to_return;
      }

      if(!create_cmd(rdc, template, url, flags, pipes[1],
		     &argv, &argc, error)) {
	      close(pipes[0]);
	      close(pipes[1]);
	      g_warning(_("Cannot parse handler template: %s"), template);
	      flags |= GNOME_URL_DISPLAY_NO_RETURN_CONTEXT;
	      goto about_to_return;
      }

      pid = gnome_execute_async_fds (NULL, argc, argv, FALSE);
      close(pipes[1]);

      if(pid == -1) {
	      close(pipes[0]);
	      g_warning(_("Cannot execute command '%s'!"), argv[0]);
	      g_set_error (error,
			   GNOME_URL_ERROR,
			   GNOME_URL_ERROR_PIPE,
			   _("Cannot execute command '%s'!"),
			   argv[0]);
	      flags |= GNOME_URL_DISPLAY_NO_RETURN_CONTEXT;
	      goto about_to_return;
      }

      g_strfreev(argv);

      fh = fdopen(pipes[0], "r");

      if(fh)
	{
	  while(fgets(aline, sizeof(aline), fh))
	    {
	      g_strstrip(aline);
	      if(aline[0] == '0' && aline[1] == 'x')
		{
		  retid = g_strdup(aline);
		  break;
		}
	    }

	  fclose(fh);
	}
      close(pipes[0]);

      /* if no ID was found this will free the context as it would be bogus
       * anyway, and return an error */
      if(retid == NULL) {
	      g_set_error (error,
			   GNOME_URL_ERROR,
			   GNOME_URL_ERROR_NO_ID,
			   _("Command '%s' did not return an ID!"),
			   argv[0]);
	      flags |= GNOME_URL_DISPLAY_NO_RETURN_CONTEXT;
	      goto about_to_return;
      }
    }
  else
    {
      char **argv = NULL;
      int argc = 0;

      if(!create_cmd(rdc, template, url, flags, 0, &argv, &argc, error)) {
	      g_warning(_("Cannot parse handler template: %s"), template);
	      goto about_to_return;
      }

      if(gnome_execute_async (NULL, argc, argv) == -1) {
	      g_warning(_("Cannot execute command '%s'!"), argv[0]);
	      g_set_error (error,
			   GNOME_URL_ERROR,
			   GNOME_URL_ERROR_PIPE,
			   _("Cannot execute command '%s'!"),
			   argv[0]);
	      goto about_to_return;
      }

      g_strfreev(argv);
    }

about_to_return:

  if (flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT &&
      rdc)
    {
      gnome_url_display_context_free(rdc, 0, NULL);
      rdc = NULL;
    }

  if (!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT)
      && !rdc)
    {
      rdc = g_new0(struct _GnomeURLDisplayContext, 1);

      rdc->command = free_template?template:g_strdup(template);
      rdc->id = retid;
    }
  else if(rdc && retid)
    {
      g_free(rdc->id);
      rdc->id = retid;
    }
  else
    {
      if (free_template)
	g_free (template);

      g_free(retid);
    }

  return rdc;
}

gboolean
gnome_url_show(const gchar *url)
{
  GError *error = NULL;
  gnome_url_show_full(NULL, url, NULL, GNOME_URL_DISPLAY_NO_RETURN_CONTEXT,
		      &error);
  if (error == NULL)
	  return TRUE;
  g_error_free (error);
  return FALSE;
}

static void
gnome_udc_free_all(void)
{
  /* with GNOME_URL_DISPLAY_CLOSE it removes the context from free_atexit,
   * we cannot normally iterate through the list for that very reason here
   * since the list would be changing from underneath us */
  while(free_atexit)
      gnome_url_display_context_free((GnomeURLDisplayContext *)free_atexit->data,
				     GNOME_URL_DISPLAY_CLOSE, NULL);
}

/**
 * gnome_url_display_context_free:
 * @display_context: The url context
 * @flags: the flags
 * @error: if an error happens GError will be set with domain of GNOME_URL_ERROR and code one of the GnomeURLError enum values.  May be NULL.
 *
 * Description:
 **/
void
gnome_url_display_context_free(GnomeURLDisplayContext *display_context,
			       GnomeURLDisplayFlags flags,
			       GError **error)
{
  if(flags & GNOME_URL_DISPLAY_CLOSE_ATEXIT)
    {
      if(!free_atexit)
	g_atexit(gnome_udc_free_all);

      free_atexit = g_slist_prepend(free_atexit, display_context);
    }
  else
    {
      if(flags & GNOME_URL_DISPLAY_CLOSE)
	gnome_url_show_full(display_context, "", "", flags, error);

      free_atexit = g_slist_remove(free_atexit, display_context);

      g_free(display_context->command);
      g_free(display_context);
    }
}

GQuark
gnome_url_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
			g_quark_from_static_string ("gnome-url-error-quark");

	return error_quark;
}
