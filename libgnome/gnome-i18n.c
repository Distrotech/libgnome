/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
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
#include <string.h>
#include <stdio.h>
#include <locale.h>

#include "gnome-i18nP.h"

/**
 * gnome_i18n_get_language_list:
 * @category_name: Name of category to look up, e.g. %"LC_MESSAGES".
 * 
 * This computes a list of language strings that the user wants.  It searches in
 * the standard environment variables to find the list, which is sorted in order
 * from most desirable to least desirable.  The `C' locale is appended to the
 * list if it does not already appear (other routines depend on this
 * behaviour). If @category_name is %NULL, then %LC_ALL is assumed.
 * 
 * Return value: the list of languages, this list should not be freed as it is
 * owned by gnome-i18n.
 **/
const GList *
gnome_i18n_get_language_list (const gchar *category_name)
{
  return bonobo_activation_i18n_get_language_list (category_name);
}

static int numeric_c_locale_depth = 0;
static char *numeric_locale = NULL;

/**
 * gnome_i18n_push_c_numeric_locale:
 *
 * Description:  Saves the current LC_NUMERIC locale and sets it to "C"
 * This way you can safely read write floating point numbers all in the
 * same format.  You should make sure that code between
 * gnome_i18n_push_c_numeric_locale() and gnome_i18n_pop_c_numeric_locale()
 * doesn't do any setlocale calls or locale may end up in a strange setting.
 * Also make sure to always pop the c numeric locale after you've pushed it.
 * The calls can be nested.
 **/
void
gnome_i18n_push_c_numeric_locale (void)
{
	if (numeric_c_locale_depth == 0) {
		g_free (numeric_locale);
		numeric_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
		setlocale (LC_NUMERIC, "C");
	}
	numeric_c_locale_depth ++;
}

/**
 * gnome_i18n_pop_c_numeric_locale:
 *
 * Description:  Restores the LC_NUMERIC locale to what it was 
 * before the matching gnome_i18n_push_c_numeric_locale(). If these calls
 * were nested, then this is a no-op until we get to the most outermost
 * layer. Code in between these should not do any setlocale calls
 * to change the LC_NUMERIC locale or things may come out very strange.
 **/
void
gnome_i18n_pop_c_numeric_locale (void)
{
	if (numeric_c_locale_depth == 0) {
		return;
	}

	numeric_c_locale_depth --;

	if (numeric_c_locale_depth == 0) {
		setlocale (LC_NUMERIC, numeric_locale);
		g_free (numeric_locale);
		numeric_locale = NULL;
	}
}
