/* gnome-mime-info.c - GNOME mime-information implementation.

   Copyright (C) 1998 Miguel de Icaza

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <config.h>
#include <glib.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include "gnome-regex.h"
#ifdef NEED_GNOMESUPPORT_H
#include "gnomesupport.h"
#endif
#include "gnome-mime-info.h"

static char *current_lang;


static GnomeMimeContext *
gnome_mime_type_context_new (GString *str)
{
	GnomeMimeContext *context;

	context = g_new (GnomeMimeContext, 1);
	context->mime_type = g_strdup (str->str);
	context->keys = g_hash_table_new (g_str_hash, g_str_equal);

	return context;
}

static void
gnome_mime_type_context_destroy (GnomeMimeContext *context)
{
	g_hash_table_destroy (context->keys);
	g_free (context->mime_type);
	g_free (context);
}

static void
gnome_mime_type_context_add (GnomeMimeContext *context, char *key, char *value)
{
	printf ("%s\n", context->mime_type);
	printf ("\tAdding [%s]=[%s]\n", key, value);
}

typedef enum {
	STATE_NONE,
	STATE_LANG,
	STATE_LOOKING_FOR_KEY,
	STATE_ON_MIME_TYPE,
	STATE_ON_KEY,
	STATE_ON_VALUE
} ParserState;

#define SWITCH_TO_MIME_TYPE() { state = STATE_ON_MIME_TYPE; seen_sep = global_match = FALSE; }

static void
load_mime_type_info_from (char *filename)
{
	FILE *mime_file;
	gboolean in_comment, global_match, seen_sep;
	GString *line;
	int column, c;
	ParserState state;
	GnomeMimeContext *context;
	char *key;
	
	mime_file = fopen (filename, "r");
	if (mime_file == NULL)
		return;

	in_comment = FALSE;
	column = 0;
	context = NULL;
	line = g_string_sized_new (120);
	state = STATE_NONE;
	key = NULL;
	
	while ((c = getc (mime_file)) != EOF){
		column++;
		if (c == '\r')
			continue;

		if (c == '#'){
			in_comment = TRUE;
			continue;
		}
		
		if (c == '\n'){
			in_comment = FALSE;
			column = 0;
			if (state == STATE_ON_MIME_TYPE){
				context = gnome_mime_type_context_new (line);
				g_string_assign (line, "");
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			if (state == STATE_ON_VALUE){
				gnome_mime_type_context_add (context, key, line->str);
				g_string_assign (line, "");
				state = STATE_LOOKING_FOR_KEY;
				g_free (key);
				key = NULL;
				continue;
			}
			continue;
		}

		if (in_comment)
			continue;

		switch (state){
		case STATE_NONE:
			if (c != ' ' && c != '\t'){
				SWITCH_TO_MIME_TYPE();
			} else
				break;
			/* fall down */
			
		case STATE_ON_MIME_TYPE:
			if (c == '/')
				seen_sep = TRUE;
			if (seen_sep && c == '*')
				global_match = TRUE;
			
			g_string_append_c (line, c);
			break;

		case STATE_LOOKING_FOR_KEY:
			if (c == '\t' || c == ' ')
				break;

			if (c == '['){
				state = STATE_LANG;
				break;
			}

			if (column == 1){
				SWITCH_TO_MIME_TYPE ();
				g_string_append_c (line, c);
				break;
			}
			state = STATE_ON_KEY;
			/* falldown */

		case STATE_ON_KEY:
			if (c == '\\'){
				c = getc (mime_file);
				if (c == EOF)
					break;
			}
			if (c == '='){
				key = g_strdup (line->str);
				g_string_assign (line, "");
				state = STATE_ON_VALUE;
				break;
			}
			g_string_append_c (line, c);
			break;

		case STATE_ON_VALUE:
			g_string_append_c (line, c);
			break;
			
		case STATE_LANG:
			if (c == ']'){
				state = STATE_ON_KEY;      
				if (current_lang && line->str [0]){
					if (strcmp (current_lang, line->str) != 0){
						in_comment = TRUE;
						state = STATE_LOOKING_FOR_KEY;
					}
				}
				g_string_assign (line, "");
				break;
			}
			g_string_append_c (line, c);
			break;
		}
	}

	if (context){
		if (key && line->str [0])
			gnome_mime_type_context_add (context, key, line->str);
		else
			gnome_mime_type_context_destroy (context);
	}

	g_string_free (line, TRUE);
	if (key)
		g_free (key);

	fclose (mime_file);
}

static void
load_mime_type_info (void)
{
	char *mime_info_dir;

	current_lang = getenv ("LANG");
	
	mime_info_dir = gnome_unconditional_datadir_file ("mime-info");	
	load_mime_type_info_from (mime_info_dir);
	g_free (mime_info_dir);

	mime_info_dir = g_concat_dir_and_file (gnome_util_user_home (), ".gnome/mime-info");
	load_mime_type_info_from (mime_info_dir);
	g_free (mime_info_dir);
}

GnomeMimeContext *
gnome_mime_type_get_info (char *mime_type)
{
	g_return_if_fail (mime_type != NULL);
	
}

main ()
{
	load_mime_type_info ();
}
