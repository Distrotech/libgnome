/*
 * Support for manipulating .desktop files
 *
 * (C) 1997 the Free Software Foundation
 *
 * Authors: Miguel de Icaza.
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

#define free_if_empty(x) { if (x) g_free (x); }

int
gnome_is_program_in_path (char *program)
{
	static char *path;
	static char **paths;
	char **p;
	char *f;
	
	if (!path){
		char *p;
		int i, pc = 1;

		path = strdup (getenv ("PATH"));
		for (p = path; *p; p++)
			if (*p == ':')
				pc++;

		paths = (char **) g_malloc (sizeof (char *) * (pc+1));

		for (p = path, i = 0; i < pc; i++){
			paths [i] = strtok (p, ":");
			p = NULL;
		}
		paths [pc] = NULL;
	}
	p = paths;
	while (*p){
		f = g_concat_dir_and_file (*p, program);
		if (g_file_exists (f)){
			g_free (f);
			return 1;
		}
		g_free (f);
		p++;
	}
	return 0;
}
	      
GnomeDesktopEntry *
gnome_desktop_entry_load (char *file)
{
	GnomeDesktopEntry *newitem;
	char *prefix;
	char *exec_file, *try_file, *dot;

	g_assert(file != NULL);

	prefix = g_copy_strings ("=", file, "=/Desktop Entry/", NULL);

	gnome_config_set_prefix (prefix);
	g_free (prefix);

	exec_file = gnome_config_get_string ("Exec");
	if (!exec_file){
		gnome_config_drop_prefix ();
		return 0;
	}
	try_file = gnome_config_get_string ("TryExec");
	if (try_file){
		if (!gnome_is_program_in_path (try_file)){
			g_free (try_file);
			g_free (exec_file);
			gnome_config_drop_prefix ();
			return 0;
		}
	}
	newitem = g_new(GnomeDesktopEntry, 1);
	newitem->exec      = exec_file;
	newitem->tryexec   = try_file;
	newitem->icon_base = gnome_config_get_string ("Icon");
	newitem->docpath   = gnome_config_get_string ("DocPath");
	newitem->info      = gnome_config_get_string ("Info");
	newitem->terminal  = gnome_config_get_bool   ("Terminal");
	newitem->type      = gnome_config_get_string ("Type");
	newitem->location  = strdup (file);
	
	if (newitem->icon_base && *newitem->icon_base){
		dot = strstr (newitem->icon_base, ".xpm");

		if (dot){
			*dot = 0;

			newitem->small_icon = g_copy_strings (newitem->icon_base,
					      "-small.xpm", NULL);
			newitem->transparent_icon = g_copy_strings (newitem->icon_base,
					      "-transparent.xpm", NULL);
			*dot = '.';
		}

		/* Sigh, now we need to make them local to the gnome install */
		if (*newitem->icon_base != '/'){
			char *s = newitem->small_icon;
			char *t = newitem->transparent_icon;

			newitem->small_icon = gnome_pixmap_file (s);
			newitem->transparent_icon = gnome_pixmap_file (t);
			g_free (s);
			g_free (t);
		}
	} else {
		newitem->small_icon = newitem->transparent_icon = 0;
	}
	gnome_config_drop_prefix ();
	return newitem;
}

void
gnome_desktop_entry_save (GnomeDesktopEntry *dentry)
{
	char *prefix;
	
	g_assert(dentry != NULL);
	g_assert(dentry->location != NULL);

	prefix = g_copy_strings("=", dentry->location, "=/Desktop Entry", NULL);

	gnome_config_clean_section(prefix);

	prefix = g_copy_strings(prefix, "/", NULL);
	gnome_config_set_prefix(prefix);
	g_free(prefix);

	if (dentry->exec)
		gnome_config_set_string("Exec", dentry->exec);

	if (dentry->tryexec)
		gnome_config_set_string("TryExec", dentry->tryexec);

	if (dentry->icon_base)
		gnome_config_set_string("Icon", dentry->icon_base);

	if (dentry->docpath)
		gnome_config_set_string("DocPath", dentry->docpath);

	if (dentry->info)
		gnome_config_set_string("Info", dentry->info);

	gnome_config_set_bool("Terminal", dentry->terminal);

	if (dentry->type)
		gnome_config_set_string("Type", dentry->type);

	gnome_config_drop_prefix();
	gnome_config_sync();
}

void
gnome_desktop_entry_free (GnomeDesktopEntry *item)
{
	g_assert(item != NULL);
	
	free_if_empty (item->exec);
	free_if_empty (item->icon_base);
	free_if_empty (item->docpath);
	free_if_empty (item->info);
	free_if_empty (item->type);
	free_if_empty (item->small_icon);
	free_if_empty (item->transparent_icon);
	free_if_empty (item->location);
	g_free (item);
}

void
gnome_desktop_entry_launch (GnomeDesktopEntry *item)
{
	char *command;

	g_assert(item != NULL);

	if (item->terminal)
		command = g_copy_strings ("(xterm -e \"", item->exec, "\") &", NULL);
	else
		command = g_copy_strings ("(true;", item->exec, ") &", NULL);
	system (command);
	g_free (command);
}

