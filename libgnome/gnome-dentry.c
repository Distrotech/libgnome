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
#include "gnome-mime.h"
#include "gnome-dentry.h"
#include "gnome-exec.h"

/* g_free already checks if x is NULL */
#define free_if_empty(x) g_free (x)


/**
 * gnome_desktop_entry_load_flags_conditional:
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
	void *iterator;
	char *key,*value;
	GList *dnd;
	gboolean is_kde = FALSE;
	
	g_assert (file != NULL);

	prefix = g_strconcat ("=", file, "=/Desktop Entry/", NULL);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	name = gnome_config_get_translated_string ("Name");
	if (!name) {
		gnome_config_pop_prefix ();

		prefix = g_strconcat ("=", file, "=/KDE Desktop Entry/", NULL);
		gnome_config_push_prefix (prefix);
		g_free (prefix);

		is_kde = TRUE;

		name = gnome_config_get_translated_string ("Name");
		if (!name) {
			gnome_config_pop_prefix ();
			return NULL;
		}
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
	
	newitem = g_new0 (GnomeDesktopEntry, 1);

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
	newitem->is_kde        = is_kde;
	gnome_config_get_vector ("WMClasses", &newitem->wm_classes_length, &newitem->wm_classes);
	
	icon_base              = gnome_config_get_string ("Icon");

	if (icon_base && *icon_base) {
		/* Sigh, now we need to make them local to the gnome install */
		if (*icon_base != '/') {
			/* We look for KDE icons in hardcoded /usr/share/icons
			 * I don't how we can efficiently look in the "right"
			 * place - maybe a configure time test for KDE location?
			 */
			if (newitem->is_kde) {
				gchar *iconname = g_concat_dir_and_file ("/usr/share/icons/", icon_base);
				if (g_file_exists (iconname))
					newitem->icon = iconname;
				else {
					g_free (iconname);
					newitem->icon = NULL;
				}
			} else
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

	gnome_config_push_prefix ("");
	/*This is bad, but gnome_config leaves us no choice*/
	if(is_kde)
		prefix = g_strconcat ("=", file, "=/KDE Desktop Entry/", NULL);
	else
		prefix = g_strconcat ("=", file, "=/Desktop Entry/", NULL);
	iterator = gnome_config_init_iterator(prefix);
	g_free(prefix);
	dnd = NULL;
	while((iterator=gnome_config_iterator_next(iterator,&key,&value))) {
		if(strncmp(key,"DropAction",10)==0) {
			char **argv;
			int argc;
			
			gnome_config_make_vector(value,&argc,&argv);
			if(argc<=1) {
				g_strfreev(argv);
				g_free(key);
				g_free(value);
				continue;
			}

			dnd = g_list_append(dnd,argv);
		}
		g_free(key);
		g_free(value);
	}
	newitem->dnd_entries = dnd;
	gnome_config_pop_prefix();
	
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
	
	g_return_if_fail (dentry != NULL);
	g_return_if_fail (dentry->location != NULL);
	g_return_if_fail (!dentry->is_kde);

	gnome_config_push_prefix ("");
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

	if (dentry->wm_classes)
		gnome_config_set_vector ("WMClasses", dentry->wm_classes_length,
					 (const char * const *) dentry->wm_classes);
	
	if (dentry->dnd_entries) {
		GList *dnd = dentry->dnd_entries;
		int i;
		for(i=0;dnd;i++,dnd=g_list_next(dnd)) {
			char **argv=dnd->data;
			int argc;
			char *s;
			if(!argv) continue;
			for(argc=0;argv[argc];argc++)
				;
			if(argc<2) continue;
			s = g_strdup_printf("DropAction%d",i);
			gnome_config_set_vector (s, argc,
						 (const char * const *)argv);
			g_free(s);
		}
	}

	gnome_config_pop_prefix ();
	prefix = g_strconcat ("=", dentry->location, "=", NULL);
	gnome_config_sync_file (prefix);
	gnome_config_drop_file (prefix);
	g_free (prefix);

	gnome_config_pop_prefix ();
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
		g_strfreev (item->wm_classes);
		g_list_foreach(item->dnd_entries,(GFunc)g_strfreev,NULL);
		g_list_free(item->dnd_entries);
		g_free (item);
	}
}

