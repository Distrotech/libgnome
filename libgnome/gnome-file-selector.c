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
#include <gtk/gtkfilesel.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "gnome-macros.h"
#include <libgnomebase/gnome-i18n.h>
#include "gnome-file-selector.h"
#include "gnome-selectorP.h"
#include "gnome-entry.h"

#include <libgnomevfs/gnome-vfs.h>

#undef DEBUG_ASYNC_STUFF

typedef struct _GnomeFileSelectorAsyncData      GnomeFileSelectorAsyncData;
typedef struct _GnomeFileSelectorSubAsyncData   GnomeFileSelectorSubAsyncData;

struct _GnomeFileSelectorAsyncData {
    GnomeSelectorAsyncHandle *async_handle;

    GnomeSelectorAsyncType type;
    GnomeFileSelector *fselector;

    GSList *async_ops;
    gboolean completed;

    GnomeVFSAsyncHandle *vfs_handle;
    GnomeVFSURI *uri;
    GSList *uri_list;
    gint position;
    guint list_id;
};

struct _GnomeFileSelectorSubAsyncData {
    GnomeSelectorAsyncHandle *async_handle;

    GnomeFileSelectorAsyncData *async_data;
};

struct _GnomeFileSelectorPrivate {
    GnomeVFSDirectoryFilter *filter;
    GnomeVFSFileInfoOptions file_info_options;

    GtkWidget *browse_dialog;
};


static void gnome_file_selector_class_init  (GnomeFileSelectorClass *class);
static void gnome_file_selector_init        (GnomeFileSelector      *fselector);
static void gnome_file_selector_destroy     (GtkObject              *object);
static void gnome_file_selector_finalize    (GObject                *object);

static void      add_uri_handler                (GnomeSelector            *selector,
                                                 const gchar              *uri,
                                                 gint                      position,
						 guint                     list_id,
						 GnomeSelectorAsyncHandle *async_handle);

static void      add_uri_list_handler           (GnomeSelector            *selector,
						 GSList                   *list,
						 gint                      position,
						 guint                     list_id,
						 GnomeSelectorAsyncHandle *async_handle);

static void      add_directory_handler          (GnomeSelector            *selector,
                                                 const gchar              *uri,
                                                 gint                      position,
						 guint                     list_id,
						 GnomeSelectorAsyncHandle *async_handle);
static void      check_filename_handler         (GnomeSelector            *selector,
                                                 const gchar              *filename,
						 GnomeSelectorAsyncHandle *async_handle);
static void      check_directory_handler        (GnomeSelector            *selector,
                                                 const gchar              *directory,
						 GnomeSelectorAsyncHandle *async_handle);

static void      activate_entry_handler         (GnomeSelector   *selector);

static void      do_construct_handler           (GnomeSelector   *selector);


static GObject*
gnome_file_selector_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_properties);


/**
 * gnome_file_selector_get_type
 *
 * Returns the type assigned to the GnomeFileSelector widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeFileSelector, gnome_file_selector,
			 GnomeEntry, gnome_entry)

static void
gnome_file_selector_class_init (GnomeFileSelectorClass *class)
{
    GnomeSelectorClass *selector_class;
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    selector_class = (GnomeSelectorClass *) class;
    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    object_class->destroy = gnome_file_selector_destroy;
    gobject_class->finalize = gnome_file_selector_finalize;

    gobject_class->constructor = gnome_file_selector_constructor;

    selector_class->add_uri = add_uri_handler;

    selector_class->add_uri_list = add_uri_list_handler; 

    selector_class->add_directory = add_directory_handler;

    selector_class->activate_entry = activate_entry_handler;

    selector_class->check_filename = check_filename_handler;
    selector_class->check_directory = check_directory_handler;

    selector_class->do_construct = do_construct_handler;
}

static void
free_the_async_data (gpointer data)
{
    GnomeFileSelectorAsyncData *async_data = data;
    GnomeFileSelector *fselector;

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));

    fselector = async_data->fselector;

    /* free the async data. */
    gtk_object_unref (GTK_OBJECT (async_data->fselector));
    _gnome_selector_deep_free_slist (async_data->uri_list);
    if (async_data->uri)
	gnome_vfs_uri_unref (async_data->uri);
    g_free (async_data);
}

