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

#include "gnome-i18nP.h"

#include <gconf/gconf-client.h>
#include <libgnome/gnome-exec.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-init.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-url.h>
#include <popt.h>

#define DEFAULT_HANDLER "mozilla \"%s\""
#define INFO_HANDLER  "gnome-help-browser \"%s\""
#define MAN_HANDLER   "gnome-help-browser \"%s\""
#define GHELP_HANDLER "gnome-help-browser \"%s\""

static gchar *
gnome_url_default_handler (void)
{
	static gchar *default_handler = NULL;
	
	if (!default_handler) {
		gchar *str, *app;
		GConfClient *client;

		client = gconf_client_get_default ();
		
		str = gconf_client_get_string (client, "/desktop/gnome/url-handlers/default-show", NULL);

		if (str) {
			default_handler = str;
		}
		else {
			/* It's the first time gnome_url_show is run, so up some useful defaults */

			/* FIXME: Should we remove gnome-help-browser here? */
			app = gnome_is_program_in_path ("nautilus");
			if (app) {
				g_free (app);
				app = "nautilus \"%s\"";
			} else
				app = "gnome-help-browser \"%s\"";

			default_handler = DEFAULT_HANDLER;
			gconf_client_set_string (client, "/desktop/gnome/url-handlers/default-show",
						 default_handler, NULL);

			if (gconf_client_dir_exists (client, "/desktop/gnome/url-handlers/info-show", NULL) == FALSE) {
				gconf_client_set_string (client, "/desktop/gnome/url-handlers/info-show", app, NULL);
			}

			if (gconf_client_dir_exists (client, "/desktop/gnome/url-handlers/man-show", NULL) == FALSE) {
				gconf_client_set_string (client, "/desktop/gnome/url-handlers/man-show", app, NULL);
			}

			if (gconf_client_dir_exists (client, "/desktop/gnome/url-handlers/ghelp-show", NULL) == FALSE) {
				gconf_client_set_string (client, "/desktop/gnome/url-handlers/ghelp-show", app, NULL);
			}
		}

		g_object_unref (G_OBJECT (client));
	}
	
	return default_handler;
}

gboolean
gnome_url_show(const gchar *url, GError **error)
{
	gint i;
	gchar *pos, *template;
	gboolean free_template = FALSE;
	int argc;
	char **argv;
	
	g_return_val_if_fail (url != NULL, FALSE);

	pos = strchr (url, ':');

	if (pos != NULL) {
		gchar *protocol, *path;
		GConfClient *client;
		
		g_return_val_if_fail (pos >= url, FALSE);

		protocol = g_new (gchar, pos - url + 1);
		strncpy (protocol, url, pos - url);
		protocol[pos - url] = '\0';
		g_strdown (protocol);

		path = g_strconcat ("/desktop/gnome/url-handlers/", protocol, "-show", NULL);
		client = gconf_client_get_default ();

		template = gconf_client_get_string (client, path, NULL);

		if (template == NULL) {
			template = gnome_url_default_handler ();
		}
		else {
			free_template = TRUE;
		}

		g_free (protocol);
	}
	else {
		/* no ':' ? this shouldn't happen. Use default handler */
		template = gnome_url_default_handler ();
	}

	/* We use a popt function as it does exactly what we want to do and
	   gnome already uses popt */
	if (poptParseArgvString (template, &argc, &argv) != 0) {
		/* can't parse */
		g_warning ("Parse error of '%s'", template);
		
		return FALSE;
	}
	
	/* We can just replace the entry in the array since the
	 * array is all in one buffer, we won't leak */
	for (i = 0; i < argc; i++) {
		if (strcmp (argv[i], "%s") == 0)
				/* we can safely cast as it will not get freed nor
				 * modified */
			argv[i] = (char *)url;
	}
	
	/* use execute async, and not the shell, shell is evil and a
	 * security hole */
	/* FIXME: Should we use g_spawn here? */
	gnome_execute_async (NULL, argc, argv);
	
	if (free_template)
		g_free (template);
	
	/* the way the poptParseArgvString works is that the entire thing
	 * is allocated as one buffer, so just free will suffice, also
	 * it must be free and not g_free */
	free(argv);

	return TRUE;
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
