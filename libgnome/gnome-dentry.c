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

		path = g_strdup (getenv ("PATH"));
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

/* Are the following two functions useful enough to be put into gnome-config? */

static char *
get_translated_string (char *key)
{
	/* FIXME: I don't know if getenv("LANG") is the right way to get the language */

	char *lang;
	char *tkey;
	char *value;

	lang = getenv ("LANG");

	if (lang) {
		tkey = g_copy_strings (key, "[", lang, "]", NULL);
		value = gnome_config_get_string (tkey);
		g_free (tkey);

		if (!value)
			value = gnome_config_get_string (key); /* Fall back to untranslated case */
	} else
		value = gnome_config_get_string (key);

	return value;
}

static void
put_translated_string (char *key, char *string)
{
	/* FIXME: same as previous function */

	char *lang;
	char *tkey;

	lang = getenv ("LANG");

	if (lang) {
		tkey = g_copy_strings (key, "[", lang, "]", NULL);
		gnome_config_set_string (tkey, string);
		g_free (tkey);
	} else
		gnome_config_set_string (key, string);
}
	      
GnomeDesktopEntry *
gnome_desktop_entry_load (char *file)
{
	GnomeDesktopEntry *newitem;
	char *prefix;
	char *name, *type;
	char *exec_file, *try_file, *dot;

	g_assert (file != NULL);

	prefix = g_copy_strings ("=", file, "=/Desktop Entry/", NULL);

	gnome_config_push_prefix (prefix);
	g_free (prefix);

	name = get_translated_string ("Name");
	if (!name) {
		gnome_config_pop_prefix ();
		return 0;
	}

	/* FIXME: we only test for presence of Exec/TryExec keys if
	 * the type of the desktop entry is not a Directory.  Since
	 * Exec/TryExec may not make sense for other types of desktop
	 * entries, we will later need to make this code smarter.
	 */

	type      = gnome_config_get_string ("Type");
	exec_file = gnome_config_get_string ("Exec");
	try_file  = gnome_config_get_string ("TryExec");

	if (!type || (strcmp (type, "Directory") != 0)) {
		if (!exec_file) {
			free_if_empty (name);
			free_if_empty (type);
			free_if_empty (try_file);

			gnome_config_pop_prefix ();
			return 0;
		}

		if (try_file) {
			if (!gnome_is_program_in_path (try_file)) {
				free_if_empty (name);
				free_if_empty (type);
				free_if_empty (exec_file);
				free_if_empty (try_file);

				gnome_config_pop_prefix ();
				return 0;
			}
		}
	}
	
	newitem = g_new (GnomeDesktopEntry, 1);

	newitem->name      = get_translated_string ("Name");
	newitem->comment   = get_translated_string ("Comment");
	newitem->exec      = exec_file;
	newitem->tryexec   = try_file;
	newitem->icon_base = gnome_config_get_string ("Icon");
	newitem->docpath   = gnome_config_get_string ("DocPath");
	newitem->terminal  = gnome_config_get_bool   ("Terminal");
	newitem->type      = gnome_config_get_string ("Type");
	newitem->location  = g_strdup (file);
	
	if (newitem->icon_base && *newitem->icon_base) {
		dot = strstr (newitem->icon_base, ".xpm");

		if (dot) {
			*dot = 0;

			newitem->small_icon = g_copy_strings (newitem->icon_base,
					      "-small.xpm", NULL);
			newitem->transparent_icon = g_copy_strings (newitem->icon_base,
					      "-transparent.xpm", NULL);
			newitem->opaque_icon = g_copy_strings (newitem->icon_base,
					      ".xpm", NULL);
			*dot = '.';
		}

		/* Sigh, now we need to make them local to the gnome install */
		if (*newitem->icon_base != '/') {
			char *s = newitem->small_icon;
			char *t = newitem->transparent_icon;
			char *o = newitem->opaque_icon;

			newitem->small_icon = gnome_pixmap_file (s);
			newitem->transparent_icon = gnome_pixmap_file (t);
			newitem->opaque_icon = gnome_pixmap_file (o);
			g_free (s);
			g_free (t);
			g_free (o);
		}
	} else {
		newitem->small_icon = newitem->transparent_icon =
			newitem->opaque_icon = 0;
	}
	gnome_config_pop_prefix ();
	return newitem;
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
		put_translated_string ("Name", dentry->name);

	if (dentry->comment)
		put_translated_string ("Comment", dentry->comment);

	if (dentry->exec)
		gnome_config_set_string ("Exec", dentry->exec);

	if (dentry->tryexec)
		gnome_config_set_string ("TryExec", dentry->tryexec);

	if (dentry->icon_base)
		gnome_config_set_string ("Icon", dentry->icon_base);

	if (dentry->docpath)
		gnome_config_set_string ("DocPath", dentry->docpath);

	gnome_config_set_bool ("Terminal", dentry->terminal);

	if (dentry->type)
		gnome_config_set_string ("Type", dentry->type);

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

void
gnome_desktop_entry_free (GnomeDesktopEntry *item)
{
	g_assert (item != NULL);
	
	free_if_empty (item->name);
	free_if_empty (item->comment);
	free_if_empty (item->exec);
	free_if_empty (item->icon_base);
	free_if_empty (item->docpath);
	free_if_empty (item->type);
	free_if_empty (item->small_icon);
	free_if_empty (item->transparent_icon);
	free_if_empty (item->opaque_icon);
	free_if_empty (item->location);

	g_free (item);
}

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