static void
add_uri_async_done_cb (GnomeFileSelectorAsyncData *async_data)
{
    GnomeSelectorAsyncHandle *async_handle;
    gchar *path;

    g_return_if_fail (async_data != NULL);

    /* When we finish our async reading, we set async_data->completed -
     * but we need to wait until all our async operations are completed as
     * well.
     */

#ifdef DEBUG_ASYNC_STUFF
    g_message (G_STRLOC ": %p - %d - %p", async_data, async_data->completed,
	       async_data->async_ops);
#endif

    if (!async_data->completed || async_data->async_ops != NULL)
	return;

    async_handle = async_data->async_handle;

    path = gnome_vfs_uri_to_string (async_data->uri,
				    GNOME_VFS_URI_HIDE_NONE);
    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, add_uri,
			       (GNOME_SELECTOR (async_data->fselector), path,
				async_data->position, async_data->list_id,
				async_handle));

#ifdef DEBUG_ASYNC_STUFF
    g_message (G_STRLOC ": %p - async reading completed (%s).",
	       async_data->fselector, path);
#endif

    g_free (path);

    _gnome_selector_async_handle_remove (async_handle, async_data);
}

static void
add_uri_async_add_cb (GnomeSelector *selector,
		      GnomeSelectorAsyncHandle *async_handle,
		      GnomeSelectorAsyncType async_type,
		      const char *uri, GError *error,
		      gboolean success, gpointer user_data)
{
    GnomeFileSelectorSubAsyncData *sub_async_data = user_data;
    GnomeFileSelectorAsyncData *async_data;

    g_return_if_fail (sub_async_data != NULL);
    g_assert (sub_async_data->async_data != NULL);
    async_data = sub_async_data->async_data;

    /* This operation is completed, remove it from the list and call
     * add_uri_async_done_cb() - this function will check whether
     * this was the last one and the uri checking is done as well.
     */

    async_data->async_ops = g_slist_remove (async_data->async_ops,
					    sub_async_data);

#ifdef DEBUG_ASYNC_STUFF
    g_message (G_STRLOC ": %p - %p - `%s'", async_data->async_ops,
	       sub_async_data, uri);
#endif

    add_uri_async_done_cb (async_data);
}


static void
add_uri_async_cb (GnomeVFSAsyncHandle *handle, GList *results,
		  gpointer callback_data)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList *list;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->vfs_handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_SELECTOR_ASYNC_TYPE_ADD_URI);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    for (list = results; list; list = list->next) {
	GnomeVFSGetFileInfoResult *file = list->data;
	gchar *uri;

	/* better assert this than risking a crash. */
	g_assert (file != NULL);

	uri = gnome_vfs_uri_to_string (file->uri, GNOME_VFS_URI_HIDE_NONE);

	if (file->result != GNOME_VFS_OK) {
	    g_message (G_STRLOC ": `%s': %s", uri,
		       gnome_vfs_result_to_string (file->result));
	    g_free (uri);
	    continue;
	}

	if (file->file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
	    GnomeFileSelectorSubAsyncData *sub_async_data;

	    sub_async_data = g_new0 (GnomeFileSelectorSubAsyncData, 1);
	    sub_async_data->async_data = async_data;

	    async_data->async_ops = g_slist_prepend
		(async_data->async_ops, sub_async_data);

	    gnome_selector_add_directory (GNOME_SELECTOR (fselector),
					  &sub_async_data->async_handle, uri,
					  async_data->position,
					  async_data->list_id,
					  add_uri_async_add_cb,
					  sub_async_data);

	    g_free (uri);
	    continue;
	}

	if (fselector->_priv->filter &&
	    !gnome_vfs_directory_filter_apply (fselector->_priv->filter,
					       file->file_info)) {

	    g_message (G_STRLOC ": dropped by directory filter");

	    g_free (uri);
	    continue;
	}

	if (file->file_info->type == GNOME_VFS_FILE_TYPE_REGULAR) {
	    GnomeFileSelectorSubAsyncData *sub_async_data;

	    sub_async_data = g_new0 (GnomeFileSelectorSubAsyncData, 1);
	    sub_async_data->async_data = async_data;

	    async_data->async_ops = g_slist_prepend
		(async_data->async_ops, sub_async_data);

	    gnome_selector_add_file (GNOME_SELECTOR (fselector),
				     &sub_async_data->async_handle, uri,
				     async_data->position, async_data->list_id,
				     add_uri_async_add_cb,
				     sub_async_data);

	    g_free (uri);
	    continue;
	}

	g_free (uri);
    }

    /* Completed, but we may have async operations running. We set
     * async_data->completed here to inform add_uri_async_done_cb()
     * that we're done with our async operation.
     */
    async_data->completed = TRUE;
    add_uri_async_done_cb (async_data);
}

