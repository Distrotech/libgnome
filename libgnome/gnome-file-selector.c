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
#include <gtk/gtkentry.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-file-selector.h"
#include "gnome-selectorP.h"
#include "gnome-entry.h"

#include <libgnomevfs/gnome-vfs.h>

struct _GnomeFileSelectorPrivate {
    GtkWidget *combo;
    GtkWidget *entry;
};
	

static void gnome_file_selector_class_init  (GnomeFileSelectorClass *class);
static void gnome_file_selector_init        (GnomeFileSelector      *fselector);
static void gnome_file_selector_destroy         (GtkObject       *object);
static void gnome_file_selector_finalize        (GObject         *object);

static gchar *   get_entry_text_handler         (GnomeSelector   *selector);
static void      set_entry_text_handler         (GnomeSelector   *selector,
                                                 const gchar     *text);
static void      activate_entry_handler         (GnomeSelector   *selector);
static void      history_changed_handler        (GnomeSelector   *selector);

static gboolean  check_filename_handler         (GnomeSelector   *selector,
                                                 const gchar     *filename);
static gboolean  check_directory_handler        (GnomeSelector   *selector,
                                                 const gchar     *directory);
static void      update_file_list_handler       (GnomeSelector   *selector);


static GnomeSelectorClass *parent_class;

guint
gnome_file_selector_get_type (void)
{
    static guint fselector_type = 0;

    if (!fselector_type) {
	GtkTypeInfo fselector_info = {
	    "GnomeFileSelector",
	    sizeof (GnomeFileSelector),
	    sizeof (GnomeFileSelectorClass),
	    (GtkClassInitFunc) gnome_file_selector_class_init,
	    (GtkObjectInitFunc) gnome_file_selector_init,
	    NULL,
	    NULL,
	    NULL
	};

	fselector_type = gtk_type_unique 
	    (gnome_selector_get_type (), &fselector_info);
    }

    return fselector_type;
}

static void
gnome_file_selector_class_init (GnomeFileSelectorClass *class)
{
    GnomeSelectorClass *selector_class;
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;

    selector_class = (GnomeSelectorClass *) class;
    object_class = (GtkObjectClass *) class;
    gobject_class = (GObjectClass *) class;

    parent_class = gtk_type_class (gnome_selector_get_type ());

    object_class->destroy = gnome_file_selector_destroy;
    gobject_class->finalize = gnome_file_selector_finalize;

    selector_class->get_entry_text = get_entry_text_handler;
    selector_class->set_entry_text = set_entry_text_handler;
    selector_class->activate_entry = activate_entry_handler;
    selector_class->history_changed = history_changed_handler;

    selector_class->check_filename = check_filename_handler;
    selector_class->check_directory = check_directory_handler;
    selector_class->update_file_list = update_file_list_handler;
}

static gchar *
get_entry_text_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;
    gchar *text;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_FILE_SELECTOR (selector), NULL);

    fselector = GNOME_FILE_SELECTOR (selector);

    text = gtk_entry_get_text (GTK_ENTRY (fselector->_priv->entry));
    return g_strdup (text);
}

static void
set_entry_text_handler (GnomeSelector *selector, const gchar *text)
{
    GnomeFileSelector *fselector;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    gtk_entry_set_text (GTK_ENTRY (fselector->_priv->entry), text);
}

static void
activate_entry_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;
    gchar *text;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    text = gtk_entry_get_text (GTK_ENTRY (fselector->_priv->entry));
    gnome_selector_prepend_history (selector, TRUE, text);

    gtk_signal_emit_by_name (GTK_OBJECT (fselector->_priv->entry),
			     "activate");
}

static void
history_changed_handler (GnomeSelector *selector)
{
    GnomeFileSelector *fselector;
    GtkWidget *list_widget;
    GList *items = NULL;
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    fselector = GNOME_FILE_SELECTOR (selector);

    g_message (G_STRLOC);

    list_widget = GTK_COMBO (fselector->_priv->combo)->list;

    gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);

    for (c = selector->_priv->history; c; c = c->next) {
	GnomeSelectorHistoryItem *hitem = c->data;
	GtkWidget *item;

	g_print ("HISTORY: `%s'\n", hitem->text);

	item = gtk_list_item_new_with_label (hitem->text);
	items = g_list_prepend (items, item);
	gtk_widget_show_all (item);
    }

    items = g_list_reverse (items);

    gtk_list_prepend_items (GTK_LIST (list_widget), items);
}

