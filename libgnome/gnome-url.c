/* gnome-url.c
 * Copyright (C) 1998, James Henstridge <james@daa.com.au>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-config.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-portability.h>
#include "gnome-url.h"

#define DEFAULT_HANDLER "gnome-moz-remote \"%s\""
#define INFO_HANDLER  "gnome-help-browser \"%s\""
#define MAN_HANDLER   "gnome-help-browser \"%s\""
#define GHELP_HANDLER "gnome-help-browser \"%s\""

static GSList *free_atexit = NULL;

struct _GnomeURLDisplayContext {
  char *command;
  char *id;
};

static gchar *
gnome_url_default_handler (void)
{
	static gchar *default_handler = 0;
	
	if (!default_handler) {
		gchar *str;
		gboolean def;
		str = gnome_config_get_string_with_default ("/Gnome/URL Handlers/default-show",
							    &def);
		if (def) {
			/* first time gnome_url_show is run -- set up some useful defaults */
			default_handler = DEFAULT_HANDLER;
			gnome_config_set_string ("/Gnome/URL Handlers/default-show", default_handler);

			g_free (gnome_config_get_string_with_default(
				"/Gnome/URL Handlers/info-show", &def));
			if (def)
				gnome_config_set_string ("/Gnome/URL Handlers/info-show", INFO_HANDLER);
			g_free (gnome_config_get_string_with_default(
				"/Gnome/URL Handlers/man-show", &def));
			if (def)
				gnome_config_set_string ("/Gnome/URL Handlers/man-show", MAN_HANDLER);
			g_free (gnome_config_get_string_with_default(
				"/Gnome/URL Handlers/ghelp-show", &def));
			if (def)
				gnome_config_set_string ("/Gnome/URL Handlers/ghelp-show",
							 GHELP_HANDLER);
			gnome_config_sync_file ("/Gnome/");
		} else
			default_handler = str;
	}
	return default_handler;
}

/**
 * gnome_url_show
 * @display_context: An existing GnomeURLDisplayContext to use for displaying this URL in. May be NULL.
 * @url: URL to show
 * @url_type: The type of the URL (e.g. "help")
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
 * found, uses the contents of the DEFAULT_HANDLER macro in this file.
 *
 * If no /Gnome/URL Handlers keys are set, some sensible defaults are added
 * to the user's configuration files.
 *
 **/
GnomeURLDisplayContext
gnome_url_show_full(GnomeURLDisplayContext display_context, const char *url, const char *url_type,
		    GnomeURLDisplayFlags flags)
{
  gint len;
  char *retid = NULL;
  gchar *pos, *template = NULL, *cmd;
  char path[PATH_MAX];
  gboolean def, free_template = FALSE;
  GnomeURLDisplayContext rdc = display_context;

  g_return_val_if_fail (!(flags & GNOME_URL_DISPLAY_CLOSE) || display_context, display_context);

  if(flags & GNOME_URL_DISPLAY_CLOSE)
    url = url_type = ""; /* Stick in a dummy URL to make code happy */

  g_return_val_if_fail (url != NULL, display_context);

  if(!rdc)
    {
      if (url_type)
	{
	  g_snprintf(path, sizeof(path), "/Gnome/URL Handlers/%s-show", url_type);
	  template = gnome_config_get_string_with_default (path, &def);
	  if(def)
	    {
	      g_free(template);
	      template = NULL;
	    }
	}

      if (!template && (pos = strstr (url, "://")))
	{
	  char protocol[64];

	  g_snprintf(protocol, sizeof(protocol), "%.*s", pos - url - 3, url);
	  g_strdown (protocol);

	  g_snprintf (path, sizeof(path), "/Gnome/URL Handlers/%.*s-show", pos - url - 3, url);

	  template = gnome_config_get_string_with_default (path, &def);

	  if (def)
	    template = gnome_url_default_handler ();
	  else
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
	
  len = strlen (template) + strlen (url) + 2 + strlen("--print-id=1234567890 --id=0xFFFFFFFF --closewin");
  
  cmd = g_alloca (len);
  /* Attempt to make sure putting too many %s's in "template" won't spaz out this sprintf */
  g_snprintf (cmd, len, template, url, url, url, url, url);

  if(flags & GNOME_URL_DISPLAY_NEWWIN)
    strcat(cmd, " --newwin");

  if(rdc && rdc->id)
    {
      strcat(cmd, " --id=");
      strcat(cmd, rdc->id);
    }
  if(flags & GNOME_URL_DISPLAY_CLOSE)
    {
      strcat(cmd, " --closewin");
    }

  if(!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT))
    {
      int pipes[2];
      char aline[LINE_MAX];
      FILE *fh;

      pipe(pipes);
      strcat(cmd, " --print-id=");
      sprintf(cmd + strlen(cmd), "%d", pipes[1]);

      gnome_execute_shell_fds(NULL, cmd, FALSE);
      close(pipes[1]);

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
    }
  else
    {
      gnome_execute_shell (NULL, cmd);

      if(rdc)
	{
	  gnome_url_display_context_free(rdc, 0);
	  rdc = NULL;
	}
    }

  if (!(flags & GNOME_URL_DISPLAY_NO_RETURN_CONTEXT)
      && !rdc)
    {
      rdc = g_new0(struct _GnomeURLDisplayContext, 1);

      rdc->command = free_template?template:g_strdup(template);
      rdc->id = retid;
    }
  else if(rdc)
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

void
gnome_url_show(const gchar *url)
{
  gnome_url_show_full(NULL, url, NULL, GNOME_URL_DISPLAY_NO_RETURN_CONTEXT);
}

static void
gnome_udc_free_all(void)
{
  GSList *cur;

  for(cur = free_atexit; cur; cur = cur->next)
    {
      gnome_url_display_context_free((GnomeURLDisplayContext)cur->data, GNOME_URL_DISPLAY_CLOSE);
    }

  g_slist_free(free_atexit); free_atexit = NULL;
}

void
gnome_url_display_context_free(GnomeURLDisplayContext display_context, GnomeURLDisplayFlags flags)
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
	gnome_url_show_full(display_context, "", "", flags);

      free_atexit = g_slist_remove(free_atexit, display_context);

      g_free(display_context->command);
      g_free(display_context);
    }
}