void
add_uri_handler (GnomeSelector *selector, const gchar *uri, gint position,
		 guint list_id, GnomeSelectorAsyncHandle *async_handle)
{
    GnomeFileSelector *fselector;
    GnomeFileSelectorAsyncData *async_data;
    GList fake_list;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (position >= -1);
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->async_handle = async_handle;
    async_data->type = GNOME_SELECTOR_ASYNC_TYPE_ADD_URI;
    async_data->fselector = fselector;
    async_data->uri = gnome_vfs_uri_new (uri);
    async_data->position = position;
    async_data->list_id = list_id;

    gnome_vfs_uri_ref (async_data->uri);
    gtk_object_ref (GTK_OBJECT (async_data->fselector));

    fake_list.data = async_data->uri;
    fake_list.prev = NULL;
    fake_list.next = NULL;

    _gnome_selector_async_handle_add (async_handle, async_data,
				      free_the_async_data);

    gnome_vfs_async_get_file_info (&async_data->vfs_handle, &fake_list,
				   fselector->_priv->file_info_options,
				   add_uri_async_cb, async_data);

    gnome_vfs_uri_unref (fake_list.data);
}

static void
add_uri_list_async_done_cb (GnomeFileSelectorAsyncData *async_data)
{
    GnomeSelectorAsyncHandle *async_handle;

    g_return_if_fail (async_data != NULL);

    /* When we finish our directory reading, we set async_data->completed -
     * but we need to wait until all our async operations are completed as
     * well.
     */

    if (!async_data->completed || async_data->async_ops != NULL)
	return;

    async_handle = async_data->async_handle;

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, add_uri_list,
			       (GNOME_SELECTOR (async_data->fselector),
				async_data->uri_list, async_data->position,
				async_data->list_id, async_handle));

    _gnome_selector_async_handle_remove (async_handle, async_data);
}

static void
add_uri_list_async_cb (GnomeSelector *selector,
		       GnomeSelectorAsyncHandle *async_handle,
		       GnomeSelectorAsyncType async_type,
		       const char *uri, GError *error,
		       gboolean success, gpointer user_data)
{
    GnomeFileSelectorSubAsyncData *sub_async_data = user_data;
    GnomeFileSelectorAsyncData *async_data;

    g_return_if_fail (sub_async_data != NULL);
    g_assert (sub_async_data->async_data != NULL);
    async_data = sub_async_data->async_data;

    /* This operation is completed, remove it from the list and call
     * add_uri_list_async_done_cb() - this function will check whether
     * this was the last one and the directory reading is done as well.
     */

    async_data->async_ops = g_slist_remove (async_data->async_ops,
					    sub_async_data);

    add_uri_list_async_done_cb (async_data);
}

static void
add_uri_list_handler (GnomeSelector *selector, GSList *list,
		      gint position, guint list_id,
		      GnomeSelectorAsyncHandle *async_handle)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->async_handle = async_handle;
    async_data->type = GNOME_SELECTOR_ASYNC_TYPE_ADD_URI_LIST;
    async_data->fselector = fselector;
    async_data->position = position;
    async_data->uri_list = _gnome_selector_deep_copy_slist (list);
    async_data->list_id = list_id;

    gtk_object_ref (GTK_OBJECT (async_data->fselector));

    _gnome_selector_async_handle_add (async_handle, async_data,
				      free_the_async_data);

    for (c = list; c; c = c->next) {
	GnomeFileSelectorSubAsyncData *sub_async_data;

	sub_async_data = g_new0 (GnomeFileSelectorSubAsyncData, 1);
	sub_async_data->async_data = async_data;

	async_data->async_ops = g_slist_prepend
	    (async_data->async_ops, sub_async_data);

	gnome_selector_add_uri (GNOME_SELECTOR (fselector),
				&sub_async_data->async_handle,
				c->data, position, list_id,
				add_uri_list_async_cb,
				sub_async_data);
    }

    async_data->completed = TRUE;
    add_uri_list_async_done_cb (async_data);
}

