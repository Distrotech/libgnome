/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - gnome-gconf.h
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#include <libgnome/gnome-defs.h>
#include <stdlib.h>
#include <liboaf/liboaf.h>

#define GCONF_ENABLE_INTERNALS 1
#include <gconf/gconf.h>
extern struct poptOption gconf_options[];

#include <libgnome/libgnome.h>
#include "oafgnome.h"
#include "gnome-gconf.h"
#include "gnome-messagebox.h"
#include "gnome-stock-ids.h"
#include <gtk/gtkmain.h>


GConfValue *
gnome_gconf_gtk_entry_get (GtkEntry       *entry,
			   GConfValueType  type)
{
	GConfValue *retval = NULL;
	const char *text;
	gint i;
	gfloat f;

	g_return_val_if_fail (entry != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_STRING) ||
			      (type == GCONF_VALUE_INT) ||
			      (type == GCONF_VALUE_FLOAT), NULL);

	text = gtk_entry_get_text (entry);
	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_STRING:
		gconf_value_set_string (retval, text);
		break;
	case GCONF_VALUE_INT:
		i = strtol (text, NULL, 0);
		gconf_value_set_int (retval, i);
		break;
	case GCONF_VALUE_FLOAT:
		f = strtod (text, NULL);
		gconf_value_set_float (retval, f);
		break;
	default:
		g_assert_not_reached ();
	}

	return retval;

}

void
gnome_gconf_gtk_entry_set (GtkEntry       *entry,
			   GConfValue     *value)
{
	gchar string[33];

	g_return_if_fail (entry != NULL);
	g_return_if_fail (GTK_IS_ENTRY (entry));
	g_return_if_fail (value != NULL);
	g_return_if_fail ((value->type == GCONF_VALUE_STRING) ||
			      (value->type == GCONF_VALUE_INT) ||
			      (value->type == GCONF_VALUE_FLOAT));

	switch (value->type) {
	case GCONF_VALUE_STRING:
		gtk_entry_set_text (entry, gconf_value_get_string (value));
		break;
	case GCONF_VALUE_INT:
		g_snprintf (string, 32, "%d", gconf_value_get_int (value));
		gtk_entry_set_text (entry, string);
		break;
	case GCONF_VALUE_FLOAT:
		g_snprintf (string, 32, "%f", gconf_value_get_float (value));
		gtk_entry_set_text (entry, string);
		break;
	default:
		g_assert_not_reached ();
	}
}

GConfValue *
gnome_gconf_spin_button_get (GtkSpinButton *spin_button,
			     GConfValueType  type)
{
	GConfValue *retval = NULL;
	gint i;
	gfloat f;

	g_return_val_if_fail (spin_button != NULL, NULL);
	g_return_val_if_fail (GTK_IS_SPIN_BUTTON (spin_button), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_INT) ||
			      (type == GCONF_VALUE_FLOAT), NULL);

	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_INT:
		i = gtk_spin_button_get_value_as_int (spin_button);
		gconf_value_set_int (retval, i);
		break;
	case GCONF_VALUE_FLOAT:
		f = gtk_spin_button_get_value_as_float (spin_button);
		gconf_value_set_float (retval, f);
		break;
	default:
		break;
	}

	return retval;
}

void
gnome_gconf_spin_button_set (GtkSpinButton *spin_button,
			     GConfValue    *value)
{
	float f;

	g_return_if_fail (spin_button != NULL);
	g_return_if_fail (GTK_IS_SPIN_BUTTON (spin_button));
	g_return_if_fail (value != NULL);
	g_return_if_fail ((value->type == GCONF_VALUE_INT) ||
			  (value->type == GCONF_VALUE_FLOAT));

	switch (value->type) {
	case GCONF_VALUE_INT:
		f = gconf_value_get_int (value);
		gtk_spin_button_set_value (spin_button, f);
		break;
	case GCONF_VALUE_FLOAT:
		f = gconf_value_get_float (value);
		gtk_spin_button_set_value (spin_button, f);
		break;
	default:
		break;
	}
}

