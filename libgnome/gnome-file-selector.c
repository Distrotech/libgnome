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
#include "libgnome/libgnomeP.h"
#include "gnome-file-selector.h"
#include "gnome-selectorP.h"
#include "gnome-entry.h"

#include <libgnomevfs/gnome-vfs.h>

typedef struct _GnomeFileSelectorAsyncData      GnomeFileSelectorAsyncData;
typedef enum   _GnomeFileSelectorAsyncType      GnomeFileSelectorAsyncType;

enum _GnomeFileSelectorAsyncType {
    GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_FILE,
    GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY
};

struct _GnomeFileSelectorAsyncData {
    GnomeFileSelectorAsyncType type;
    GnomeFileSelector *fselector;
    GnomeVFSAsyncHandle *handle;
    GnomeVFSURI *uri;
    gint position;
};

struct _GnomeFileSelectorPrivate {
    GSList *async_ops;

    GnomeVFSDirectoryFilter *filter;
    GnomeVFSFileInfoOptions file_info_options;
};


static void gnome_file_selector_class_init  (GnomeFileSelectorClass *class);
static void gnome_file_selector_init        (GnomeFileSelector      *fselector);
static void gnome_file_selector_destroy     (GtkObject              *object);
static void gnome_file_selector_finalize    (GObject                *object);

static void      add_file_handler               (GnomeSelector   *selector,
                                                 const gchar     *uri,
                                                 gint             position);
static void      add_directory_handler          (GnomeSelector   *selector,
                                                 const gchar     *uri,
                                                 gint             position);

static void      activate_entry_handler         (GnomeSelector   *selector);

static gboolean  check_filename_handler         (GnomeSelector   *selector,
                                                 const gchar     *filename);
static gboolean  check_directory_handler        (GnomeSelector   *selector,
                                                 const gchar     *directory);
static void      stop_loading_handler           (GnomeSelector   *selector);


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

    selector_class->add_file = add_file_handler;
    selector_class->add_directory = add_directory_handler;

    selector_class->activate_entry = activate_entry_handler;

    selector_class->check_filename = check_filename_handler;
    selector_class->check_directory = check_directory_handler;

    selector_class->stop_loading = stop_loading_handler;
}

static void
free_the_async_data (GnomeFileSelector *fselector,
		     GnomeFileSelectorAsyncData *async_data)
{
    g_return_if_fail (fselector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));
    g_return_if_fail (async_data != NULL);

    /* free the async data. */
    fselector->_priv->async_ops = g_slist_remove
	(fselector->_priv->async_ops, async_data);
    gtk_object_unref (GTK_OBJECT (async_data->fselector));
    gnome_vfs_uri_unref (async_data->uri);
    g_free (async_data);
}

static void
add_file_async_cb (GnomeVFSAsyncHandle *handle, GList *results,
		   gpointer callback_data)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;
    GList *list;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_FILE);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    g_assert (g_slist_find (fselector->_priv->async_ops, async_data));

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
	    gnome_selector_add_directory (GNOME_SELECTOR (fselector), uri,
					  async_data->position, FALSE);
	    g_free (uri);
	    continue;
	}

	if (fselector->_priv->filter &&
	    !gnome_vfs_directory_filter_apply (fselector->_priv->filter,
					       file->file_info)) {
	    g_free (uri);
	    continue;
	}

	gtk_signal_emit_by_name (GTK_OBJECT (fselector), "do_add_file",
				 uri, async_data->position);

	g_free (uri);
    }

    free_the_async_data (fselector, async_data);
}

