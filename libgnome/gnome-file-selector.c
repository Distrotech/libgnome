/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomeFileSelector widget - a file selector widget.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gobject/gboxed.h>
#include <gobject/gvaluearray.h>
#include <gobject/gvaluetypes.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-file-selector.h>
#include "gnome-selectorP.h"

#include <libgnomevfs/gnome-vfs.h>

#define DEBUG_ASYNC_STUFF

#define PARENT_TYPE gnome_directory_filter_get_type()
GnomeDirectoryFilterClass *gnome_file_selector_parent_class = NULL;

typedef struct _GnomeFileAsyncData      GnomeFileAsyncData;
typedef struct _GnomeFileSelectorSubAsyncData   GnomeFileSelectorSubAsyncData;

struct _GnomeFileAsyncData {
    GnomeAsyncHandle *async_handle;

    GnomeAsyncType type;
    GnomeFileSelector *fselector;

    gboolean directory_ok;

    GSList *async_ops;
    gboolean completed;
    gboolean success;

    GnomeVFSAsyncHandle *vfs_handle;
    GnomeVFSURI *uri;
    GSList *uri_list;
    gint position;
    guint list_id;
};

struct _GnomeFileSelectorSubAsyncData {
    GnomeAsyncHandle *async_handle;

    GnomeFileAsyncData *async_data;
};

struct _GnomeFileSelectorPrivate {
    GnomeVFSDirectoryFilter *filter;
    GnomeVFSFileInfoOptions file_info_options;
};


static void gnome_file_selector_class_init  (GnomeFileSelectorClass *class);
static void gnome_file_selector_init        (GnomeFileSelector      *fselector);
static void gnome_file_selector_finalize    (GObject                *object);

static void      check_uri_handler              (GnomeDirectoryFilter     *filter,
                                                 const gchar              *uri,
						 gboolean                  directory_ok,
						 GnomeAsyncHandle         *async_handle);

static void      scan_directory_handler         (GnomeDirectoryFilter     *filter,
                                                 const gchar              *uri,
						 GnomeAsyncHandle         *async_handle);


static void
gnome_file_selector_class_init (GnomeFileSelectorClass *class)
{
    GObjectClass *object_class;
    GnomeDirectoryFilterClass *filter_class;

    gnome_file_selector_parent_class = g_type_class_peek_parent (class);

    object_class = (GObjectClass *) class;
    filter_class = (GnomeDirectoryFilterClass *) class;

    object_class->finalize = gnome_file_selector_finalize;

    filter_class->check_uri = check_uri_handler;
    filter_class->scan_directory = scan_directory_handler;
}

static void
free_the_async_data (gpointer data)
{
    GnomeFileAsyncData *async_data = data;
    GnomeFileSelector *fselector;

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));

    fselector = async_data->fselector;

    /* free the async data. */
    g_object_unref (G_OBJECT (async_data->fselector));
    _gnome_selector_deep_free_slist (async_data->uri_list);
    if (async_data->uri)
	gnome_vfs_uri_unref (async_data->uri);
    g_free (async_data);
}

static void
add_directory_async_done_cb (GnomeFileAsyncData *async_data)
{
    GnomeAsyncHandle *async_handle;

    g_return_if_fail (async_data != NULL);

    /* When we finish our directory reading, we set async_data->completed -
     * but we need to wait until all our async operations are completed as
     * well.
     */

    if (!async_data->completed || async_data->async_ops != NULL)
	return;

    async_handle = async_data->async_handle;

    gnome_async_handle_completed (async_handle, async_data->success);
    gnome_async_handle_remove (async_handle, async_data);
}

