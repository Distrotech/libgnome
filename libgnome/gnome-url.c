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

#include <config.h>
#include <string.h>
#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "gnome-i18nP.h"

#include <gconf/gconf-client.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>

#include "gnome-exec.h"
#include "gnome-util.h"
#include "gnome-init.h"
#include "gnome-i18n.h"
#include "gnome-gconfP.h"

#include "gnome-url.h"

#include <popt.h>

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
 */
gboolean
gnome_url_show_with_env (const char  *url,
                         char       **envp,
			 GError     **error)
{
	GnomeVFSResult result;

	g_return_val_if_fail (url != NULL, FALSE);

	result = gnome_vfs_url_show_with_env (url, envp);
	
	switch (result) {
	case GNOME_VFS_OK:
		return TRUE;
		break;
		
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
			     
	default:
		g_set_error (error,
			     GNOME_URL_ERROR,
			     GNOME_URL_ERROR_VFS,
			     _("Unknown error code: %d"), result);
	}

	return FALSE;
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
