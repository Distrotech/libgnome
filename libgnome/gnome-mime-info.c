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
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include "gnome-regex.h"
#ifdef NEED_GNOMESUPPORT_H
#include "gnomesupport.h"
#endif
#include "gnome-mime-info.h"

typedef struct {
	char       *mime_type;
	GHashTable *keys;
} GnomeMimeContext;

static char *current_lang;

static gboolean gnome_mime_type_inited = FALSE;

/*
 * A hash table containing all of the Mime records for specific
 * mime types (full description, like image/png)
 */
static GHashTable *specific_types;

/*
 * A hash table containing all of the Mime records for non-specific
 * mime types (like image/\*)
 */
static GHashTable *generic_types;

static GnomeMimeContext *
context_new (GString *str)
{
	GnomeMimeContext *context;
	GHashTable *table;
	char *mime_type, *p;

	mime_type = g_strdup (str->str);
	
	if ((p = strstr (mime_type, "/*")) == NULL){
		table = specific_types;
	} else {
		*(p+1) = 0;
		table = generic_types;
	}
	
	context = g_hash_table_lookup (table, mime_type);

	if (context)
		return context;
	
	context = g_new (GnomeMimeContext, 1);
	context->mime_type = mime_type;
	context->keys = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (table, context->mime_type, context);
	return context;
}

static gboolean
release_key_and_value (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	g_free (value);

	return TRUE;
}

static void
context_destroy (GnomeMimeContext *context)
{
	printf ("destroying: %s\n", context->mime_type);
	/*
	 * Remove the context from our hash tables, we dont know
	 * where it is: so just remove it from both (it can
	 * only be in one).
	 */
	g_hash_table_remove (specific_types, context->mime_type);
	g_hash_table_remove (generic_types, context->mime_type);

	/*
	 * Destroy it
	 */
	g_hash_table_foreach_remove (context->keys, release_key_and_value, NULL);
	g_hash_table_destroy (context->keys);
	g_free (context->mime_type);
	g_free (context);
}

static gboolean
remove_this_key (gpointer key, gpointer value, gpointer user_data)
{
	if (strcmp (key, user_data) == 0){
		g_free (key);
		g_free (user_data);
		return TRUE;
	}

	return FALSE;
}

static void
context_add_key (GnomeMimeContext *context, char *key, char *value)
{
	char *v;

	v = g_hash_table_lookup (context->keys, key);
	if (v)
		g_hash_table_foreach_remove (context->keys, remove_this_key, key);
		
	g_hash_table_insert (context->keys, g_strdup (key), g_strdup (value));
}

typedef enum {
	STATE_NONE,
	STATE_LANG,
	STATE_LOOKING_FOR_KEY,
	STATE_ON_MIME_TYPE,
	STATE_ON_KEY,
	STATE_ON_VALUE
} ParserState;

#define SWITCH_TO_MIME_TYPE() { 

static void
load_mime_type_info_from (char *filename)
{
	FILE *mime_file;
	gboolean in_comment, context_used;
	GString *line;
	int column, c;
	ParserState state;
	GnomeMimeContext *context;
	char *key;
	
	mime_file = fopen (filename, "r");
	if (mime_file == NULL)
		return;

	in_comment = FALSE;
	context_used = FALSE;
	column = 0;
	context = NULL;
	key = NULL;
	line = g_string_sized_new (120);
	state = STATE_NONE;
	
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
				context = context_new (line);
				context_used = FALSE;
				g_string_assign (line, "");
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			if (state == STATE_ON_VALUE){
				context_used = TRUE;
				context_add_key (context, key, line->str);
				g_string_assign (line, "");
				g_free (key);
				key = NULL;
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			continue;
		}

		if (in_comment)
			continue;

		switch (state){
		case STATE_NONE:
			if (c != ' ' && c != '\t')
				state = STATE_ON_MIME_TYPE;
			else
				break;
			/* fall down */
			
		case STATE_ON_MIME_TYPE:
			if (c == ':'){
				in_comment = TRUE;
				break;
			}
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
				state = STATE_ON_MIME_TYPE;
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
				} else {
					in_comment = TRUE;
					state = STATE_LOOKING_FOR_KEY;
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
			context_add_key (context, key, line->str);
		else
			if (!context_used)
				context_destroy (context);
	}

	g_string_free (line, TRUE);
	if (key)
		g_free (key);

	fclose (mime_file);
}