/* Replace %s and %f and %u, by data from drops, this may later
   work for kde entries as well, it will ignore any other %<letter>
   to let it be substituted later for kde entries
   %s == directly the data as a string
   %f == list of files dropped
   %u == list of files as url's*/
static gchar *
gnome_desktop_entry_sub_data (gchar *arg, gpointer data)
{
	char *p, *q;
	char tmp;
	GString *result = NULL;
	GList *list,*li;

	p = arg;
	q = strchr(arg, '%');
	while (q) {
		tmp = *q;
		*q = '\0';
		if (!result)
			result = g_string_new (p);
		else
			g_string_append (result, p);
		*q = tmp;

		q++;
		switch (*q) {
		case '\0':
			g_string_append_c (result, '%');
			q = NULL;
			p = NULL;
			break;
		case 's':
			/* put in directly the data as a string */
			if(data)
				g_string_append(result,(char *)data);
			break;
		case 'f':
			/* get a list of files from data */
			if(data) {
				list = gnome_uri_list_extract_filenames((char *)data);
				
				for(li=list;li;li=g_list_next(li)) {
					if(li!=list)
						g_string_append_c (result, ' ');
					g_string_append (result, (char *)(li->data));
				}

				gnome_uri_list_free_strings(list);
			}
			break;
		case 'u':
			/* get a list of url's from data */
			if(data) {
				list = gnome_uri_list_extract_uris((char *)data);
				
				for(li=list;li;li=g_list_next(li)) {
					if(li!=list)
						g_string_append_c (result, ' ');
					g_string_append (result, (char *)(li->data));
				}

				gnome_uri_list_free_strings(list);
			}
			break;
		default:
			g_string_append_c (result, '%');
			g_string_append_c (result, *q);
			break;
		}

		if (q) {
			p = q + 1;
			q = strchr (p, '%');
		}
	}

	if (result) {
		char *r = result->str;
		
		if (p)
			g_string_append (result, p);
		arg = result->str;
		g_string_free (result, FALSE);

		return r;
	} else
		return NULL;

}

/* Replace the KDE subsitution strings %... in this argument
 * if any are found, returns g_malloc()'d string containing
 * new argument, otherwise NULL.
 */
static gchar *
gnome_desktop_entry_sub_kde_arg (GnomeDesktopEntry *item, gchar *arg)
{
	char *p, *q;
	char tmp;
	GString *result = NULL;

	p = arg;
	q = strchr(arg, '%');
	while (q) {
		tmp = *q;
		*q = '\0';
		if (!result)
			result = g_string_new (p);
		else
			g_string_append (result, p);
		*q = tmp;

		q++;
		switch (*q) {
		case '\0':
			q = NULL;
			p = NULL;
			break;
		case 'c':
			/* The comment field */
			if (item->comment)
				g_string_append (result, item->comment);
			break;
		case 'i':
			/* The item field */
			if (item->icon) {
				g_string_append (result, "-icon ");
				g_string_append (result, item->icon);
			}
			break;
		case 'm':
			/* The mini-icon field. Ignore for now.
			 * sometime this seems to be %mi, so we
			 * ignore that too.
			 */
			if (*(q+1) == 'i')
				q++;
			break;
		case 'f': /* File arguments */
		case 'u': /* File arguments as URLs */
		default:
			break;
		}

		if (q) {
			p = q + 1;
			q = strchr (p, '%');
		}
	}

	if (result) {
		char *r = result->str;
		
		if (p)
			g_string_append (result, p);
		arg = result->str;
		g_string_free (result, FALSE);

		return r;
	} else
		return NULL;
}

/**
 * gnome_desktop_entry_launch_full:
 * @item: a gnome desktop entry.
 * @the_argc: the number of arguments to invoke the desktop entry with.
 * @the_argv: a vector of arguments for calling the program in @item
 * @info: the info field from the drop data (<0 means this isn't a drop launch)
 * @data: the selection data from the drop (NULL means this isn't a drop launch)
 *
 * Launches the program associated with @item with @the_argv as its
 * arguments. It can also use an alternative execution vector in that was
 * set for one of the drop targets. The info specifies the index of the 
 * vector in dnd_entries, it can be -1 to mean this isn't a drop launch,
 * or you can set data to NULL to mean the same thing. The name info comes
 * from the fact that most likely that will be the info field in the
 * GtkTargets array.
 *
 * Returns the pid of the process that was run
 */
