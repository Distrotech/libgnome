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
#include <locale.h>	/* setlocale() */

#include "libgnomeP.h"

/* Name of config key we use when looking up preferred language. */
#define LANGKEY "/Gnome/i18n/LANG"

/**
 * gnome_i18n_get_language:
 * 
 * Returns current language (contents of "LANG" environment variable).
 * 
 * Return value: 
 **/
const char *
gnome_i18n_get_language(void)
{
  return g_getenv("LANG");
}

/**
 * gnome_i18n_get_language_list:
 * @category_name: Name of category to look up, e.g. "LC_MESSAGES".
 * 
 * This computes a list of language strings.  It searches in the
 * standard environment variables to find the list, which is sorted
 * in order from most desirable to least desirable.  The `C' locale
 * is appended to the list if it does not already appear (other routines depend on this behaviour).
 * If @category_name is %NULL, then LC_ALL is assumed.
 * 
 * Return value: the list of languages, this list should not be freed as it is owned by gnome-i18n
 **/
const GList *
gnome_i18n_get_language_list (const gchar *category_name)
{
  g_warning (G_STRLOC ": This function is deprecated, "
	     "use g_i18n_get_language_list instead.");

#if 0
  return g_i18n_get_language_list (category_name);
#else
  return NULL;
#endif
}

/**
 * gnome_i18n_set_preferred_language:
 * @val: Preferred language
 * 
 * This sets the user's preferred language in the Gnome config
 * database.  This value can always be overridden by the standard
 * environment variables.  It exists so that a config applet which
 * chooses the preferred language has a standard place to put the
 * resulting information.
 **/
void
gnome_i18n_set_preferred_language (const char *val)
{
#ifdef FIXME
  gnome_config_set_string (LANGKEY, val);
#endif
}

/**
 * gnome_i18n_init:
 * 
 * Initialize the i18n environment variables (if not already set) from
 * the Gnome config database.  Ordinarily this should not be called by
 * user code.
 **/
void
gnome_i18n_init (void)
{
#if 0
  const gchar *val = g_i18n_guess_category_value ("LC_ALL");
#else
  const gchar *val = NULL;
#endif

  if (val == NULL)
    {
      /* No value in environment.  So we might set up environment
	 according to what is in the config database.  We do this so
	 that the user can override the config db using the
	 environment.  */
#ifdef FIXME
      val = gnome_config_get_string (LANGKEY);
#endif
      if (val != NULL) 
        {
#ifdef HAVE_SETENV      
	  setenv ("LC_ALL", val, 1);
#else
#ifdef HAVE_PUTENV
	  /* It is not safe to free the value passed to putenv.  */
	  putenv (g_strconcat ("LC_ALL=", val, NULL));
#endif
#endif
	}
    }
}

/**
 * gnome_i18n_get_preferred_language:
 * 
 * Return value: the preferred language as set in the Gnome config database.
 **/
char *
gnome_i18n_get_preferred_language (void)
{
#ifdef FIXME
  return gnome_config_get_string (LANGKEY);
#else
  return NULL;
#endif
}

static GList *numeric_locale_stack = NULL;

/**
 * gnome_i18n_push_c_numeric_locale:
 *
 * Description:  Pushes the current LC_NUMERIC locale onto a stack, then 
 * sets LC_NUMERIC to "C".  This way you can safely read write flaoting
 * point numbers all in the same format.  You should make sure that
 * code between #gnome_i18n_push_c_numeric_locale and
 * #gnome_i18n_pop_c_numeric_locale doesn't do any setlocale calls or locale
 * may end up in a strange setting.  Also make sure to always pop the
 * c numeric locale after you've pushed it.
 **/
void
gnome_i18n_push_c_numeric_locale (void)
{
	char *current;

	current = g_strdup (setlocale (LC_NUMERIC, NULL));
	numeric_locale_stack = g_list_prepend (numeric_locale_stack,
					       current);
	setlocale (LC_NUMERIC, "C");
}

/**
 * gnome_i18n_pop_c_numeric_locale:
 *
 * Description:  Pops the last LC_NUMERIC locale from the stack (where
 * it was put with #gnome_i18n_push_c_numeric_locale).  Then resets the
 * current locale to that one.
 **/
void
gnome_i18n_pop_c_numeric_locale (void)
{
	char *old;

	if (numeric_locale_stack == NULL)
		return;

	old = numeric_locale_stack->data;

	setlocale (LC_NUMERIC, old);

	numeric_locale_stack = g_list_remove (numeric_locale_stack, old);

	g_free (old);
}
