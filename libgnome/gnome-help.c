/*
 * Copyright (C) 1998, 1999, 2000 Red Hat Inc.
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

/*
 * gnome-help: provides localized access to help files and help file display
 * services.
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include "gnome-defs.h"
#include "gnome-help.h"
#include "gnome-i18nP.h"
#include "gnome-util.h"
#include "gnome-url.h"

#define HELP_PROG "gnome-help-browser"

/**
 * gnome_help_file_find_file:
 * @app: Name of this application
 * @path: File that contains the help document
 *
 * Returns a fully resolved file name for path.  Use needs to g_free the path when
 * done.  It looks in LANG, then unser C if LANG is not set
 *
 * I added this because I didnt want to break all apps using 
 * gnome_help_file_path() currently. We need a good solution (if this isnt it)
 * to handle case where locale file didnt exist
 */
gchar *
gnome_help_file_find_file (const gchar *app, const gchar *path)
{
	GList *language_list;
	GString *buf;
	
	gchar *res= NULL;
	gchar *p, c = 0;
	
	language_list= gnome_i18n_get_language_list ("LC_MESSAGES");
	
	while (!res && language_list)
	{
		const gchar *lang;
		
		lang= language_list->data;
		
		buf= g_string_new (NULL);
		g_string_sprintf (buf, "gnome/help/%s/%s/%s", app, lang, path);
		res= (gchar *)gnome_unconditional_datadir_file (buf->str);
		p = strrchr (res, '#');
		if (p) {
			c = *p;
			*p = '\0';
		}
		g_string_free (buf, TRUE);
		
		if (!g_file_exists (res))
		{
			g_free (res);
			res = NULL;
		}

		if (c && res){
			*p = c;
			c = 0;
		}
		
		language_list = language_list->next;
	}
	
	return res;
}

/**
 * gnome_help_file_path:
 * @app: Name of this application
 * @path: File that contains the help document
 *
 * Returns a fully resolved file name for path.  Use needs to g_free the path when
 * done.  It looks in LANG, then unser C if LANG is not set
 */
gchar *
gnome_help_file_path(const gchar *app, const gchar *path)
{
	gchar *res;
	GString *buf;
	
	res= gnome_help_file_find_file (app, path);
	
	/* If we found no document on the language depending datadirs, we
	   return a non existing file from a default datadir.  It's non
	   existing, because 'C' is always included in a language list.  */
	
	if (!res)
	{
		buf = g_string_new(NULL);
		g_string_sprintf(buf, "gnome/help/%s/C/%s", app, path);
		res = (gchar *)gnome_unconditional_datadir_file(buf->str);
		g_string_free(buf, TRUE);
	}
	
	return res;
}

/**
 * gnome_help_display:
 * @ignore: value of this is ignored.  To simplify hooking into clicked
 *          signals
 * @ref: A GnomeHelpMenuEntry.
 * 
 * Cause a help viewer to display help entry defined in @ref.
 */
void
gnome_help_display (gpointer ignore, const GnomeHelpMenuEntry *ref)
{
	gchar *file, *url;
	
	g_assert(ref != NULL);
	g_assert(ref->path != NULL);
	g_assert(ref->name != NULL);

	file = gnome_help_file_path (ref->name, ref->path);
	
	if (!file)
		return;
	
	url = alloca (strlen (file)+10);
	strcpy (url,"ghelp:");
	strcat (url, file);
	gnome_help_goto (ignore, url);
	g_free (file);
}

/**
 * gnome_help_pbox_display
 * @ignore: ignored.
 * @page_num: The number of the current notebook page in the
 * properties box.
 * @ref: A GnomeHelpMenuEntry
 *
 * Cause a help viewer to display the help entry defined in ref.  This
 * function is meant to be connected to the "help" signal of a
 * GnomePropertyBox.  If ref is { "my-app", "properties-blah" }, and
 * the current page number is 3, then the help viewer will display
 * my-app/lang/properties-blah-3.html, which can be symlinked to the
 * appropriate file.
 */
void
gnome_help_pbox_display (gpointer ignore, gint page_num, const GnomeHelpMenuEntry *ref)
{
	gchar *file, *url, *path;

	g_assert(ref != NULL);
	g_assert(ref->path != NULL);
	g_assert(ref->name != NULL);

	path = g_strdup_printf("%s-%d.html", ref->path, page_num);
	file = gnome_help_file_path (ref->name, path);
	g_free (path);
	
	if (!file)
		return;
	
	url = alloca (strlen (file)+10);
	strcpy (url,"ghelp:");
	strcat (url, file);
	g_free (file);

	gnome_help_goto (ignore, url);
}

/**
 * gnome_help_pbox_goto
 * @ignore: ignored.
 * @ignore2: also ignored.
 * @ref: A GnomeHelpMenuEntry
 *
 * Cause a help viewer to display the help entry defined in @ref.  This
 * function is meant to be connected to the help button of a
 * GnomePropertyBox, when you don't want any munging of the help path
 * done.  Most of the time, you can use gnome_help_pbox_display,
 * though.
 */
void
gnome_help_pbox_goto (gpointer ignore, int ignore2, const GnomeHelpMenuEntry *ref)
{
        gchar *file, *url;

	g_assert(ref != NULL);
	g_assert(ref->path != NULL);
	g_assert(ref->name != NULL);

	file = gnome_help_file_path (ref->name, ref->path);
	if (file == NULL)
	  return;
	
	url = alloca (strlen (file)+10);
	strcpy (url, "ghelp:");
	strcat (url, file);
	g_free (file);
	
	gnome_help_goto (ignore, url);
}

/**
 * gnome_help_goto:
 * @ignore: ignored.
 * @file: file to display in the help browser.
 *
 * Cause a help viewer to display file.
 */
void gnome_help_goto (gpointer ignore, const gchar *file)
{
	
#ifdef GNOME_ENABLE_DEBUG    
	printf("gnome_help_goto: %s\n", (char *)file);
#endif

	gnome_url_show (file);
}
