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

#ifndef GNOME_FILE_SELECTOR_H
#define GNOME_FILE_SELECTOR_H


#include <gtk/gtkvbox.h>

#include "gnome-entry.h"

#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-directory-filter.h>


G_BEGIN_DECLS


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
        GnomeEntry entry;
        
        /*< private >*/
        GnomeFileSelectorPrivate *_priv;
};

struct _GnomeFileSelectorClass {
        GnomeEntryClass parent_class;
};


guint      gnome_file_selector_get_type      (void) G_GNUC_CONST;

GtkWidget *gnome_file_selector_new           (const gchar *history_id,
                                              const gchar *dialog_title);

void       gnome_file_selector_set_filter    (GnomeFileSelector *fselector,
                                              GnomeVFSDirectoryFilter *filter);

void       gnome_file_selector_clear_filter  (GnomeFileSelector *fselector);

G_END_DECLS

#endif