static void
add_directory_async_done_cb (GnomeFileSelectorAsyncData *async_data)
{
    GnomeSelectorAsyncHandle *async_handle;
    gchar *path;

    g_return_if_fail (async_data != NULL);

    /* When we finish our directory reading, we set async_data->completed -
     * but we need to wait until all our async operations are completed as
     * well.
     */

    if (!async_data->completed || async_data->async_ops != NULL)
	return;

    async_handle = async_data->async_handle;

    path = gnome_vfs_uri_to_string (async_data->uri,
				    GNOME_VFS_URI_HIDE_NONE);
    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, add_directory,
			       (GNOME_SELECTOR (async_data->fselector), path,
				async_data->position, async_data->list_id,
				async_handle));

    g_free (path);

    _gnome_selector_async_handle_remove (async_handle, async_data);
}

static void
add_directory_async_file_cb (GnomeSelector *selector,
			     GnomeSelectorAsyncHandle *async_handle,
			     GnomeSelectorAsyncType async_type,
			     const char *uri, GError *error,
			     gboolean success, gpointer user_data)
{
    GnomeFileSelectorSubAsyncData *sub_async_data = user_data;
    GnomeFileSelectorAsyncData *async_data;

    g_return_if_fail (sub_async_data != NULL);
    g_assert (sub_async_data->async_data != NULL);
    async_data = sub_async_data->async_data;

    /* This operation is completed, remove it from the list and call
     * add_directory_async_done_cb() - this function will check whether
     * this was the last one and the directory reading is done as well.
     */

    async_data->async_ops = g_slist_remove (async_data->async_ops,
					    sub_async_data);

    add_directory_async_done_cb (async_data);
}

static void
add_directory_async_cb (GnomeVFSAsyncHandle *handle, GnomeVFSResult result,
			GList *list, guint entries_read, gpointer callback_data)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->vfs_handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    if (list != NULL) {
	GList *c;

	for (c = list; c; c = c->next) {
	    GnomeVFSFileInfo *info = c->data;
	    GnomeFileSelectorSubAsyncData *sub_async_data;
	    GnomeVFSURI *uri;
	    gchar *text;

	    if (fselector->_priv->filter &&
		!gnome_vfs_directory_filter_apply (fselector->_priv->filter,
						   info)) {
		continue;
	    }

	    uri = gnome_vfs_uri_append_file_name (async_data->uri, info->name);
	    text = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	    /* We keep a list of currently running async operations in
	     * async_data->async_ops. This is that add_directory_async_done_cb()
	     * knows whether we're really done or whether we still need to wait.
	     */

	    sub_async_data = g_new0 (GnomeFileSelectorSubAsyncData, 1);
	    sub_async_data->async_data = async_data;

	    async_data->async_ops = g_slist_prepend
		(async_data->async_ops, sub_async_data);

	    gnome_selector_add_file (GNOME_SELECTOR (fselector),
				     &sub_async_data->async_handle,
				     text, async_data->position,
				     async_data->list_id,
				     add_directory_async_file_cb,
				     sub_async_data);

	    gnome_vfs_uri_unref (uri);
	    g_free (text);
	}
    }

    if (result == GNOME_VFS_ERROR_EOF) {
	/* Completed, but we may have async operations running. We set
	 * async_data->completed here to inform add_directory_async_done_cb()
	 * that we're done with our directory reading.
	 */
	async_data->completed = TRUE;
	add_directory_async_done_cb (async_data);
    }
}

static void
add_directory_handler (GnomeSelector *selector, const gchar *uri,
		       gint position, guint list_id,
		       GnomeSelectorAsyncHandle *async_handle)
{
    GnomeFileSelector *fselector;
    GnomeFileSelectorAsyncData *async_data;
    GnomeVFSURI *vfs_uri;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (position >= -1);
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

#ifdef DEBUG_ASYNC_STUFF
    g_message (G_STRLOC ": %p - starting async reading (%s).",
	       selector, uri);
#endif

    vfs_uri = gnome_vfs_uri_new (uri);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->type = GNOME_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY;
    async_data->fselector = fselector;
    async_data->uri = vfs_uri;
    async_data->position = position;
    async_data->list_id = list_id;
    async_data->async_handle = async_handle;

    gnome_vfs_uri_ref (async_data->uri);
    gtk_object_ref (GTK_OBJECT (async_data->fselector));

    _gnome_selector_async_handle_add (async_handle, async_data,
				      free_the_async_data);

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
activate_entry_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;
    gchar *text;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, activate_entry,
			       (selector));

    text = gnome_selector_get_entry_text (selector);
    gnome_selector_add_uri (selector, NULL, text, 0, FALSE, NULL, NULL);
    g_free (text);
}

