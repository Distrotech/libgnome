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

#ifndef GNOME_FILE_SELECTOR_H
#define GNOME_FILE_SELECTOR_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>
#include "gnome-selector.h"


BEGIN_GNOME_DECLS


#define GNOME_TYPE_FILE_SELECTOR            (gnome_file_selector_get_type ())
#define GNOME_FILE_SELECTOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_FILE_SELECTOR, GnomeFileSelector))
#define GNOME_FILE_SELECTOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_FILE_SELECTOR, GnomeFileSelectorClass))
#define GNOME_IS_FILE_SELECTOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_FILE_SELECTOR))
#define GNOME_IS_FILE_SELECTOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_FILE_SELECTOR))
#define GNOME_FILE_SELECTOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_FILE_SELECTOR, GnomeFileSelectorClass))


typedef struct _GnomeFileSelector         GnomeFileSelector;
typedef struct _GnomeFileSelectorPrivate  GnomeFileSelectorPrivate;
typedef struct _GnomeFileSelectorClass    GnomeFileSelectorClass;

struct _GnomeFileSelector {
        GnomeSelector selector;
        
        /*< private >*/
        GnomeFileSelectorPrivate *_priv;
};

struct _GnomeFileSelectorClass {
        GnomeSelectorClass parent_class;
};


guint        gnome_file_selector_get_type    (void);

GtkWidget   *gnome_file_selector_new         (const gchar *history_id,
                                              const gchar *dialog_title);

GtkWidget   *gnome_file_selector_new_custom  (const gchar *history_id,
                                              const gchar *dialog_title,
                                              GtkWidget *entry_widget,
                                              GtkWidget *selector_widget,
                                              GtkWidget *browse_dialog,
                                              guint32 flags);

void         gnome_file_selector_construct   (GnomeFileSelector *selector,
                                              const gchar *history_id,
                                              const gchar *dialog_title,
                                              GtkWidget *entry_widget,
                                              GtkWidget *selector_widget,
                                              GtkWidget *browse_dialog,
                                              guint32 flags);


END_GNOME_DECLS

#endif
