#include <config.h>
#include <string.h>

#include "libgnomeP.h"

/* Name of config key we use when looking up preferred language.   */
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
  return getenv("LANG");
}

/* The following is (partly) taken from the gettext package.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

static const gchar *
guess_category_value (const gchar *categoryname)
{
  const gchar *retval;

  /* The highest priority value is the `LANGUAGE' environment
     variable.  This is a GNU extension.  */
  retval = getenv ("LANGUAGE");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* `LANGUAGE' is not set.  So we have to proceed with the POSIX
     methods of looking to `LC_ALL', `LC_xxx', and `LANG'.  On some
     systems this can be done by the `setlocale' function itself.  */

  /* Setting of LC_ALL overwrites all other.  */
  retval = getenv ("LC_ALL");  
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Next comes the name of the desired category.  */
  retval = getenv (categoryname);
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Last possibility is the LANG environment variable.  */
  retval = getenv ("LANG");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  return NULL;
}


static GHashTable *category_table= NULL;


/**
 * gnome_i18n_get_language_list:
 * @category_name: Name of category to look up, e.g. "LC_MESSAGES".
 * 
 * This computes a list of language strings.  It searches in the
 * standard environment variables to find the list, which is sorted
 * in order from most desirable to least desirable.  The `C' locale
 * is appended to the list if it does not already appear.
 * If @category_name is %NULL, then LC_ALL is assumed.
 * 
 * Return value: the list of languages
 **/
GList *
gnome_i18n_get_language_list (const gchar *category_name)
{
  GList *list;

  if (!category_name)
    category_name= "LC_ALL";

  if (category_table)
    {
      list= g_hash_table_lookup (category_table, (const gpointer) category_name);
    }
  else
    {
      category_table= g_hash_table_new (g_str_hash, g_str_equal);
      list= NULL;
    }

  if (!list)
    {
      gint c_locale_defined= FALSE;
  
      const gchar *category_value;
      gchar *category_memory;

      category_value = guess_category_value (category_name);
      if (! category_value)
	category_value = "C";
      category_memory= g_malloc (strlen (category_value)+1);
      
      while (category_value[0] != '\0')
	{
	  while (category_value[0] != '\0' && category_value[0] == ':')
	    ++category_value;
	  
	  if (category_value[0] != '\0')
	    {
	      char *cp= category_memory;
	      
	      while (category_value[0] != '\0' && category_value[0] != ':')
		*category_memory++= *category_value++;
	      
	      category_memory[0]= '\0'; 
	      category_memory++;
	      
	      if (strcmp (cp, "C") == 0)
		c_locale_defined= TRUE;
	      
	      list= g_list_append (list, cp);
	    }
	}
      
      if (!c_locale_defined)
	list= g_list_append (list, "C");

      g_hash_table_insert (category_table, (gpointer) category_name, list);
    }
  
  return list;
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
  gnome_config_set_string (LANGKEY, val);
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
  const gchar *val = guess_category_value ("LC_ALL");

  if (val == NULL)
    {
      /* No value in environment.  So we might set up environment
	 according to what is in the config database.  We do this so
	 that the user can override the config db using the
	 environment.  */
      val = gnome_config_get_string (LANGKEY);
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
const char *
gnome_i18n_get_preferred_language (void)
{
  return gnome_config_get_string (LANGKEY);
}