static void
load_mime_type_info_from_dir (char *mime_info_dir)
{
	DIR *dir;
	struct dirent *dent;
	const int extlen = sizeof (".keys") - 1;
	
	dir = opendir (mime_info_dir);
	if (!dir)
		return;

	while ((dent = readdir (dir)) != NULL){
		char *filename;
		
		int len = strlen (dent->d_name);

		if (len <= extlen)
			continue;
		
		if (strcmp (dent->d_name + len - extlen, ".keys"))
			continue;

		filename = g_concat_dir_and_file (mime_info_dir, dent->d_name);
		load_mime_type_info_from (filename);
		g_free (filename);
	}
	closedir (dir);
}

static void
load_mime_type_info (void)
{
	char *mime_info_dir;

	current_lang = getenv ("LANG");
	
	mime_info_dir = gnome_unconditional_datadir_file ("mime-info");	
	load_mime_type_info_from_dir (mime_info_dir);
	g_free (mime_info_dir);

	mime_info_dir = g_concat_dir_and_file (gnome_util_user_home (), ".gnome/mime-info");
	load_mime_type_info_from_dir (mime_info_dir);
	g_free (mime_info_dir);
}

static void
gnome_mime_type_init ()
{
	specific_types = g_hash_table_new (g_str_hash, g_str_equal);
	generic_types  = g_hash_table_new (g_str_hash, g_str_equal);
	load_mime_type_info ();
}

/**
 * gnome_mime_context_get_value:
 * @mime_type: a mime type.
 * @key: A key to lookup for the given mime-type
 *
 * This function retrieves the value associated with @key in 
 * the given GnomeMimeContext.  The string is private, you
 * should not free the result.
 */
char *
gnome_mime_type_get_value (char *mime_type, char *key)
{
	char *value, *generic_type, *p;
	GnomeMimeContext *context;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	if (!gnome_mime_type_inited){
		gnome_mime_type_init ();
		gnome_mime_type_inited = TRUE;
	}

	context = g_hash_table_lookup (specific_types, mime_type);
	if (context){
		value = g_hash_table_lookup (context->keys, key);

		if (value)
			return value;
	}

	generic_type = g_strdup (mime_type);
	p = strchr (generic_type, '/');
	if (p)
		*(p+1) = 0;
	
	context = g_hash_table_lookup (generic_types, generic_type);
	g_free (generic_type);
	
	if (context){
		value = g_hash_table_lookup (context->keys, key);
		if (value)
			return value;
	}

	return NULL;
}

static void
assemble_list (gpointer key, gpointer value, gpointer user_data)
{
	GList **listp = user_data;

	(*listp) = g_list_prepend ((*listp), key);
}

/**
 * gnome_mime_type_get_keys:
 * @mime_type: the mime type to lookup.
 *
 * Returns a GList that contains private strings with all of the keys
 * associated with the @mime_type.  
 */
GList *
gnome_mime_type_get_keys (char *mime_type)
{
	char *p, *generic_type;
	GnomeMimeContext *context;
	GList *list = NULL, *l;
	
	g_return_val_if_fail (mime_type != NULL, NULL);

	if (!gnome_mime_type_inited){
		gnome_mime_type_init ();
		gnome_mime_type_inited = TRUE;
	}

	generic_type = g_strdup (mime_type);
	p = strchr (generic_type, '/');
	if (p)
		*(p+1) = 0;
	
	context = g_hash_table_lookup (generic_types, generic_type);
	g_free (generic_type);
	if (context){
		g_hash_table_foreach (
			context->keys, assemble_list, &list);
	}

	context = g_hash_table_lookup (specific_types, mime_type);
	if (context){
		g_hash_table_foreach (
			context->keys, assemble_list, &list);
	}

	for (l = list; l;){
		if (l->next){
			void *this = l->data;
			GList *m;

			for (m = l->next; m; m = m->next){
				if (strcmp (this, m->data) != 0)
					continue;
				list = g_list_remove (list, m->data);
				break;
			}
		}
		l = l->next;
	}
	return list;
}

#if 0
int
main ()
{
	GList *keys;

	g_warning ("El lookup de image/* fall!\n");
		
	keys = gnome_mime_type_get_keys ("image/gif");
	printf ("Dumping keys for image/gif\n");
	for (; keys; keys = keys->next){
		char *value;
		
		value = gnome_mime_type_get_value ("image/gif", keys->data);
		printf ("Key=%s, value=%s\n", (char *) keys->data, value);
	}
	
	return 0;
}
#endif
