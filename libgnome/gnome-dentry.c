/*
 * Support for manipulating .desktop files
 *
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza
 *          Federico Mena
 */

#include <config.h>
#include <glib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gnome-defs.h"
#include "gnome-util.h"
#include "gnome-config.h"
#include "gnome-dentry.h"
#include "gnome-exec.h"

/* g_free already checks if x is NULL */
#define free_if_empty(x) g_free (x)

char *
gnome_is_program_in_path (const char *program)
{
	static char **paths = NULL;
	char **p;
	char *f;
	
	if (!paths)
	  paths = g_strsplit(getenv("PATH"), ":", -1);

	p = paths;
	while (*p){
		f = g_concat_dir_and_file (*p, program);
		if (g_file_exists(f))
			return f;
		g_free(f);
		p++;
	}
	return 0;
}
	      
GnomeDesktopEntry *
gnome_desktop_entry_load_flags_conditional (const char *file, int clean_from_memory, int unconditional)
{
	GnomeDesktopEntry *newitem;
	char *prefix;
	char *name, *type;
	char *try_file;
	char **exec_vector;
	int exec_length;
	char *icon_base;
	char *p;
	
	g_assert (file != NULL);

	prefix = g_copy_strings ("=", file, "=/Desktop Entry/", NULL);

	gnome_config_push_prefix (prefix);
	g_free (prefix);

	name = gnome_config_get_translated_string ("Name");
	if (!name) {
		gnome_config_pop_prefix ();
		return NULL;
	}

	/* FIXME: we only test for presence of Exec/TryExec keys if
	 * the type of the desktop entry is not a Directory.  Since
	 * Exec/TryExec may not make sense for other types of desktop
	 * entries, we will later need to make this code smarter.
	 */

	type      = gnome_config_get_string ("Type");
	gnome_config_get_vector ("Exec", &exec_length, &exec_vector);
	try_file  = gnome_config_get_string ("TryExec");
	p = 0;

	if (!type || (strcmp (type, "Directory") != 0)){
		if(!unconditional && ( !exec_vector || (try_file && !(p = gnome_is_program_in_path(try_file))))){
			free_if_empty (p);
			free_if_empty (name);
			free_if_empty (type);
			g_strfreev (exec_vector);
			free_if_empty (try_file);
			
			gnome_config_pop_prefix ();
			return NULL;
		}
		if (p)
			g_free (p);
	}
	
	newitem = g_new (GnomeDesktopEntry, 1);

	newitem->name          = name;
	newitem->comment       = gnome_config_get_translated_string ("Comment");
	newitem->exec_length   = exec_length;
	newitem->exec          = exec_vector;
	newitem->tryexec       = try_file;
	newitem->docpath       = gnome_config_get_string ("DocPath");
	newitem->terminal      = gnome_config_get_bool   ("Terminal=0");
	newitem->type          = type;
	newitem->geometry      = gnome_config_get_string ("Geometry");
	newitem->multiple_args = gnome_config_get_bool   ("MultipleArgs=0");
	newitem->location      = g_strdup (file);
	icon_base	       = gnome_config_get_string ("Icon");
	
	if (icon_base && *icon_base) {
		/* Sigh, now we need to make them local to the gnome install */
		if (*icon_base != '/') {
			newitem->icon = gnome_pixmap_file (icon_base);
			g_free (icon_base);
		} else
			newitem->icon = icon_base;
	} else {
		/*no icon*/
		if(icon_base) g_free(icon_base);
		newitem->icon = NULL;
	}
	gnome_config_pop_prefix ();
	
	if (clean_from_memory){
		prefix = g_copy_strings ("=", file, "=", NULL);
		gnome_config_drop_file (prefix);
		g_free (prefix);
	}
	
	return newitem;
}

GnomeDesktopEntry *
gnome_desktop_entry_load_flags (const char *file, int clean_from_memory)
{
	return gnome_desktop_entry_load_flags_conditional (file, clean_from_memory, FALSE);
}

GnomeDesktopEntry *
gnome_desktop_entry_load (const char *file)
{
	return gnome_desktop_entry_load_flags (file, 1);
}

GnomeDesktopEntry *
gnome_desktop_entry_load_unconditional (const char *file)
{
	return gnome_desktop_entry_load_flags_conditional (file, 1, TRUE);
}