static void
add_file_handler (GnomeSelector *selector, const gchar *uri, gint position)
{
    GnomeFileSelector *fselector;
    GnomeFileSelectorAsyncData *async_data;
    GList fake_list;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (position >= -1);
    g_return_if_fail (uri != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->type = GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_FILE;
    async_data->fselector = fselector;
    async_data->uri = gnome_vfs_uri_new (uri);
    async_data->position = position;

    gnome_vfs_uri_ref (async_data->uri);
    gtk_object_ref (GTK_OBJECT (fselector));

    fake_list.data = async_data->uri;
    fake_list.prev = NULL;
    fake_list.next = NULL;

    fselector->_priv->async_ops = g_slist_prepend
	(fselector->_priv->async_ops, async_data);

    gnome_vfs_async_get_file_info (&async_data->handle, &fake_list,
				   fselector->_priv->file_info_options,
				   add_file_async_cb, async_data);

    gnome_vfs_uri_unref (fake_list.data);
}

static void
add_directory_async_cb (GnomeVFSAsyncHandle *handle, GnomeVFSResult result,
			GnomeVFSDirectoryList *list, guint entries_read,
			gpointer callback_data)
{
    GnomeFileSelectorAsyncData *async_data;
    GnomeFileSelector *fselector;

    g_return_if_fail (callback_data != NULL);

    async_data = callback_data;
    g_assert (async_data->handle == handle);
    g_assert (GNOME_IS_FILE_SELECTOR (async_data->fselector));
    g_assert (async_data->type == GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY);

    fselector = GNOME_FILE_SELECTOR (async_data->fselector);

    g_assert (g_slist_find (fselector->_priv->async_ops, async_data));

    if (list != NULL) {
	GnomeVFSFileInfo *info;

	info = gnome_vfs_directory_list_current (list);
	while (info != NULL) {
	    GnomeVFSURI *uri;
	    gchar *text;

	    if (fselector->_priv->filter &&
		!gnome_vfs_directory_filter_apply (fselector->_priv->filter,
						   info)) {
		info = gnome_vfs_directory_list_next (list);
		continue;
	    }

	    uri = gnome_vfs_uri_append_file_name (async_data->uri, info->name);
	    text = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	    gtk_signal_emit_by_name (GTK_OBJECT (fselector), "add_file",
				     text, async_data->position);

	    gnome_vfs_uri_unref (uri);
	    g_free (text);

	    info = gnome_vfs_directory_list_next (list);
	}
    }

    if (result == GNOME_VFS_ERROR_EOF) {
	gchar *path;

	path = gnome_vfs_uri_to_string (async_data->uri,
					GNOME_VFS_URI_HIDE_NONE);
 	GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, do_add_directory,
				   (GNOME_SELECTOR (fselector), path,
				    async_data->position));
	g_free (path);

	free_the_async_data (fselector, async_data);

	g_message (G_STRLOC ": %p - async reading completed.", fselector);
    }
}

static void
add_directory_handler (GnomeSelector *selector, const gchar *uri, gint position)
{
    GnomeFileSelector *fselector;
    GnomeFileSelectorAsyncData *async_data;
    GnomeVFSURI *vfs_uri;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));
    g_return_if_fail (position >= -1);
    g_return_if_fail (uri != NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    g_message (G_STRLOC ": starting async reading (%s).", uri);

    vfs_uri = gnome_vfs_uri_new (uri);

    async_data = g_new0 (GnomeFileSelectorAsyncData, 1);
    async_data->type = GNOME_FILE_SELECTOR_ASYNC_TYPE_ADD_DIRECTORY;
    async_data->fselector = fselector;
    async_data->uri = vfs_uri;
    async_data->position = position;

    gnome_vfs_uri_ref (async_data->uri);
    gtk_object_ref (GTK_OBJECT (fselector));

    fselector->_priv->async_ops = g_slist_prepend
	(fselector->_priv->async_ops, async_data);

    gnome_vfs_async_load_directory_uri (&async_data->handle, vfs_uri,
					fselector->_priv->file_info_options,
					NULL, FALSE,
					GNOME_VFS_DIRECTORY_FILTER_NONE,
					GNOME_VFS_DIRECTORY_FILTER_NODIRS,
					NULL, 1, add_directory_async_cb,
					async_data);

    gnome_vfs_uri_unref (vfs_uri);
}

static void
stop_loading_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    while (fselector->_priv->async_ops != NULL) {
	GnomeFileSelectorAsyncData *async_data =
	    fselector->_priv->async_ops->data;

	g_message (G_STRLOC ": cancelling async handler %p",
		   async_data->handle);
	gnome_vfs_async_cancel (async_data->handle);

	free_the_async_data (fselector, async_data);
    }

    /* it's important to always call the parent handler of this signal
     * since the parent class may have pending async operations as well. */
    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, stop_loading, (selector));
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
    gnome_selector_add_file (selector, text, 0, FALSE);
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
    GnomeSelector *selector;
    GtkFileSelection *fs;

    selector = GNOME_SELECTOR (data);
    fs = GTK_FILE_SELECTION (selector->_priv->browse_dialog);

    if (GTK_WIDGET (fs)->window)
	gdk_window_lower (GTK_WIDGET (fs)->window);
    gtk_widget_hide (GTK_WIDGET (fs));
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
    GnomeSelector *selector;
    GtkFileSelection *fs;
    gchar *filename;

    selector = GNOME_SELECTOR (data);

    fs = GTK_FILE_SELECTION (selector->_priv->browse_dialog);
    filename = gtk_file_selection_get_filename (fs);

    /* This is "directory safe". */
    if (!gnome_selector_set_filename (selector, filename)) {
	gdk_beep ();
	return;
    }

    if (GTK_WIDGET (fs)->window)
	gdk_window_lower (GTK_WIDGET (fs)->window);
    gtk_widget_hide (GTK_WIDGET (fs));
}

