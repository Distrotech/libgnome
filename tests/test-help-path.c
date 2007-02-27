/* Compile with:  gcc `pkg-config --libs --cflags libgnomeui-2.0` -o test test.c */
/* Add -DWITH_FIX if you want to test my fix for the bug */

/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-help.c
 * Copyright (C) 2001 Sid Vicious
 * Copyright (C) 2001 Jonathan Blandford <jrb@alum.mit.edu>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-help.h>
#include <libgnomeui/gnome-ui-init.h>


static char *
locate_help_file (const char *path, const char *doc_name)
{
	int i, j;
	char *exts[] = { "", ".xml", ".docbook", ".sgml", ".html", NULL };
	const char * const * lang_list = g_get_language_names ();

	for (j = 0;lang_list[j] != NULL; j++) {
		const char *lang = lang_list[j];

		/* This has to be a valid language AND a language with
		 * no encoding postfix.  The language will come up without
		 * encoding next */
		if (lang == NULL ||
		    strchr (lang, '.') != NULL)
			continue;

		for (i = 0; exts[i] != NULL; i++) {
			char *name;
			char *full;

			name = g_strconcat (doc_name, exts[i], NULL);
			full = g_build_filename (path, lang, name, NULL);
			g_free (name);

			if (g_file_test (full, G_FILE_TEST_EXISTS))
				return full;

			g_free (full);
		}
	}

	return NULL;
}

gboolean
help_display_with_doc_id_and_env (GnomeProgram  *program,
				  const char    *doc_id,
				  const char    *file_name,
				  const char    *link_id,
				  char         **envp,
				  GError       **error)
{
	gchar *local_help_path;
	gchar *global_help_path;
	gchar *file, *file2 = NULL;
	struct stat local_help_st;
	struct stat global_help_st;
	gchar *uri;
	gboolean retval;

	g_return_val_if_fail (file_name != NULL, FALSE);

	retval = FALSE;

	local_help_path = NULL;
	global_help_path = NULL;
	file = NULL;
	uri = NULL;

	if (program == NULL)
		program = gnome_program_get ();

#ifdef WITH_FIX
	if (doc_id == NULL)
		g_object_get (program, GNOME_PARAM_APP_ID, &doc_id, NULL);
#endif

	if (doc_id == NULL)
		doc_id = "";

	g_print ("Doc ID: %s\n", doc_id);

	/* Compute the local and global help paths */

	local_help_path = gnome_program_locate_file (program,
						     GNOME_FILE_DOMAIN_APP_HELP,
						     doc_id,
						     FALSE /* only_if_exists */,
						     NULL /* ret_locations */);

	g_print ("Local help path: %s\n", local_help_path);

	if (local_help_path == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_INTERNAL,
			     _("Unable to find the GNOME_FILE_DOMAIN_APP_HELP domain"));
		goto out;
	}

	global_help_path = gnome_program_locate_file (program,
						      GNOME_FILE_DOMAIN_HELP,
						      doc_id,
						      FALSE /* only_if_exists */,
						      NULL /* ret_locations */);

	g_print ("Global help path: %s\n", global_help_path);

	if (global_help_path == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_INTERNAL,
			     _("Unable to find the GNOME_FILE_DOMAIN_HELP domain."));
		goto out;
	}

	/* Try to access the help paths, first the app-specific help path
	 * and then falling back to the global help path if the first one fails.
	 */

	if (stat (local_help_path, &local_help_st) == 0) {
		if (!S_ISDIR (local_help_st.st_mode)) {
			g_set_error (error,
				     GNOME_HELP_ERROR,
				     GNOME_HELP_ERROR_NOT_FOUND,
				     _("Unable to show help as %s is not a directory.  "
				       "Please check your installation."),
				     local_help_path);
			goto out;
		}

		file = locate_help_file (local_help_path, file_name);
	}

	g_print ("Help file found in local path: %s\n", file);

	{
		if (stat (global_help_path, &global_help_st) == 0) {
			if (!S_ISDIR (global_help_st.st_mode)) {
				g_set_error (error,
					     GNOME_HELP_ERROR,
					     GNOME_HELP_ERROR_NOT_FOUND,
					     _("Unable to show help as %s is not a directory.  "
					       "Please check your installation."),
					     global_help_path);
				goto out;
			}
		} else {
			g_set_error (error,
				     GNOME_HELP_ERROR,
				     GNOME_HELP_ERROR_NOT_FOUND,
				     _("Unable to find the help files in either %s "
				       "or %s.  Please check your installation"),
				     local_help_path,
				     global_help_path);
			goto out;
		}

//		if (!(local_help_st.st_dev == global_help_st.st_dev
//		      && local_help_st.st_ino == global_help_st.st_ino))
			file2 = locate_help_file (global_help_path, file_name);

		g_print ("Help file found in global path: %s\n", file2);
		if (file == NULL) file = file2;
	}

	if (file == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_NOT_FOUND,
			     _("Unable to find the help files in either %s "
			       "or %s.  Please check your installation"),
			     local_help_path,
			     global_help_path);
		goto out;
	}

	/* Now that we have a file name, try to display it in the help browser */

	if (link_id)
		uri = g_strconcat ("ghelp://", file, "?", link_id, NULL);
	else
		uri = g_strconcat ("ghelp://", file, NULL);

	g_print ("Help file %s found, starting help viewer\n", uri);
	retval = TRUE;
	//retval = gnome_help_display_uri_with_env (uri, envp, error);

 out:

	g_free (local_help_path);
	g_free (global_help_path);
	g_free (file);
	g_free (uri);

	return retval;
}

int
main (int argc, char **argv)
{
	GnomeProgram *program;
	GSList *helpl = NULL, *apphelpl = NULL, *l;
	GError *error = NULL;

	if (argc != 3) {
		g_print ("Usage: %s <datadir> <program>\n", argv[0]);
		exit (1);
	}

	program = gnome_program_init (argv[2], "0.0.0.0", LIBGNOMEUI_MODULE,
				argc, argv,
				GNOME_PARAM_GOPTION_CONTEXT, g_option_context_new (""),
				GNOME_PARAM_APP_DATADIR, argv[1],
				GNOME_PARAM_NONE);

	help_display_with_doc_id_and_env (NULL, NULL, argv[2], "", NULL, &error);
	if (error != NULL) {
		g_print ("Error showing help: %s\n", error->message);
		g_error_free (error);
	}

	g_object_unref (program);

	exit (0);
}

