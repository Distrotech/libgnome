/*
 * Support for manipulating .desktop files
 *
 * (C) 1997, 1999 the Free Software Foundation
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


/**
 * gnome_desktop_entry_load_flags:
 * @file: a file name that contains a desktop entry.
 * @clean_from_memory_after_load: flag
 * @unconditional: flag
 *
 * Returns a newly created desktop entry loaded from @file or NULL
 * if the file does not exist.
 *
 * if @unconditional is TRUE then the desktop entry is loaded even if
 * it contains stale data, otherwise, NULL is returned if stale data
 * is found (like, the program referenced not existing).
 *
 * if @clean_from_memory_after_load is TRUE, then any data cached used by loading
 * process is discarded after loading the desktop entry.
 */
GnomeDesktopEntry *
gnome_desktop_entry_load_flags_conditional (const char *file,
					    int clean_from_memory_after_load,
					    int unconditional)
{
	GnomeDesktopEntry *newitem;
	char *prefix;
	char *name, *type;
	char *try_file;
	char **exec_vector;
	int exec_length;
	char *icon_base;
	char *p = NULL;
	
	g_assert (file != NULL);

	prefix = g_strconcat ("=", file, "=/Desktop Entry/", NULL);
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
	
	if (clean_from_memory_after_load){
		prefix = g_strconcat ("=", file, "=", NULL);
		gnome_config_drop_file (prefix);
		g_free (prefix);
	}
	
	return newitem;
}

/**
 * gnome_desktop_entry_load_flags:
 * @file: a file name that contains a desktop entry.
 * @clean_from_memory_after_load: flag
 *
 * Returns a newly created desktop entry loaded from @file or NULL
 * if the file does not exist or contains stale data.
 *
 * if @clean_from_memory_after_load is TRUE, then any data cached used by loading
 * process is discarded after loading the desktop entry.
 */
GnomeDesktopEntry *
gnome_desktop_entry_load_flags (const char *file, int clean_from_memory_after_load)
{
	return gnome_desktop_entry_load_flags_conditional (file, clean_from_memory_after_load, FALSE);
}

/**
 * gnome_desktop_entry_load:
 * @file: a file name that contains a desktop entry.
 *
 * Returns a newly created desktop entry loaded from @file or NULL
 * if the file does not exist or contains stale data.
 */
GnomeDesktopEntry *
gnome_desktop_entry_load (const char *file)
{
	return gnome_desktop_entry_load_flags (file, 1);
}

/**
 * gnome_desktop_entry_load_unconditional:
 * @file: file name where the desktop entry resides
 *
 * Returns a newly created GnomeDesktopEntry loaded from
 * @file even if the file does not contain a valid desktop entry or NULL
 * if the file does not exist.
 */
GnomeDesktopEntry *
gnome_desktop_entry_load_unconditional (const char *file)
{
	return gnome_desktop_entry_load_flags_conditional (file, 1, TRUE);
}

/**
 * gnome_desktop_entry_save:
 * @dentry: A gnome desktop entry.
 *
 * Saves the desktop entry to disk
 */
void
gnome_desktop_entry_save (GnomeDesktopEntry *dentry)
{
	char *prefix;
	
/* XXX:this should have same clean_from_memory logic as above maybe??? */
	
	g_assert (dentry != NULL);
	g_assert (dentry->location != NULL);

	prefix = g_strconcat ("=", dentry->location, "=/Desktop Entry/", NULL);
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
	prefix = g_strconcat ("=", dentry->location, "=", NULL);
	gnome_config_sync_file (prefix);
	gnome_config_drop_file (prefix);
	g_free (prefix);
}

/**
 * gnome_desktop_entry_free:
 * @item: a gnome desktop entry.
 *
 * Releases the information used by @item.
 */
void
gnome_desktop_entry_free (GnomeDesktopEntry *item)
{
	if(item){
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

/**
 * gnome_desktop_entry_launch_with_args:
 * @item: a gnome desktop entry.
 * @the_argc: the number of arguments to invoke the desktop entry with.
 * @the_argv: a vector of arguments for calling the program in @item
 *
 * Launches the program associated with @item with @the_argv as its
 * arguments.
 */
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

/**
 * gnome_desktop_entry_launch:
 * @item: a gnome desktop entry.
 *
 * Launchs the program associated to the @item desktop entry.
 */
void
gnome_desktop_entry_launch (GnomeDesktopEntry *item)
{
	gnome_desktop_entry_launch_with_args (item, 0, 0);
}

/**
 * gnome_desktop_entry_destroy:
 * @item: a gnome deskop entry.
 *
 * Erases the file that represents @item and releases the 
 * memory used by @item.
 */
void
gnome_desktop_entry_destroy (GnomeDesktopEntry *item)
{
      char *prefix;

      if (!item)
	      return;
      
      prefix = g_strconcat ("=", item->location, "=", NULL);
      gnome_config_clean_file (prefix);
      gnome_desktop_entry_free (item);
      gnome_config_sync_file (prefix);
      g_free (prefix);
}

/**
 * gnome_desktop_entry_copy:
 * @source: a GnomeDesktop entry.
 *
 * Returns a copy of the @source GnomeDesktopEntry
 */
GnomeDesktopEntry *
gnome_desktop_entry_copy (GnomeDesktopEntry * source)
{
	GnomeDesktopEntry * newitem;
	
	g_return_val_if_fail (source != NULL, NULL);
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