static void
gnome_file_selector_init (GnomeFileSelector *fselector)
{
    fselector->_priv = g_new0 (GnomeFileSelectorPrivate, 1);
}

static void
browse_dialog_cancel (GtkWidget *widget, gpointer data)
{
    GnomeFileSelector *fselector;
    GtkFileSelection *fs;

    g_assert (GNOME_IS_FILE_SELECTOR (data));

    fselector = GNOME_FILE_SELECTOR (data);
    fs = GTK_FILE_SELECTION (fselector->_priv->browse_dialog);

    if (GTK_WIDGET (fs)->window)
	gdk_window_lower (GTK_WIDGET (fs)->window);
    gtk_widget_hide (GTK_WIDGET (fs));
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
    GnomeFileSelector *fselector;
    GtkFileSelection *fs;
    const gchar *filename;

    g_assert (GNOME_IS_FILE_SELECTOR (data));

    fselector = GNOME_FILE_SELECTOR (data);

    fs = GTK_FILE_SELECTION (fselector->_priv->browse_dialog);
    filename = gtk_file_selection_get_filename (fs);

    gnome_selector_set_uri (GNOME_SELECTOR (fselector), NULL, filename,
			    NULL, NULL);

    if (GTK_WIDGET (fs)->window)
	gdk_window_lower (GTK_WIDGET (fs)->window);
    gtk_widget_hide (GTK_WIDGET (fs));
}

static void
check_uri_async_cb (GnomeVFSAsyncHandle *handle, GList *results, gpointer callback_data)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList *list;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->vfs_handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert ((async_data->type == GNOME_SELECTOR_ASYNC_TYPE_CHECK_FILENAME) ||
	      (async_data->type == GNOME_SELECTOR_ASYNC_TYPE_CHECK_DIRECTORY));

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    g_assert ((results == NULL) || (results->next == NULL));

    for (list = results; list; list = list->next) {
	GnomeVFSGetFileInfoResult *file = list->data;
	GnomeSelectorAsyncHandle *async_handle = async_data->async_handle;

	/* better assert this than risking a crash. */
	g_assert (file != NULL);

	_gnome_selector_async_handle_remove (async_handle, async_data);

	if (file->result != GNOME_VFS_OK) {
	    _gnome_selector_async_handle_completed (async_handle, FALSE);
	    return;
	}

	if ((file->file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) &&
	    (async_data->type != GNOME_SELECTOR_ASYNC_TYPE_CHECK_DIRECTORY)) {
	    _gnome_selector_async_handle_completed (async_handle, FALSE);
	    return;
	}

	if (fselector->_priv->filter &&
	    !gnome_vfs_directory_filter_apply (fselector->_priv->filter,
					       file->file_info)) {
	    _gnome_selector_async_handle_completed (async_handle, FALSE);
	    return;
	}

	_gnome_selector_async_handle_completed (async_handle, TRUE);
	return;
    }
}

static void
check_uri_handler (GnomeSelector *selector, const gchar *uri,
		   GnomeSelectorAsyncType async_type,
		   GnomeSelectorAsyncHandle *async_handle)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList fake_list;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->async_handle = async_handle;
    async_data->type = GNOME_SELECTOR_ASYNC_TYPE_CHECK_FILENAME;
    async_data->fselector = fselector;
    async_data->uri = gnome_vfs_uri_new (uri);

    gnome_vfs_uri_ref (async_data->uri);
    gtk_object_ref (GTK_OBJECT (async_data->fselector));

    fake_list.data = async_data->uri;
    fake_list.prev = NULL;
    fake_list.next = NULL;

    _gnome_selector_async_handle_add (async_handle, async_data,
				      free_the_async_data);

    gnome_vfs_async_get_file_info (&async_data->vfs_handle, &fake_list,
				   fselector->_priv->file_info_options,
				   check_uri_async_cb, async_data);

    gnome_vfs_uri_unref (fake_list.data);
}

