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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n-lib.h>

#include <gconf/gconf-client.h>
#ifndef G_OS_WIN32
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#else
#include <windows.h>
#endif

#include "gnome-exec.h"
#include "gnome-util.h"
#include "gnome-init.h"
#include "gnome-gconfP.h"

#include "gnome-url.h"

/**
 * gnome_url_show_with_env:
 * @url: The url or path to display.
 * @envp: child's environment, or %NULL to inherit parent's.
 * @error: Used to store any errors that result from trying to display the @url.
 *
 * Description: Like gnome_url_show(), but gnome_vfs_url_show_with_env
 * will be called with the given envirnoment.
 *
 * Returns: %TRUE if everything went fine, %FALSE otherwise (in which case
 * @error will contain the actual error).
 *
 * Since: 2.2
 */
gboolean
gnome_url_show_with_env (const char  *url,
                         char       **envp,
			 GError     **error)
{
#ifndef G_OS_WIN32
	GnomeVFSResult result;
	GnomeVFSURI *vfs_uri;

	g_return_val_if_fail (url != NULL, FALSE);

	result = gnome_vfs_url_show_with_env (url, envp);

	switch (result) {
	case GNOME_VFS_OK:
		return TRUE;

	case GNOME_VFS_ERROR_INTERNAL:
		g_set_error (error,
		             GNOME_URL_ERROR,
			     GNOME_URL_ERROR_VFS,
			     _("Unknown internal error while displaying this location."));
		break;

	case GNOME_VFS_ERROR_BAD_PARAMETERS:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_URL,
			     _("The specified location is invalid."));
		break;

	case GNOME_VFS_ERROR_PARSE:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_PARSE,
			     _("There was an error parsing the default action command associated "
			       "with this location."));
		break;

	case GNOME_VFS_ERROR_LAUNCH:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_LAUNCH,
			     _("There was an error launching the default action command associated "
			       "with this location."));
		break;

	case GNOME_VFS_ERROR_NO_DEFAULT:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_NO_DEFAULT,
			     _("There is no default action associated with this location."));
		break;

	case GNOME_VFS_ERROR_NOT_SUPPORTED:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_NOT_SUPPORTED,
			     _("The default action does not support this protocol."));
		break;

	case GNOME_VFS_ERROR_CANCELLED:
		g_set_error (error,
		             GNOME_URL_ERROR,
			     GNOME_URL_ERROR_CANCELLED,
			     _("The request was cancelled."));
		break;

	case GNOME_VFS_ERROR_HOST_NOT_FOUND:
		{
			vfs_uri = gnome_vfs_uri_new (url);
			if (gnome_vfs_uri_get_host_name (vfs_uri) != NULL) {
				g_set_error (error,
					     GNOME_URL_ERROR,
					     GNOME_URL_ERROR_VFS,
					     _("The host \"%s\" could not be found."),
					     gnome_vfs_uri_get_host_name (vfs_uri));
			} else {
				g_set_error (error,
					     GNOME_URL_ERROR,
					     GNOME_URL_ERROR_VFS,
					     _("The host could not be found."));
			}
			gnome_vfs_uri_unref (vfs_uri);
		}
		break;

	case GNOME_VFS_ERROR_INVALID_URI:
	case GNOME_VFS_ERROR_NOT_FOUND:
		g_set_error (error,
		             GNOME_URL_ERROR,
			     GNOME_URL_ERROR_VFS,
			     _("The location or file could not be found."));
		break;

	case GNOME_VFS_ERROR_LOGIN_FAILED:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_VFS,
			     _("The login has failed."));
		break;
	default:
		g_set_error_literal (error,
				     GNOME_URL_ERROR,
				     GNOME_URL_ERROR_VFS,
				     gnome_vfs_result_to_string (result));
	}

	return FALSE;
#else
	/* FIXME: Just call ShellExecute... Good enough? */

	if ((int) ShellExecute (HWND_DESKTOP, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32) {
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_LAUNCH,
			     _("There was an error launching the default action command associated "
			       "with this location."));
		return FALSE;
	}

	return TRUE;
#endif
}

/**
 * gnome_url_show:
 * @url: The url or path to display. The path can be relative to the current working
 * directory or the user's home directory. This function will convert it into a fully
 * qualified url using the gnome_url_get_from_input function.
 * @error: Used to store any errors that result from trying to display the @url.
 *
 * Once the input has been converted into a fully qualified url this function
 * calls gnome_vfs_url_show. Any error codes returned by gnome-vfs will be wrapped
 * in the error parameter. All errors comes from the %GNOME_URL_ERROR% domain.
 *
 * Returns: %TRUE if everything went fine, %FALSE otherwise (in which case
 * @error will contain the actual error).
 */
gboolean
gnome_url_show (const char  *url,
		GError     **error)
{
	return gnome_url_show_with_env (url, NULL, error);
}

/**
 * gnome_url_error_quark
 *
 * Returns: A quark representing gnome-url module errors.
 */
GQuark
gnome_url_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
			g_quark_from_static_string ("gnome-url-error-quark");

	return error_quark;
}
