/* gnome-url.h
 * Copyright (C) 1998 James Henstridge <james@daa.com.au>
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

#ifndef GNOME_URL_H
#define GNOME_URL_H

#include <glib.h>
#include <libgnomebase/gnome-defs.h>

G_BEGIN_DECLS

typedef struct _GnomeURLDisplayContext GnomeURLDisplayContext;

typedef enum {
  GNOME_URL_DISPLAY_CLOSE_ATEXIT = 1<<0,
  GNOME_URL_DISPLAY_CLOSE = 1<<1,
  GNOME_URL_DISPLAY_NO_RETURN_CONTEXT = 1<<2,
  GNOME_URL_DISPLAY_NEWWIN = 1<<3,
  GNOME_URL_DISPLAY_NO_HISTORY = 1<<4 /* Don't call the history callback for this URL */
} GnomeURLDisplayFlags;

typedef enum {
  GNOME_URL_ERROR_EXEC, /* could not execute handler */
  GNOME_URL_ERROR_PARSE, /* if the parsing of the handler failed */
  GNOME_URL_ERROR_PIPE, /* if 'pipe' did not work when getting id,
			 * handler has not been executed */
  GNOME_URL_ERROR_NO_ID, /* if id could not be gotten, the handler has however
			  * been executed */
  GNOME_URL_ERROR_NO_MOZILLA /* could not find mozilla for gnome-moz-remote */
} GnomeURLError;

#define GNOME_URL_ERROR (gnome_url_error_quark ())
GQuark gnome_url_error_quark (void) G_GNUC_CONST;

typedef gboolean (*GnomeURLHistoryCallback)(GnomeURLDisplayContext *display_context,
					    const char *url,
					    const char *url_type,
					    GnomeURLDisplayFlags flags);

/* One callback for everyone, sorry folks. */
extern GnomeURLHistoryCallback gnome_url_history_callback;

/* This function displays the given URL in the appropriate viewer.  The
 * Appropriate viewer is user definable, according to these rules:
 *  1) Extract the protocol from URL.  This is defined as everything before
 *     the first colon
 *  2) Check if the key /Gnome/URL Handlers/<protocol>-show exists in the
 *     gnome config database.  If it does, use use this as a command
 *     template.  If it doesn't, check for the key
 *     /Gnome/URL Handlers/default-show, and if that doesn't exist fallback
 *     on the compiled in default.
 *  3) substitute the %s in the template with the URL.
 *  4) call gnome_execute_shell, with this expanded command as the second
 *     argument.
 */

/* if error is not NULL, it is set to one of the errors above */
GnomeURLDisplayContext *gnome_url_show_full(GnomeURLDisplayContext *display_context,
					    const char *url,
					    const char *url_type,
					    GnomeURLDisplayFlags flags,
					    GError **error);
void gnome_url_display_context_free(GnomeURLDisplayContext *display_context,
				    GnomeURLDisplayFlags flags,
				    GError **error);

/* returns FALSE on error, TRUE if everything went fine */
gboolean gnome_url_show(const char *url);

G_END_DECLS
#endif