int
gnome_desktop_entry_launch_full (GnomeDesktopEntry *item,
				 int the_argc, char *the_argv[],
				 int info, gpointer data)
{
	char *uargv[4];
	char *exec_str;
	char **term_argv;
	int term_argc = 0;
	char *xterm_argv[2];
	char **argv;
	GSList *args_to_free = NULL;
	gchar *sub_arg;
	int i, argc;
	int pid;

	g_assert (item != NULL);

	/*if info is over a thousand, use the drop execution string*/
	if (!item->terminal && the_argc == 0 && !item->is_kde && (info<0 || !data))
	    exec_str = g_strjoinv (" ", (char **)(item->exec));
	else {
		char **exec_argv;
		int exec_argc;
		
		/*decide which exec vector are we using*/
		if(info<0 || !data) {
			exec_argv = item->exec;
			exec_argc = item->exec_length;
		} else {
			char **a = g_list_nth_data(item->dnd_entries,info);
			/*this is not a good entry or it doesn't exist*/
			if(!a || !a[1]) {
				exec_argv = item->exec;
				exec_argc = item->exec_length;
			} else {
				exec_argv = &a[1];
				for(exec_argc=0;a[exec_argc+1];exec_argc++)
					;
			}
		}

		if (item->terminal) {
			gnome_config_get_vector ("/Gnome/Applications/Terminal",
						 &term_argc, &term_argv);
			if (term_argv == NULL) {
				term_argc = 2;
				term_argv = xterm_argv;
				xterm_argv[0] = "xterm";
				xterm_argv[1] = "-e";
			}
		}
		
		/* ... terminal arguments */
		argc = the_argc + term_argc + exec_argc;
		argv = (char **) g_malloc ((argc + 1) * sizeof (char *));

		/* Assemble together... */

		/* ... terminal arguments */
		for (i = 0; i < term_argc; ++i)
			argv[i] = term_argv[i];

		/* ... arguments from the desktop file */
		for (i = 0; i < exec_argc; ++i) {
			char *arg;
			arg = exec_argv[i];

			if (info>=0 && data) {
				sub_arg = gnome_desktop_entry_sub_data(arg,data);
				if(sub_arg) {
					args_to_free = g_slist_prepend (args_to_free, sub_arg);
					arg = sub_arg;
				}
			}

			if (item->is_kde) {
				sub_arg = gnome_desktop_entry_sub_kde_arg (item, arg);
				if (sub_arg) {
					args_to_free = g_slist_prepend (args_to_free, sub_arg);
					arg = sub_arg;
				}
			}

			argv[term_argc + i] = arg;
		}
		
		/* ... supplied arguments */
		for (i = 0; i < the_argc; i++)
			argv[term_argc + exec_argc + i] = the_argv [i];
		
		argv[argc] = NULL;
		
		exec_str = g_strjoinv (" ", (char **)argv);
		
		/* clean up */
		if (term_argc && term_argv != xterm_argv)
			g_strfreev (term_argv);

		if (args_to_free) {
			g_slist_foreach (args_to_free, (GFunc)g_free, NULL);
			g_slist_free (args_to_free);
		}
		
		g_free ((char *) argv);
	}

	uargv[0] = "/bin/sh";
	uargv[1] = "-c";
	uargv[2] = exec_str;
	uargv[3] = NULL;

	/* FIXME: do something if there's an error.  */
	pid = gnome_execute_async (NULL, 4, uargv);

	g_free (exec_str);
	
	return pid;
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
	gnome_desktop_entry_launch_full (item, the_argc, the_argv, -1, NULL);
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
	gnome_desktop_entry_launch_full (item, 0, 0, -1, NULL);
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

static GList *
copy_list_of_vectors(GList *list)
{
	GList *new=NULL;
	for(;list;list=g_list_next(list))
		new = g_list_prepend(new, g_copy_vector(list->data));
	return g_list_reverse(new);
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
	newitem = g_new0 (GnomeDesktopEntry, 1);
	
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
	newitem->icon	       = g_strdup (source->icon);
	newitem->is_kde	       = source->is_kde;

	newitem->wm_classes_length   = source->wm_classes_length;
	newitem->wm_classes          = g_copy_vector (source->wm_classes);

	newitem->dnd_entries   = copy_list_of_vectors(source->dnd_entries);
	
	return newitem;
}
