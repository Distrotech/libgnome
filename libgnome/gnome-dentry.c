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
#include "gnome-string.h"

/* g_free already checks if x is NULL */
#define free_if_empty(x) g_free (x)

char *
gnome_is_program_in_path (char *program)
{
	static char **paths = NULL;
	char **p;
	char *f;
	
	if (!paths)
	  paths = gnome_string_split(getenv("PATH"), ":", -1);

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
gnome_desktop_entry_load_flags (char *file, int clean_from_memory)
{
	GnomeDesktopEntry *newitem;
	char *prefix;
	char *name, *type;
	char *exec_file, *try_file;
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
	exec_file = gnome_config_get_string ("Exec");
	try_file  = gnome_config_get_string ("TryExec");
	p = 0;
	
	if (!type || (strcmp (type, "Directory") != 0)){
		if(!exec_file || (try_file && !(p = gnome_is_program_in_path(try_file)))){
			free_if_empty (p);
			free_if_empty (name);
			free_if_empty (type);
			free_if_empty (exec_file);
			free_if_empty (try_file);
			
			gnome_config_pop_prefix ();
			return NULL;
		}
		if (p)
			g_free (p);
	}
	
	newitem = g_new (GnomeDesktopEntry, 1);

	newitem->name          = gnome_config_get_translated_string ("Name");
	newitem->comment       = gnome_config_get_translated_string ("Comment");
	newitem->exec          = exec_file;
	newitem->tryexec       = try_file;
	newitem->docpath       = gnome_config_get_string ("DocPath");
	newitem->terminal      = gnome_config_get_bool   ("Terminal=0");
	newitem->type          = gnome_config_get_string ("Type");
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
	} else 
		/*no icon*/
		newitem->icon = NULL;
	gnome_config_pop_prefix ();
	
	if (clean_from_memory){
		prefix = g_copy_strings ("=", file, "=", NULL);
		gnome_config_clean_file (prefix);
		g_free (prefix);
	}
	
	return newitem;
}

GnomeDesktopEntry *
gnome_desktop_entry_load (char *file)
{
	return gnome_desktop_entry_load_flags (file, 1);
}

void
gnome_desktop_entry_save (GnomeDesktopEntry *dentry)
{
	char *prefix;
	
	g_assert (dentry != NULL);
	g_assert (dentry->location != NULL);

	prefix = g_copy_strings ("=", dentry->location, "=/Desktop Entry", NULL);

	gnome_config_clean_section (prefix);

	prefix = g_copy_strings (prefix, "/", NULL);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	if (dentry->name)
		gnome_config_set_translated_string ("Name", dentry->name);

	if (dentry->comment)
		gnome_config_set_translated_string ("Comment", dentry->comment);

	if (dentry->exec)
		gnome_config_set_string ("Exec", dentry->exec);

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
}

void
gnome_desktop_entry_free (GnomeDesktopEntry *item)
{
  if(item)
    {
      free_if_empty (item->name);
      free_if_empty (item->comment);
      free_if_empty (item->exec);
      free_if_empty (item->icon);
      free_if_empty (item->docpath);
      free_if_empty (item->type);
      free_if_empty (item->location);
      
      g_free (item);
    }
}

/* TODO this needs redoing properly (we need to write a
   gnome_fork_and_execlp() probably) */
void
gnome_desktop_entry_launch (GnomeDesktopEntry *item)
{
	char *command;

	g_assert (item != NULL);

	if (item->terminal)
	  command = g_copy_strings ("(xterm -e \"", item->exec, "\") &", NULL);
	else
	  command = g_copy_strings ("(true;", item->exec, ") &", NULL);

	system (command);
	g_free (command);
}