GConfValue *
gnome_gconf_gtk_radio_button_get (GtkRadioButton  *radio,
				  GConfValueType   type)
{
	GConfValue *retval;
	int i = 0;
	GSList *group;
	
	g_return_val_if_fail (radio != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RADIO_BUTTON (radio), NULL);
	g_return_val_if_fail (((type == GCONF_VALUE_BOOL) ||
			       (type == GCONF_VALUE_INT)), NULL);

	retval = gconf_value_new (type);
	for (i = 0, group = radio->group; group != NULL; i++, group = group->next) {
		if (GTK_TOGGLE_BUTTON (group->data)->active) {
			if (type == GCONF_VALUE_BOOL) {
				if (i > 1)
					g_warning ("more then two radio buttons used with a boolean\n");
				else
					gconf_value_set_bool (retval, i);
			} else {
				gconf_value_set_int (retval, i);
			}
			break;
		}
	}
	return retval;
}

void
gnome_gconf_gtk_radio_button_set (GtkRadioButton  *radio,
				  GConfValue      *value)
{
	gint i, j = 0;
	GSList *list;

	g_return_if_fail (radio != NULL);
	g_return_if_fail (GTK_IS_RADIO_BUTTON (radio));
	g_return_if_fail (value != NULL);
	g_return_if_fail ((value->type == GCONF_VALUE_BOOL) ||
			  (value->type == GCONF_VALUE_INT));

	switch (value->type) {
	case GCONF_VALUE_BOOL:
		j = gconf_value_get_bool (value);
		break;
	case GCONF_VALUE_INT:
		j = gconf_value_get_int (value);
		break;
	default:
		g_assert_not_reached();
	}

	for (list = radio->group, i = 0 ; i != j && list != NULL; i ++, list = list->next)
		;
	if (list)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (list->data), TRUE);
}

GConfValue *
gnome_gconf_gtk_range_get (GtkRange       *range,
			   GConfValueType  type)
{
	GtkAdjustment *adjustment;
	GConfValue *retval;
	int i;

	g_return_val_if_fail (range != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RANGE (range), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_FLOAT) ||
			      (type == GCONF_VALUE_INT), NULL);

	adjustment = gtk_range_get_adjustment (range);
	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_INT:
		i = adjustment->value;
		gconf_value_set_int (retval, i);
		break;
	case GCONF_VALUE_FLOAT:
		gconf_value_set_float (retval, adjustment->value);
		break;
	default:
		break;
	}
	return retval;
}

void
gnome_gconf_gtk_range_set (GtkRange       *range,
			   GConfValue     *value)
{
	GtkAdjustment *adjustment;
	gdouble f = 0.0;

	g_return_if_fail (range != NULL);
	g_return_if_fail (GTK_IS_RANGE (range));
	g_return_if_fail ((value->type == GCONF_VALUE_FLOAT) ||
			  (value->type == GCONF_VALUE_INT));

	switch (value->type) {
	case GCONF_VALUE_FLOAT:
		f = gconf_value_get_float (value);
		break;
	case GCONF_VALUE_INT:
		f = gconf_value_get_int (value);
		break;
	default:
		g_assert_not_reached();
	}
	adjustment = gtk_range_get_adjustment (range);
	gtk_adjustment_set_value (adjustment, f);
}

GConfValue *
gnome_gconf_gtk_toggle_button_get (GtkToggleButton *toggle,
				   GConfValueType   type)
{
	GConfValue *retval;

	g_return_val_if_fail (toggle != NULL, NULL);
	g_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (toggle), NULL);
	g_return_val_if_fail (((type == GCONF_VALUE_BOOL)||
			       (type == GCONF_VALUE_INT)), NULL);

	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_BOOL:
		gconf_value_set_bool (retval, toggle->active);
		break;
	case GCONF_VALUE_INT:
		gconf_value_set_int (retval, toggle->active);
		break;
	default:
		g_assert_not_reached ();
	}
	return retval;
}