/*XXX:this should have same clean_from_memory logic as above maybe???*/
void
gnome_desktop_entry_save (GnomeDesktopEntry *dentry)
{
	char *prefix;
	
	g_assert (dentry != NULL);
	g_assert (dentry->location != NULL);

	prefix = g_copy_strings ("=", dentry->location, "=/Desktop Entry/", NULL);
	gnome_config_clean_section (prefix);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	if (dentry->name)
		gnome_config_set_translated_string ("Name", dentry->name);

	if (dentry->comment)
		gnome_config_set_translated_string ("Comment", dentry->comment);

	if (dentry->exec)
		gnome_config_set_vector ("Exec", dentry->exec_length,
					 (const char * const *) dentry->exec);

	if (dentry->tryexec)
		gnome_config_set_string ("TryExec", dentry->tryexec);

	if (dentry->icon)
		gnome_config_set_string ("Icon", dentry->icon);

	if (dentry->geometry)
		gnome_config_set_string ("Geometry", dentry->geometry);
	
	if (dentry->docpath)
		gnome_config_set_string ("DocPath", dentry->docpath);

	gnome_config_set_bool ("Terminal", dentry->terminal);
	gnome_config_set_bool ("MultipleArgs", dentry->multiple_args);
	
	if (dentry->type)
		gnome_config_set_string ("Type", dentry->type);

	gnome_config_pop_prefix ();
	gnome_config_sync ();
	prefix = g_copy_strings ("=", dentry->location, "=", NULL);
	gnome_config_drop_file(prefix);
	g_free(prefix);
}

void
gnome_desktop_entry_free (GnomeDesktopEntry *item)
{
  if(item)
    {
      free_if_empty (item->name);
      free_if_empty (item->comment);
      g_strfreev (item->exec);
      free_if_empty (item->tryexec);
      free_if_empty (item->icon);
      free_if_empty (item->docpath);
      free_if_empty (item->type);
      free_if_empty (item->location);
      free_if_empty (item->geometry);
      g_free (item);
    }
}

void
gnome_desktop_entry_launch_with_args (GnomeDesktopEntry *item, int the_argc, char *the_argv[])
{
	char *uargv[4];
	char *exec_str;

	g_assert (item != NULL);

	if (item->terminal) {
		char **term_argv;
		int term_argc;
		char *xterm_argv[2];
		char **argv;
		int i, argc;

		gnome_config_get_vector ("/Gnome/Applications/Terminal",
					 &term_argc, &term_argv);
		if (term_argv == NULL) {
			term_argc = 2;
			term_argv = xterm_argv;
			xterm_argv[0] = "xterm";
			xterm_argv[1] = "-e";
		}

		argc = the_argc + term_argc + item->exec_length;
		argv = (char **) malloc ((argc + 1) * sizeof (char *));

		for (i = 0; i < term_argc; ++i)
			argv[i] = term_argv[i];

		for (i = 0; i < item->exec_length; ++i)
			argv[term_argc + i] = item->exec[i];

		for (i = 0; i < the_argc; i++)
			argv[term_argc + item->exec_length + i] = the_argv [i];
		
		argv[argc] = NULL;
		
		exec_str = g_strjoinv (" ", (char **)argv);

		if (term_argv != xterm_argv)
			g_strfreev (term_argv);

		g_free ((char *) argv);
	} else {
		if (the_argc != 0){
			char **argv;
			int i, argc;
			argc = the_argc + item->exec_length;
			argv = (char **) malloc ((argc + 1) * sizeof (char *));

			for (i = 0; i < item->exec_length; i++)
				argv [i] = item->exec [i];
			for (i = 0; i < the_argc; i++)
				argv [item->exec_length + i] = the_argv [i];
			argv [argc] = NULL;

			exec_str = g_strjoinv (" ", (char **)argv);

			g_free ((char *) argv);
		} else {
			exec_str = g_strjoinv (" ", (char **)(item->exec));
		}
	}

	uargv[0] = gnome_util_user_shell ();
	uargv[1] = "-c";
	uargv[2] = exec_str;
	uargv[3] = NULL;

	/* FIXME: do something if there's an error.  */
	gnome_execute_async (NULL, 4, uargv);

	g_free (uargv[0]);
	g_free (exec_str);
}

void
gnome_desktop_entry_launch (GnomeDesktopEntry *item)
{
	gnome_desktop_entry_launch_with_args (item, 0, 0);
}

void
gnome_desktop_entry_destroy (GnomeDesktopEntry *item)
{
      char *prefix;

      if (!item)
	      return;
      
      prefix = g_copy_strings ("=", item->location, "=", NULL);
      gnome_config_clean_file (prefix);
      g_free (prefix);
      gnome_desktop_entry_free (item);
      gnome_config_sync();
}

GnomeDesktopEntry *gnome_desktop_entry_copy (GnomeDesktopEntry * source)
{
  GnomeDesktopEntry * newitem;

  newitem = g_new (GnomeDesktopEntry, 1);

  newitem->name          = g_strdup (source->name);
  newitem->comment       = g_strdup (source->comment);
  newitem->exec_length   = source->exec_length;
  newitem->exec          = g_copy_vector (source->exec);
  newitem->tryexec       = g_strdup (source->tryexec);
  newitem->docpath       = g_strdup (source->docpath);
  newitem->terminal      = source->terminal;
  newitem->type          = g_strdup (source->type);
  newitem->geometry      = g_strdup (source->geometry);
  newitem->multiple_args = source->multiple_args;
  newitem->location      = g_strdup (source->location);
  newitem->icon	         = g_strdup (source->icon);
	
  return newitem;
}
