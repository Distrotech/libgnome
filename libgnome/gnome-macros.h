/* gnome-macros.h
 *   Macros for making GTK+ objects to avoid typos and reduce code size
 * Copyright (C) 2000  Eazel, Inc.
 *
 * Authors: George Lebl <jirka@5z.com>
 *
 * All rights reserved.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#ifndef GNOME_MACROS_H
#define GNOME_MACROS_H

/* Macros for defining classes.  Ideas taken from Nautilus and GOB. */

/* Define the boilerplate type stuff to reduce typos and code size.  Defines
 * the get_type method and the parent_class static variable. */
#define GNOME_CLASS_BOILERPLATE(type, type_as_function,			\
				parent_type, parent_type_as_function)	\
static parent_type *parent_class = NULL;				\
GtkType									\
type_as_function ## _get_type (void)					\
{									\
	static GtkType object_type = 0;					\
	if (object_type == 0) {						\
		GtkType type_of_parent;					\
		static const GtkTypeInfo object_info = {		\
			#type,						\
			sizeof (type),					\
			sizeof (type ## Class),				\
			(GtkClassInitFunc) type_as_function ## _class_init, \
			(GtkObjectInitFunc) type_as_function ## _init,	\
			/* reserved_1 */ NULL,				\
			/* reserved_2 */ NULL,				\
			(GtkClassInitFunc) NULL				\
		};							\
		type_of_parent = parent_type_as_function ## _get_type (); \
		object_type = gtk_type_unique (type_of_parent, &object_info); \
		parent_class = gtk_type_class (type_of_parent);		\
	}								\
	return object_type;						\
}

/* Just call the parent handler.  This assumes that there is a variable
 * named parent_class that points to the (duh!) parent class */
#define GNOME_CALL_PARENT_HANDLER(parent_class_cast, name, args)	\
	((parent_class_cast(parent_class)->name != NULL) ?		\
	 parent_class_cast(parent_class)->name args : 0)

/* Same as above, but in case there is no implementation, it evaluates
 * to def_return */
#define GNOME_CALL_PARENT_HANDLER_WITH_DEFAULT(parent_class_cast,	\
					       name, args, def_return)	\
	((parent_class_cast(parent_class)->name != NULL) ?		\
	 parent_class_cast(parent_class)->name args : def_return)

#endif /* GNOME_MACROS_H */