static gboolean
check_filename_handler (GnomeSelector *selector, const gchar *filename)
{
    GnomeFileSelector *fselector;
    GnomeVFSResult result;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *uri;
    gboolean retval;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_FILE_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

    fselector = GNOME_FILE_SELECTOR (selector);

    g_message (G_STRLOC ": `%s'", filename);

    info = gnome_vfs_file_info_new ();
    uri = gnome_vfs_uri_new (filename);

    result = gnome_vfs_get_file_info_uri (uri, info,
					  GNOME_VFS_FILE_INFO_DEFAULT);
    if (result != GNOME_VFS_OK) {
	g_warning (G_STRLOC ": `%s': %s", filename,
		   gnome_vfs_result_to_string (result));
	gnome_vfs_file_info_unref (info);
	gnome_vfs_uri_unref (uri);
	return FALSE;
    }

    if (fselector->_priv->filter)
	retval = gnome_vfs_directory_filter_apply (fselector->_priv->filter,
						   info);
    else
	retval = info->type == GNOME_VFS_FILE_TYPE_REGULAR;

    gnome_vfs_file_info_unref (info);
    gnome_vfs_uri_unref (uri);

    return retval;
}

static gboolean
check_directory_handler (GnomeSelector *selector, const gchar *directory)
{
    GnomeVFSResult result;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *uri;
    gboolean retval;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_FILE_SELECTOR (selector), FALSE);
    g_return_val_if_fail (directory != NULL, FALSE);

    g_message (G_STRLOC ": `%s'", directory);

    info = gnome_vfs_file_info_new ();
    uri = gnome_vfs_uri_new (directory);

    result = gnome_vfs_get_file_info_uri (uri, info,
					  GNOME_VFS_FILE_INFO_DEFAULT);
    if (result != GNOME_VFS_OK) {
	g_warning (G_STRLOC ": `%s': %s", directory,
		   gnome_vfs_result_to_string (result));
	gnome_vfs_file_info_unref (info);
	gnome_vfs_uri_unref (uri);
	return FALSE;
    }

    retval = info->type == GNOME_VFS_FILE_TYPE_DIRECTORY;

    gnome_vfs_file_info_unref (info);
    gnome_vfs_uri_unref (uri);

    return retval;
}


/**
 * gnome_file_selector_construct:
 * @fselector: Pointer to GnomeFileSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeFileSelector object, for language bindings or subclassing
 * use #gnome_file_selector_new from C
 *
 * Returns: 
 */
void
gnome_file_selector_construct (GnomeFileSelector *fselector,
			       const gchar *history_id,
			       const gchar *dialog_title,
			       GtkWidget *entry_widget,
			       GtkWidget *selector_widget,
			       GtkWidget *browse_dialog,
			       guint32 flags)
{
    guint32 newflags = flags;

    g_return_if_fail (fselector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));

    /* Create the default browser dialog if requested. */
    if (flags & GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG) {
	GtkWidget *filesel_widget;
	GtkFileSelection *filesel;

	if (browse_dialog != NULL) {
	    g_warning (G_STRLOC ": It makes no sense to use "
		       "GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG "
		       "and pass a `browse_dialog' as well.");
	    return;
	}

	filesel_widget = gtk_file_selection_new (dialog_title);
	filesel = GTK_FILE_SELECTION (filesel_widget);

	gtk_signal_connect (GTK_OBJECT (filesel->cancel_button),
			    "clicked", browse_dialog_cancel,
			    fselector);
	gtk_signal_connect (GTK_OBJECT (filesel->ok_button),
			    "clicked", browse_dialog_ok,
			    fselector);

	browse_dialog = GTK_WIDGET (filesel);

	newflags &= ~GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG;
    }

    gnome_selector_construct (GNOME_SELECTOR (fselector),
			      history_id, dialog_title,
			      entry_widget, selector_widget,
			      browse_dialog, newflags);

    if (flags & GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG) {
	/* We need to unref this since it isn't put in any
	 * container so it won't get destroyed otherwise. */
	gtk_widget_unref (browse_dialog);
    }
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
			 const gchar *dialog_title,
			 guint32 flags)
{
    GnomeFileSelector *fselector;

    g_return_val_if_fail ((flags & ~GNOME_SELECTOR_USER_FLAGS) == 0, NULL);

    fselector = gtk_type_new (gnome_file_selector_get_type ());

    flags |= GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET |
	GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET |
	GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG |
	GNOME_SELECTOR_WANT_BROWSE_BUTTON;

    gnome_file_selector_construct (fselector, history_id, dialog_title,
				   NULL, NULL, NULL, flags);

    return GTK_WIDGET (fselector);
}

GtkWidget *
gnome_file_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *entry_widget,
				GtkWidget *selector_widget,
				GtkWidget *browse_dialog,
				guint32 flags)
{
    GnomeFileSelector *fselector;

    fselector = gtk_type_new (gnome_file_selector_get_type ());

    gnome_file_selector_construct (fselector, history_id,
				   dialog_title, entry_widget,
				   selector_widget, browse_dialog,
				   flags);

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