static void
check_filename_handler (GnomeSelector *selector, const gchar *filename,
			GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (filename != NULL);
    g_return_if_fail (async_handle != NULL);

    check_uri_handler (selector, filename, GNOME_SELECTOR_ASYNC_TYPE_CHECK_FILENAME, async_handle);
}

static void
check_directory_handler (GnomeSelector *selector, const gchar *directory,
			 GnomeSelectorAsyncHandle *async_handle)
{
    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (directory != NULL);
    g_return_if_fail (async_handle != NULL);

    check_uri_handler (selector, directory, GNOME_SELECTOR_ASYNC_TYPE_CHECK_DIRECTORY, async_handle);
}

static gboolean
get_value_boolean (GnomeFileSelector *fselector, const gchar *prop_name)
{
    GValue value = { 0, };
    gboolean retval;

    g_value_init (&value, G_TYPE_BOOLEAN);
    g_object_get_property (G_OBJECT (fselector), prop_name, &value);
    retval = g_value_get_boolean (&value);
    g_value_unset (&value);

    return retval;
}

static gboolean
has_value_widget (GnomeFileSelector *fselector, const gchar *prop_name)
{
	GValue value = { 0, };
	gboolean retval;

	g_value_init (&value, GTK_TYPE_WIDGET);
	g_object_get_property (G_OBJECT (fselector), prop_name, &value);
	retval = g_value_get_object (&value) != NULL;
	g_value_unset (&value);

	return retval;
}

static void
do_construct_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    if (get_value_boolean (fselector, "want_default_behaviour")) {
	g_object_set (G_OBJECT (fselector),
		      "want_default_behaviour", FALSE,
		      "use_default_entry_widget", TRUE,
		      "use_default_selector_widget", TRUE,
		      "use_default_browse_dialog", TRUE,
		      "want_browse_button", TRUE,
		      "want_clear_button", FALSE,
		      "want_default_button", FALSE,
		      NULL);
    }

    /* Create the default browser dialog if requested. */
    if (get_value_boolean (fselector, "use_default_browse_dialog") &&
	!has_value_widget (fselector, "browse_dialog")) {
	GtkWidget *filesel_widget;
	GtkFileSelection *filesel;
	GValue value = { 0, };

	g_value_init (&value, G_TYPE_STRING);
	g_object_get_property (G_OBJECT (fselector), "dialog_title", &value);

	filesel_widget = gtk_file_selection_new (g_value_get_string (&value));
	filesel = GTK_FILE_SELECTION (filesel_widget);

	g_value_unset (&value);

	gtk_signal_connect (GTK_OBJECT (filesel->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC (browse_dialog_cancel),
			    fselector);
	gtk_signal_connect (GTK_OBJECT (filesel->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (browse_dialog_ok),
			    fselector);

	fselector->_priv->browse_dialog = GTK_WIDGET (filesel);

	g_value_init (&value, GTK_TYPE_WIDGET);
	g_value_set_object (&value, G_OBJECT (filesel));
	g_object_set_property (G_OBJECT (fselector), "browse_dialog", &value);
	g_value_unset (&value);
    }

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, do_construct, (selector));
}


static GObject*
gnome_file_selector_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_properties)
{
    GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type,
								  n_construct_properties,
								  construct_properties);
    GnomeFileSelector *fselector = GNOME_FILE_SELECTOR (object);

    g_message (G_STRLOC ": %d - %d", type, GNOME_TYPE_FILE_SELECTOR);

    if (type == GNOME_TYPE_FILE_SELECTOR)
	gnome_selector_do_construct (GNOME_SELECTOR (fselector));

    return object;
}

/**
 * gnome_file_selector_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeFileSelector widget.  If  @history_id
 * is not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeFileSelector widget.
 */
GtkWidget *
gnome_file_selector_new (const gchar *history_id,
			 const gchar *dialog_title)
{
    GnomeFileSelector *fselector;

    fselector = g_object_new (gnome_file_selector_get_type (),
			      "history_id", history_id,
			      "dialog_title", dialog_title,
			       NULL);

    return GTK_WIDGET (fselector);
}

static void
gnome_file_selector_destroy (GtkObject *object)
{
    GnomeFileSelector *fselector;

    /* remember, destroy can be run multiple times! */

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    fselector = GNOME_FILE_SELECTOR (object);

    GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
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

    GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}