void
gnome_gconf_gtk_toggle_button_set (GtkToggleButton *toggle,
				   GConfValue      *value)
{
	g_return_if_fail (toggle != NULL);
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	g_return_if_fail (value != NULL);
	g_return_if_fail ((value->type == GCONF_VALUE_BOOL)||
			  (value->type == GCONF_VALUE_INT));

	switch (value->type) {
	case GCONF_VALUE_BOOL:
		gtk_toggle_button_set_active (toggle, gconf_value_get_bool (value));
		break;
	case GCONF_VALUE_INT:
		gtk_toggle_button_set_active (toggle, gconf_value_get_int (value));
		break;
	default:
		g_assert_not_reached ();
	}
}

GConfValue *
gnome_gconf_gnome_color_picker_get (GnomeColorPicker *picker,
				    GConfValueType    type)
{
	GConfValue *retval;
	gushort r, g, b, a;
	gchar color[18];

	g_return_val_if_fail (picker != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_COLOR_PICKER (picker), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_STRING), NULL);

	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_STRING:
		gnome_color_picker_get_i16 (picker, &r, &g, &b, &a);
		g_snprintf (color, 18, "#%04X%04X%04X%04X", r, g, b, a);
		gconf_value_set_string (retval, color);
		break;
	default:
		g_assert_not_reached ();
	}
	return retval;
}

void
gnome_gconf_gnome_color_picker_set (GnomeColorPicker *picker,
				    GConfValue       *value)
{
	gushort r, g, b, a;
	gchar *color;

	g_return_if_fail (picker != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (picker));
	g_return_if_fail (value != NULL);
	g_return_if_fail (value->type == GCONF_VALUE_STRING);

	color = g_strndup (gconf_value_get_string (value), 17);
	a = strtol (color  + 13, (char **)NULL, 16);
	*(color +13) = '\0';
	b = strtol (color  + 9, (char **)NULL, 16);
	*(color +9) = '\0';
	g = strtol (color  + 5, (char **)NULL, 16);
	*(color +5) = '\0';
	r = strtol (color  + 1, (char **)NULL, 16);
	gnome_color_picker_set_i16 (picker, r, g, b, a);
	g_free (color);
}

gchar*
gnome_gconf_get_gnome_libs_settings_relative (const gchar *subkey)
{
        gchar *dir;
        gchar *key;

        dir = g_strconcat("/apps/gnome-settings/",
                          gnome_program_get_name(gnome_program_get()),
                          NULL);

        if (subkey && *subkey) {
                key = gconf_concat_dir_and_key(dir, subkey);
                g_free(dir);
        } else {
                /* subkey == "" */
                key = dir;
        }

        return key;
}

gchar*
gnome_gconf_get_app_settings_relative (const gchar *subkey)
{
        gchar *dir;
        gchar *key;

        dir = g_strconcat("/apps/",
                          gnome_program_get_name(gnome_program_get()),
                          NULL);

        if (subkey && *subkey) {
                key = gconf_concat_dir_and_key(dir, subkey);
                g_free(dir);
        } else {
                /* subkey == "" */
                key = dir;
        }

        return key;
}

/*
 * Our global GConfClient, and module stuff
 */
static void gnome_default_gconf_client_error_handler (GConfClient                  *client,
                                                      GError                       *error);


static GConfClient* global_client = NULL;

GConfClient*
gnome_get_gconf_client (void)
{
        g_return_val_if_fail(global_client != NULL, NULL);
        
        return global_client;
}

static void
gnome_gconf_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gconf_preinit(app, (GnomeModuleInfo*)mod_info);

        gconf_client_set_global_default_error_handler(gnome_default_gconf_client_error_handler);
}

