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


GConfValue *
gnome_gconf_entry_get (GtkEntry       *entry,
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
		break;
	}

	return retval;

}

void
gnome_gconf_entry_set (GtkEntry       *entry,
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
		break;
	}
}
