 /* GNOME GUI Library - gnome-gconf.h
 * Copyright (C) 2000  Red Hat Inc.,
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

#include "gnome-gconf.h"
#include <stdlib.h>

GConfValue *
gnome_gconf_gtk_entry_get (GtkEntry       *entry,
			   GConfValueType  type)
{
	GConfValue *retval = NULL;
	gchar *text;
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
	g_return_if_fail ((value->type == GCONF_VALUE_STRING) ||
			      (value->type == GCONF_VALUE_INT) ||
			      (value->type == GCONF_VALUE_FLOAT));

	switch (value->type) {
	case GCONF_VALUE_STRING:
		gtk_entry_set_text (entry, gconf_value_string (value));
		break;
	case GCONF_VALUE_INT:
		g_snprintf (string, 32, "%d", gconf_value_int (value));
		gtk_entry_set_text (entry, string);
		break;
	case GCONF_VALUE_FLOAT:
		g_snprintf (string, 32, "%f", gconf_value_float (value));
		gtk_entry_set_text (entry, string);
		break;
	default:
		g_assert_not_reached ();
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
	g_return_if_fail ((value->type == GCONF_VALUE_BOOL) ||
			  (value->type == GCONF_VALUE_INT));

	switch (value->type) {
	case GCONF_VALUE_BOOL:
		j = gconf_value_bool (value);
		break;
	case GCONF_VALUE_INT:
		j = gconf_value_int (value);
		break;
	default:
		g_assert_not_reached();
	}

	for (list = radio->group, i = 0 ; i != j || list != NULL; i ++, list = list->next)
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

	g_return_val_if_fail (range != NULL, NULL);
	g_return_val_if_fail (GTK_IS_RANGE (range), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_FLOAT), NULL);

	adjustment = gtk_range_get_adjustment (range);
	retval = gconf_value_new (type);
	gconf_value_set_float (retval, adjustment->value);
	return retval;
}

void
gnome_gconf_gtk_range_set (GtkRange       *range,
			   GConfValue     *value)
{
	GtkAdjustment *adjustment;

	g_return_if_fail (range != NULL);
	g_return_if_fail (GTK_IS_RANGE (range));
	g_return_if_fail ((value->type == GCONF_VALUE_FLOAT));

	adjustment = gtk_range_get_adjustment (range);
	gtk_adjustment_set_value (adjustment, gconf_value_float (value));
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
	g_return_if_fail ((value->type == GCONF_VALUE_BOOL)||
			  (value->type == GCONF_VALUE_INT));

	switch (value->type) {
	case GCONF_VALUE_BOOL:
		gtk_toggle_button_set_active (toggle, gconf_value_bool (value));
		break;
	case GCONF_VALUE_INT:
		gtk_toggle_button_set_active (toggle, gconf_value_int (value));
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

	g_return_val_if_fail (picker != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_COLOR_PICKER (picker), NULL);
	g_return_val_if_fail ((type == GCONF_VALUE_STRING), NULL);

	retval = gconf_value_new (type);
	switch (type) {
	case GCONF_VALUE_STRING:
		
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
	
}