static void
gnome_gconf_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gchar *settings_dir;
        
        gconf_postinit(app, (GnomeModuleInfo*)mod_info);

        global_client = gconf_client_get_default();

        gconf_client_add_dir(global_client,
                             "/desktop/gnome",
                             GCONF_CLIENT_PRELOAD_NONE, NULL);

        settings_dir = gnome_gconf_get_gnome_libs_settings_relative("");

        gconf_client_add_dir(global_client,
                             settings_dir,
                             /* Possibly we should turn preload on for this */
                             GCONF_CLIENT_PRELOAD_NONE,
                             NULL);
        g_free(settings_dir);
}

extern GnomeModuleInfo gtk_module_info;

static GnomeModuleRequirement gnome_gconf_requirements[] = {
        { "1.2.5", &gtk_module_info },
        /* VERSION is also our version note - it's all libgnomeui */
        { VERSION, &liboafgnome_module_info },
        { NULL, NULL }
};

GnomeModuleInfo gnome_gconf_module_info = {
        "gnome-gconf", VERSION, N_("GNOME GConf Support"),
        gnome_gconf_requirements,
        gnome_gconf_pre_args_parse, gnome_gconf_post_args_parse,
        gconf_options
};


typedef struct {
        GConfClient                  *client;
} ErrorIdleData;

static guint error_handler_idle = 0;
static GSList *pending_errors = NULL;
static ErrorIdleData eid = { NULL };

static gint
error_idle_func(gpointer data)
{
        GtkWidget *dialog;
        GSList *iter;
        gboolean have_overridden = FALSE;
        gchar* mesg = NULL;
        const gchar* fmt = NULL;
        
        error_handler_idle = 0;

        g_return_val_if_fail(eid.client != NULL, FALSE);
        g_return_val_if_fail(pending_errors != NULL, FALSE);
        
        iter = pending_errors;
        while (iter != NULL) {
                GError *error = iter->data;

                if (g_error_matches (error, GCONF_ERROR, GCONF_ERROR_OVERRIDDEN))
                        have_overridden = TRUE;
                
                iter = g_slist_next(iter);
        }
        
        if (have_overridden) {
                fmt = _("You attempted to change an aspect of your configuration that your system administrator or operating system vendor does not allow you to change. Some of the settings you have selected may not take effect, or may not be restored next time you use this application (%s).");
                
        } else {
                fmt = _("An error occurred while loading or saving configuration information for %s. Some of your configuration settings may not work properly.");
        }

        mesg = g_strdup_printf(fmt, gnome_program_get_human_readable_name(gnome_program_get()));
        
        dialog = gnome_message_box_new(mesg,
                                       GNOME_MESSAGE_BOX_ERROR,
                                       GNOME_STOCK_BUTTON_OK,
                                       NULL);

        g_free(mesg);
        
        gtk_widget_show_all(dialog);


        /* FIXME put this in a "Technical Details" optional part of the dialog
           that can be opened up if users are interested */
        iter = pending_errors;
        while (iter != NULL) {
                GError *error = iter->data;
                iter->data = NULL;

                fprintf(stderr, _("GConf error details: %s\n"), error->message);

                g_error_free(error);
                
                iter = g_slist_next(iter);
        }

        g_slist_free(pending_errors);

        pending_errors = NULL;
        
        g_object_unref(G_OBJECT(eid.client));
        eid.client = NULL;

        return FALSE;
}

static void
gnome_default_gconf_client_error_handler (GConfClient                  *client,
                                          GError                       *error)
{
        g_object_ref(G_OBJECT(client));
        
        if (eid.client) {
                g_object_unref(G_OBJECT(eid.client));
        }
        
        eid.client = client;
        
        pending_errors = g_slist_append(pending_errors, g_error_copy(error));

        if (error_handler_idle == 0) {
                error_handler_idle = gtk_idle_add(error_idle_func, NULL);
        }
}