static void
add_directory_async_cb (GnomeVFSAsyncHandle *handle, GnomeVFSResult result,
			GList *list, guint entries_read, gpointer callback_data)
{
    GnomeFileAsyncData *async_data;
    GnomeFileSelector *fselector;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->vfs_handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_ASYNC_TYPE_ADD_URI);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    g_message (G_STRLOC ": %d (%s) - %p - %d", result,
	       gnome_vfs_result_to_string (result), list, entries_read);

    if (list != NULL) {
	BonoboArg *arg;
	GNOME_stringlist *stringlist;
	GList *c;

	stringlist = GNOME_stringlist__alloc ();
	stringlist->_maximum = g_list_length (list);
	stringlist->_buffer = CORBA_sequence_CORBA_string_allocbuf (stringlist->_maximum);

	for (c = list; c; c = c->next) {
	    GnomeVFSFileInfo *info = c->data;
	    GnomeVFSURI *uri;
	    gchar *text;

	    if (fselector->_priv->filter &&
		!gnome_vfs_directory_filter_apply (fselector->_priv->filter,
						   info)) {
		continue;
	    }

	    uri = gnome_vfs_uri_append_file_name (async_data->uri, info->name);
	    text = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	    stringlist->_buffer [stringlist->_length++] = CORBA_string_dup (text);

	    gnome_vfs_uri_unref (uri);
	    g_free (text);
	}

	arg = CORBA_any__alloc ();
	arg->_type = TC_GNOME_stringlist;
	arg->_release = TRUE;
	arg->_value = stringlist;

	gnome_async_handle_call_async_func (async_data->async_handle, arg);

	CORBA_free (arg);
    }

    if (result == GNOME_VFS_ERROR_NOT_A_DIRECTORY) {
	BonoboArg *arg;
	gchar *text;

	text = gnome_vfs_uri_to_string (async_data->uri, GNOME_VFS_URI_HIDE_NONE);

	arg = bonobo_arg_new (TC_string);
	BONOBO_ARG_SET_STRING (arg, text);

	gnome_async_handle_set_result (async_data->async_handle, arg);

	bonobo_arg_release (arg);
	g_free (text);
    }

    if (result != GNOME_VFS_OK) {
	/* Completed, but we may have async operations running. We set
	 * async_data->completed here to inform add_directory_async_done_cb()
	 * that we're done with our directory reading.
	 */
	if ((result == GNOME_VFS_ERROR_EOF) || (result == GNOME_VFS_ERROR_NOT_A_DIRECTORY))
	    async_data->success = TRUE;
	else
	    async_data->success = FALSE;
	async_data->completed = TRUE;
	add_directory_async_done_cb (async_data);
	return;
    }
}

static void
scan_directory_handler (GnomeDirectoryFilter *filter, const gchar *uri, GnomeAsyncHandle *async_handle)
{
    GnomeFileSelector *fselector;
    GnomeFileAsyncData *async_data;
    GnomeVFSURI *vfs_uri;

    g_return_if_fail (filter != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (filter));
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (filter);

#ifdef DEBUG_ASYNC_STUFF
    g_message (G_STRLOC ": %p - starting async reading (%s).",
	       fselector, uri);
#endif

    vfs_uri = gnome_vfs_uri_new (uri);

    async_data = g_new0 (GnomeFileAsyncData, 1);
    async_data->type = GNOME_ASYNC_TYPE_ADD_URI;
    async_data->fselector = fselector;
    async_data->uri = vfs_uri;
    async_data->async_handle = async_handle;

    gnome_vfs_uri_ref (async_data->uri);
    g_object_ref (G_OBJECT (async_data->fselector));

    gnome_async_handle_add (async_handle, async_data, free_the_async_data);

    gnome_vfs_async_load_directory_uri (&async_data->vfs_handle, vfs_uri,
					fselector->_priv->file_info_options,
					GNOME_VFS_DIRECTORY_FILTER_NONE,
					GNOME_VFS_DIRECTORY_FILTER_NODIRS,
					NULL, 1, add_directory_async_cb,
					async_data);

    gnome_vfs_uri_unref (vfs_uri);
}

