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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gobject/gvaluetypes.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-selector-dialog.h>

#define PARENT_TYPE BONOBO_OBJECT_TYPE
BonoboObjectClass *gnome_selector_dialog_parent_class = NULL;

struct _GnomeSelectorDialogPrivate {
    GNOME_Selector selector;
};


static void gnome_selector_dialog_class_init  (GnomeSelectorDialogClass *class);
static void gnome_selector_dialog_init        (GnomeSelectorDialog      *dialog);
static void gnome_selector_dialog_finalize    (GObject                  *object);

static void
gnome_selector_dialog_class_init (GnomeSelectorDialogClass *class)
{
    GObjectClass *object_class;

    gnome_selector_dialog_parent_class = g_type_class_peek_parent (class);

    object_class = (GObjectClass *) class;

    object_class->finalize = gnome_selector_dialog_finalize;
}

static void
gnome_selector_dialog_init (GnomeSelectorDialog *dialog)
{
    dialog->_priv = g_new0 (GnomeSelectorDialogPrivate, 1);
}

static void
gnome_selector_dialog_finalize (GObject *object)
{
    GnomeSelectorDialog *dialog;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_SELECTOR (object));

    dialog = GNOME_SELECTOR_DIALOG (object);

    g_free (dialog->_priv);
    dialog->_priv = NULL;

    G_OBJECT_CLASS (gnome_selector_dialog_parent_class)->finalize (object);
}

BONOBO_TYPE_FUNC_FULL (GnomeSelectorDialog, GNOME_SelectorDialog,
		       PARENT_TYPE, gnome_selector_dialog);