static void
entry_activated_cb (GtkWidget *widget, gpointer data)
{
    GnomeSelector *selector;
    GnomeVFSResult result;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *uri;

    gchar *text = NULL;

    selector = GNOME_SELECTOR (data);
    text = gnome_selector_get_entry_text (selector);

    g_message (G_STRLOC ": '%s'", text);

    info = gnome_vfs_file_info_new ();
    uri = gnome_vfs_uri_new (text);

    result = gnome_vfs_get_file_info_uri (uri, info,
					  GNOME_VFS_FILE_INFO_DEFAULT);
    if (result != GNOME_VFS_OK) {
	g_warning (G_STRLOC ": `%s': %s", text,
		   gnome_vfs_result_to_string (result));
	gnome_vfs_file_info_unref (info);
	gnome_vfs_uri_unref (uri);
	g_free (text);
	return;
    }

    switch (info->type) {
    case GNOME_VFS_FILE_TYPE_REGULAR:
	g_message (G_STRLOC ": appending `%s' as file", text);
	gnome_selector_append_file (selector, text, FALSE);
	break;
    case GNOME_VFS_FILE_TYPE_DIRECTORY:
	gnome_selector_append_directory (selector, text, FALSE);
	break;
    default:
	g_warning (G_STRLOC ": URI `%s' has invalid type %d", text,
		   info->type);
	break;
    }
    
    gnome_vfs_file_info_unref (info);
    gnome_vfs_uri_unref (uri);

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
    GnomeVFSResult result;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *uri;
    gboolean retval;

    g_return_val_if_fail (selector != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_FILE_SELECTOR (selector), FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);

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

static void
_update_file_list_handler_file (GnomeSelector *selector, const gchar *uri,
				GnomeVFSDirectoryFilter *filter,
				gboolean defaultp)
{
    GnomeVFSResult result;
    GnomeVFSDirectoryHandle *handle = NULL;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *vfs_uri;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    vfs_uri = gnome_vfs_uri_new (uri);

    info = gnome_vfs_file_info_new ();
    result = gnome_vfs_get_file_info_uri (vfs_uri, info,
					  GNOME_VFS_FILE_INFO_DEFAULT);

    if (result != GNOME_VFS_OK) {
	g_warning (G_STRLOC ": `%s': %s", uri,
		   gnome_vfs_result_to_string (result));
	gnome_vfs_file_info_unref (info);
	gnome_vfs_uri_unref (vfs_uri);
	return;
    }

    if (gnome_vfs_directory_filter_apply (filter, info))
	gtk_signal_emit_by_name (GTK_OBJECT (selector),
				 defaultp ? "add_file_default" : "add_file",
				 uri);

    gnome_vfs_file_info_unref (info);
    gnome_vfs_uri_unref (vfs_uri);
}

static void
_update_file_list_handler (GnomeSelector *selector, const gchar *uri,
			   GnomeVFSDirectoryFilter *filter, gboolean defaultp)
{
    GnomeVFSResult result;
    GnomeVFSDirectoryHandle *handle = NULL;
    GnomeVFSFileInfo *info;
    GnomeVFSURI *vfs_uri;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    vfs_uri = gnome_vfs_uri_new (uri);

    result = gnome_vfs_directory_open_from_uri (&handle, vfs_uri,
						GNOME_VFS_FILE_INFO_DEFAULT,
						filter);

    if (result != GNOME_VFS_OK) {
	g_warning (G_STRLOC ": `%s': %s", uri,
		   gnome_vfs_result_to_string (result));
	gnome_vfs_uri_unref (vfs_uri);
	return;
    }

    info = gnome_vfs_file_info_new ();

    for (;;) {
	GnomeVFSURI *file_uri;
	gchar *pathname;

	result = gnome_vfs_directory_read_next (handle, info);

	if (result == GNOME_VFS_ERROR_EOF)
	    break;
	else if (result != GNOME_VFS_OK) {
	    g_warning (G_STRLOC ": `%s': %s", uri,
		       gnome_vfs_result_to_string (result));

	    gnome_vfs_file_info_unref (info);
	    gnome_vfs_uri_unref (vfs_uri);
	    return;
	}

	file_uri = gnome_vfs_uri_append_file_name (vfs_uri, info->name);
	pathname = gnome_vfs_uri_to_string (file_uri, GNOME_VFS_URI_HIDE_NONE);

	gtk_signal_emit_by_name (GTK_OBJECT (selector),
				 defaultp ? "add_file_default" : "add_file",
				 pathname);

	gnome_vfs_uri_unref (file_uri);
	g_free (pathname);
    }

    gnome_vfs_file_info_unref (info);
    gnome_vfs_uri_unref (vfs_uri);
}

static void
update_file_list_handler (GnomeSelector *selector)
{
    GnomeVFSDirectoryFilter *filter;
    GSList *c;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (selector));

    g_message (G_STRLOC);

#if 0
    if (GNOME_SELECTOR_CLASS (parent_class)->update_file_list)
	(* GNOME_SELECTOR_CLASS (parent_class)->update_file_list) (selector);
#endif

    filter = gnome_vfs_directory_filter_new
	(GNOME_VFS_DIRECTORY_FILTER_NONE,
	 GNOME_VFS_DIRECTORY_FILTER_NODIRS |
	 GNOME_VFS_DIRECTORY_FILTER_NODOTFILES |
	 GNOME_VFS_DIRECTORY_FILTER_NOSELFDIR |
	 GNOME_VFS_DIRECTORY_FILTER_NOPARENTDIR,
	 NULL);

    g_slist_foreach (selector->_priv->total_list, (GFunc) g_free, NULL);
    g_slist_free (selector->_priv->total_list);
    selector->_priv->total_list = NULL;

    g_slist_foreach (selector->_priv->default_total_list,
		     (GFunc) g_free, NULL);
    g_slist_free (selector->_priv->default_total_list);
    selector->_priv->default_total_list = NULL;

    for (c = selector->_priv->default_dir_list; c; c = c->next)
	_update_file_list_handler (selector, c->data, filter, TRUE);

    for (c = selector->_priv->dir_list; c; c = c->next)
	_update_file_list_handler (selector, c->data, filter, FALSE);

    for (c = selector->_priv->default_file_list; c; c = c->next)
	_update_file_list_handler_file (selector, c->data, filter, TRUE);

    for (c = selector->_priv->file_list; c; c = c->next)
	_update_file_list_handler_file (selector, c->data, filter, FALSE);

    selector->_priv->default_total_list =
	g_slist_reverse (selector->_priv->default_total_list);
    selector->_priv->total_list =
	g_slist_reverse (selector->_priv->total_list);

    gnome_vfs_directory_filter_destroy (filter);
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
    g_return_if_fail (fselector != NULL);
    g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));

    /* Create the default entry widget if requested. */
    if (flags & GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET) {
	if (entry_widget != NULL) {
	    g_warning (G_STRLOC ": It makes no sense to use "
		       "GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET "
		       "and pass a `entry_widget' as well.");
	    return;
	}

	entry_widget = gtk_combo_new ();

	gtk_combo_disable_activate (GTK_COMBO (entry_widget));
	gtk_combo_set_case_sensitive (GTK_COMBO (entry_widget), TRUE);

	fselector->_priv->combo = entry_widget;
	fselector->_priv->entry = GTK_COMBO (entry_widget)->entry;

	gtk_signal_connect (GTK_OBJECT (fselector->_priv->entry),
			    "activate", entry_activated_cb, fselector);
    }

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
    }

    gnome_selector_construct (GNOME_SELECTOR (fselector),
			      history_id, dialog_title,
			      entry_widget, selector_widget,
			      browse_dialog, flags);

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

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
	(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
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

    if (G_OBJECT_CLASS (parent_class)->finalize)
	(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}
