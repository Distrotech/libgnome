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
#include "gnome-entry.h"

struct _GnomeFileSelectorPrivate {
};
	

static void gnome_file_selector_class_init (GnomeFileSelectorClass *class);
static void gnome_file_selector_init       (GnomeFileSelector      *fselector);
static void gnome_file_selector_destroy    (GtkObject       *object);
static void gnome_file_selector_finalize   (GObject         *object);

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
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_get_type ());

	object_class->destroy = gnome_file_selector_destroy;
	gobject_class->finalize = gnome_file_selector_finalize;
}

static void
gnome_file_selector_init (GnomeFileSelector *fselector)
{
	fselector->_priv = g_new0 (GnomeFileSelectorPrivate, 1);
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
			       GtkWidget *selector_widget,
			       GtkWidget *browse_dialog)
{
	g_return_if_fail (fselector != NULL);
	g_return_if_fail (GNOME_IS_FILE_SELECTOR (fselector));

	gnome_selector_construct (GNOME_SELECTOR (fselector),
				  history_id, dialog_title,
				  selector_widget, browse_dialog);
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
	GtkWidget *selector;

	fselector = gtk_type_new (gnome_file_selector_get_type ());

	selector = gtk_file_selection_new (dialog_title);

	gnome_file_selector_construct (fselector, history_id,
				       dialog_title, NULL, selector);

	return GTK_WIDGET (fselector);
}

GtkWidget *
gnome_file_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *selector_widget,
				GtkWidget *browse_dialog)
{
	GnomeFileSelector *fselector;

	fselector = gtk_type_new (gnome_file_selector_get_type ());

	gnome_file_selector_construct (fselector, history_id,
				       dialog_title, selector_widget,
				       browse_dialog);

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