void
gnome_file_selector_set_filter (GnomeFileSelector *fselector,
				GnomeVFSDirectoryFilter *filter)
{
    GnomeVFSDirectoryFilterNeeds needs;

    g_return_if_fail (fselector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));
    g_return_if_fail (filter != NULL);

    if (fselector->_priv->filter)
	gnome_vfs_directory_filter_destroy (fselector->_priv->filter);

    fselector->_priv->filter = filter;
    fselector->_priv->file_info_options = GNOME_VFS_FILE_INFO_DEFAULT;

    needs = gnome_vfs_directory_filter_get_needs (filter);
    if (needs & GNOME_VFS_DIRECTORY_FILTER_NEEDS_MIMETYPE)
	fselector->_priv->file_info_options |=
	    GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
	    GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE;
}

void
gnome_file_selector_clear_filter (GnomeFileSelector *fselector)
{
    g_return_if_fail (fselector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));

    if (fselector->_priv->filter)
	gnome_vfs_directory_filter_destroy (fselector->_priv->filter);

    fselector->_priv->filter = NULL;
    fselector->_priv->file_info_options = GNOME_VFS_FILE_INFO_DEFAULT;
}

static void
gnome_file_selector_init (GnomeFileSelector *fselector)
{
    fselector->_priv = g_new0 (GnomeFileSelectorPrivate, 1);
}

static void
check_uri_async_cb (GnomeVFSAsyncHandle *handle, GList *results, gpointer callback_data)
{
    GnomeFileAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList *list;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->vfs_handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_ASYNC_TYPE_CHECK_URI);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    g_assert ((results == NULL) || (results->next == NULL));

    for (list = results; list; list = list->next) {
	GnomeVFSGetFileInfoResult *file = list->data;
	GnomeAsyncHandle *async_handle = async_data->async_handle;

	/* better assert this than risking a crash. */
	g_assert (file != NULL);

	gnome_async_handle_remove (async_handle, async_data);

	if ((file->result != GNOME_VFS_OK) ||
	    ((file->file_info->type != GNOME_VFS_FILE_TYPE_REGULAR) &&
	     (file->file_info->type != GNOME_VFS_FILE_TYPE_DIRECTORY))) {
	    gnome_async_handle_completed (async_handle, FALSE);
	    return;
	}

	if ((file->file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) &&
	    !async_data->directory_ok) {
	    gnome_async_handle_completed (async_handle, FALSE);
	    return;
	}

	if (fselector->_priv->filter &&
	    !gnome_vfs_directory_filter_apply (fselector->_priv->filter,
					       file->file_info)) {
	    gnome_async_handle_completed (async_handle, FALSE);
	    return;
	}

	gnome_async_handle_completed (async_handle, TRUE);
	return;
    }
}

static void
check_uri_handler (GnomeDirectoryFilter *filter, const gchar *uri,
		   gboolean directory_ok, GnomeAsyncHandle *async_handle)
{
    GnomeFileAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList fake_list;

    g_return_if_fail (filter != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (filter));
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (filter);

    async_data = g_new0 (GnomeFileAsyncData, 1);
    async_data->async_handle = async_handle;
    async_data->type = GNOME_ASYNC_TYPE_CHECK_URI;
    async_data->fselector = fselector;
    async_data->uri = gnome_vfs_uri_new (uri);
    async_data->directory_ok = directory_ok;

    gnome_vfs_uri_ref (async_data->uri);
    g_object_ref (G_OBJECT (async_data->fselector));

    fake_list.data = async_data->uri;
    fake_list.prev = NULL;
    fake_list.next = NULL;

    gnome_async_handle_add (async_handle, async_data, free_the_async_data);

    gnome_vfs_async_get_file_info (&async_data->vfs_handle, &fake_list,
				   fselector->_priv->file_info_options,
				   check_uri_async_cb, async_data);

    gnome_vfs_uri_unref (fake_list.data);
}

static void
gnome_file_selector_finalize (GObject *object)
{
    GnomeFileSelector *fselector;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    fselector = GNOME_FILE_SELECTOR (object);

    g_free (fselector->_priv);
    fselector->_priv = NULL;

    G_OBJECT_CLASS (gnome_file_selector_parent_class)->finalize (object);
}

BONOBO_TYPE_FUNC (GnomeFileSelector, PARENT_TYPE, gnome_file_selector);
