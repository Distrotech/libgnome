#include "libgnome.h"
#include "gnome-i18n.h"

const char *gnome_i18n_get_language(void)
{
  return getenv("LANG");
}

/* The following is (partly) taken from the gettext package.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.  */

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

  /* We use C as the default domain.  POSIX says this is implementation
     defined.  */
  return "C";
}


static GHashTable *category_table= NULL;


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
