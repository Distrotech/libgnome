/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2001 SuSE Linux AG
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

#ifndef GNOME_DIRECTORY_FILTER_H
#define GNOME_DIRECTORY_FILTER_H


#include <libgnome/gnome-async-context.h>


G_BEGIN_DECLS


#define GNOME_TYPE_DIRECTORY_FILTER            (gnome_directory_filter_get_type ())
#define GNOME_DIRECTORY_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_DIRECTORY_FILTER, GnomeDirectoryFilter))
#define GNOME_DIRECTORY_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DIRECTORY_FILTER, GnomeDirectoryFilterClass))
#define GNOME_IS_DIRECTORY_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_DIRECTORY_FILTER))
#define GNOME_IS_DIRECTORY_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DIRECTORY_FILTER))
#define GNOME_DIRECTORY_FILTER_GET_CLASS(obj)  (GNOME_DIRECTORY_FILTER_CLASS (G_OBJECT_GET_CLASS (obj)))


typedef struct _GnomeDirectoryFilter           GnomeDirectoryFilter;
typedef struct _GnomeDirectoryFilterPrivate    GnomeDirectoryFilterPrivate;
typedef struct _GnomeDirectoryFilterClass      GnomeDirectoryFilterClass;

struct _GnomeDirectoryFilter {
    BonoboObject object;
        
    /*< private >*/
    GnomeDirectoryFilterPrivate *_priv;
};

struct _GnomeDirectoryFilterClass {
    BonoboObjectClass parent_class;

    POA_GNOME_DirectoryFilter__epv epv;

    void     (*check_uri)             (GnomeDirectoryFilter     *filter,
                                       const gchar              *uri,
                                       GnomeAsyncHandle         *async_handle);

    void     (*scan_directory)        (GnomeDirectoryFilter     *filter,
                                       const gchar              *directory,
                                       GnomeAsyncHandle         *async_handle);
};


GType                  gnome_directory_filter_get_type   (void) G_GNUC_CONST;

GnomeDirectoryFilter  *gnome_directory_filter_new        (void);

G_END_DECLS

#endif
