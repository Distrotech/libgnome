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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include "gnome-regex.h"
#ifdef NEED_GNOMESUPPORT_H
#include "gnomesupport.h"
#endif
#include "gnome-mime-info.h"

#if !defined getc_unlocked && !defined HAVE_GETC_UNLOCKED
# define getc_unlocked(fp) getc (fp)
#endif

typedef struct {
	char       *mime_type;
	GHashTable *keys;
} GnomeMimeContext;

/* Describes the directories we scan for information */
typedef struct {
	char *dirname;
	struct stat s;
	unsigned int valid : 1;
	unsigned int system_dir : 1;
} mime_dir_source_t;

/* These ones are used to automatically reload mime info on demand */
static mime_dir_source_t gnome_mime_dir, user_mime_dir;
static time_t last_checked;

/* To initialize the module automatically */
static gboolean gnome_mime_inited = FALSE;


static GList *current_lang = NULL;
/* we want to replace the previous key if the current key has a higher
   language level */
static char *previous_key = NULL;
static int previous_key_lang_level = -1;


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

	if (context) {
		g_free (mime_type);
		return context;
	}
	
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
	/*
	 * Destroy it
	 */
	g_hash_table_foreach_remove (context->keys, release_key_and_value, NULL);
	g_hash_table_destroy (context->keys);
	g_free (context->mime_type);
	g_free (context);
}

static void
context_destroy_and_unlink (GnomeMimeContext *context)
{
	/*
	 * Remove the context from our hash tables, we dont know
	 * where it is: so just remove it from both (it can
	 * only be in one).
	 */
	g_hash_table_remove (specific_types, context->mime_type);
	g_hash_table_remove (generic_types, context->mime_type);

	context_destroy (context);
}

/* this gives us a number of the language in the current language list,
   the higher the number the "better" the translation */
static int
language_level (char *lang)
{
	int i;
	GList *li;
	if(!lang) return 0;
	for (i = 1, li = current_lang;
	     li != NULL;
	     i++, li = g_list_next (li)) {
		if(strcmp (li->data,lang)==0)
			return i;
	}
	return -1;
}


static void
context_add_key (GnomeMimeContext *context, char *key, char *lang, char *value)
{
	char *v;
	char *orig_key;
	int lang_level;

	lang_level = language_level(lang);
	/* wrong language completely */
	if (lang_level<0)
		return;

	/* if we have some language defined and
	   if there was a previous_key */
	if (lang_level > 0 &&
	    previous_key) {
		/* if our new key has a better lang_level then remove the
		   previous key */
		if (previous_key_lang_level <= lang_level) {
			if (g_hash_table_lookup_extended (context->keys,
							  previous_key,
							  (gpointer *)&orig_key,
							  (gpointer *)&v)) {
				g_hash_table_remove (context->keys, orig_key);
				g_free(orig_key);
				g_free(v);
			}
		/* else, our language level really sucks and the previous
		   translation was of better language quality so just
		   ignore us */
		} else
			return;
	}

	if (g_hash_table_lookup_extended (context->keys, key,
					  (gpointer *)&orig_key,
					  (gpointer *)&v)) {
		/* if we found it in the database already, just replace it
		   here */
		g_free (v);
		g_hash_table_insert (context->keys, orig_key,
				     g_strdup (value));
	} else {
		g_hash_table_insert (context->keys, g_strdup(key),
				     g_strdup (value));
	}
	/* set this as the previous key */
	g_free(previous_key);
	previous_key = g_strdup(key);
	previous_key_lang_level = lang_level;
}

typedef enum {
	STATE_NONE,
	STATE_LANG,
	STATE_LOOKING_FOR_KEY,
	STATE_ON_MIME_TYPE,
	STATE_ON_KEY,
	STATE_ON_VALUE
} ParserState;

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
	char *lang;
	
	mime_file = fopen (filename, "r");
	if (mime_file == NULL)
		return;

	in_comment = FALSE;
	context_used = FALSE;
	column = 0;
	context = NULL;
	key = NULL;
	lang = NULL;
	line = g_string_sized_new (120);
	state = STATE_NONE;
	
	while ((c = getc_unlocked (mime_file)) != EOF){
		column++;
		if (c == '\r')
			continue;

		if (c == '#' && column == 0){
			in_comment = TRUE;
			continue;
		}
		
		if (c == '\n'){
			in_comment = FALSE;
			column = 0;
			if (state == STATE_ON_MIME_TYPE){

				/* set previous key to nothing
				   for this mime type */
				g_free(previous_key);
				previous_key = NULL;
				previous_key_lang_level = -1;

				context = context_new (line);
				context_used = FALSE;
				g_string_assign (line, "");
				state = STATE_LOOKING_FOR_KEY;
				continue;
			}
			if (state == STATE_ON_VALUE){
				context_used = TRUE;
				context_add_key (context, key, lang, line->str);
				g_string_assign (line, "");
				g_free (key);
				key = NULL;
				g_free (lang);
				lang = NULL;
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
				if (line->str [0]){
					g_free(lang);
					lang = g_strdup(line->str);
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
			context_add_key (context, key, lang, line->str);
		else
			if (!context_used)
				context_destroy_and_unlink (context);
	}

	g_string_free (line, TRUE);
	g_free (key);
	g_free (lang);

	/* free the previous_key stuff */
	g_free(previous_key);
	previous_key = NULL;
	previous_key_lang_level = -1;

	fclose (mime_file);
}

static void
mime_info_load (mime_dir_source_t *source)
{
	DIR *dir;
	struct dirent *dent;
	const int extlen = sizeof (".keys") - 1;
	char *filename;
	
	if (stat (source->dirname, &source->s) != -1)
		source->valid = TRUE;
	else
		source->valid = FALSE;
	
	dir = opendir (source->dirname);
	if (!dir){
		source->valid = FALSE;
		return;
	}
	if (source->system_dir){
		filename = g_concat_dir_and_file (source->dirname, "gnome.keys");
		load_mime_type_info_from (filename);
		g_free (filename);
	}

	while ((dent = readdir (dir)) != NULL){
		
		int len = strlen (dent->d_name);

		if (len <= extlen)
			continue;
		if (strcmp (dent->d_name + len - extlen, ".keys"))
			continue;
		if (source->system_dir && !strcmp (dent->d_name, "gnome.keys"))
			continue;
		if (!source->system_dir && !strcmp (dent->d_name, "user.keys"))
			continue;

		filename = g_concat_dir_and_file (source->dirname, dent->d_name);
		load_mime_type_info_from (filename);
		g_free (filename);
	}
	if (!source->system_dir) {
		filename = g_concat_dir_and_file (source->dirname, "user.keys");
		load_mime_type_info_from (filename);
		g_free (filename);
	}
	closedir (dir);
	
}

static void
load_mime_type_info (void)
{
	mime_info_load (&gnome_mime_dir);
	mime_info_load (&user_mime_dir);
}

static void
gnome_mime_init ()
{
	/*
	 * The hash tables that store the mime keys.
	 */
	specific_types = g_hash_table_new (g_str_hash, g_str_equal);
	generic_types  = g_hash_table_new (g_str_hash, g_str_equal);

	current_lang = gnome_i18n_get_language_list ("LC_MESSAGES");
	if(current_lang)
		current_lang = g_list_reverse(g_list_copy(current_lang));

	/*
	 * Setup the descriptors for the information loading
	 */
	gnome_mime_dir.dirname = gnome_unconditional_datadir_file ("mime-info");
	gnome_mime_dir.system_dir = TRUE;
	
	user_mime_dir.dirname  = g_concat_dir_and_file (gnome_util_user_home (), ".gnome/mime-info");
	user_mime_dir.system_dir = FALSE;

	/*
	 * Load
	 */
	load_mime_type_info ();

	last_checked = time (NULL);
	gnome_mime_inited = TRUE;
}

static gboolean
remove_keys (gpointer key, gpointer value, gpointer user_data)
{
	GnomeMimeContext *context = value;

	context_destroy (context);
	
	return TRUE;
}

static void
maybe_reload (void)
{
	time_t now = time (NULL);
	gboolean need_reload = FALSE;
	struct stat s;
	
	if (last_checked + 5 >= now)
		return;

	if (stat (gnome_mime_dir.dirname, &s) != -1)
		if (s.st_mtime != gnome_mime_dir.s.st_mtime)
			need_reload = TRUE;

	if (stat (user_mime_dir.dirname, &s) != -1)
		if (s.st_mtime != user_mime_dir.s.st_mtime)
			need_reload = TRUE;

	last_checked = now;
	
	if (!need_reload)
		return;

	/* 1. Clean */
	g_hash_table_foreach_remove (specific_types, remove_keys, NULL);
	g_hash_table_foreach_remove (generic_types, remove_keys, NULL);
	
	/* 2. Reload */
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
const char *
gnome_mime_get_value (const char *mime_type, char *key)
{
	char *value, *generic_type, *p;
	GnomeMimeContext *context;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	if (!gnome_mime_inited)
		gnome_mime_init ();

	maybe_reload ();
	
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
 * gnome_mime_get_keys:
 * @mime_type: the mime type to lookup.
 *
 * Returns a GList that contains private strings with all of the keys
 * associated with the @mime_type.  
 */
GList *
gnome_mime_get_keys (const char *mime_type)
{
	char *p, *generic_type;
	GnomeMimeContext *context;
	GList *list = NULL, *l;
	
	g_return_val_if_fail (mime_type != NULL, NULL);

	if (!gnome_mime_inited)
		gnome_mime_init ();

	maybe_reload ();
	
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
				if (strcmp ((char*) this, (char*) m->data) != 0)
					continue;
				list = g_list_remove (list, m->data);
				break;
			}
		}
		l = l->next;
	}
	return list;
}

/**
 * gnome_mime_program:
 * @mime_type: the mime_type 
 *
 * Returns the program intended to be loaded for this given mime-type
 */
const char *
gnome_mime_program (const char *mime_type)
{
	return gnome_mime_get_value (mime_type, "open");
}

/**
 * gnome_mime_description:
 * @mime_type: the mime type
 *
 * Returns the description for this mime-type
 */
const char *
gnome_mime_description (const char *mime_type)
{
	return gnome_mime_get_value (mime_type, "description");
}

/**
 * gnome_mime_test:
 * @mime_type: the mime type
 *
 * Returns the command to be executed on the file before considering
 * the file to match this mime_type.
 */
const char *
gnome_mime_test (const char *mime_type)
{
	return gnome_mime_get_value (mime_type, "test");
}

/**
 * gnome_mime_composetyped:
 * @mime_type: the mime type
 *
 * Returns the command to be executed to compose a message of
 * the given mime_type
 */
const char *
gnome_mime_composetyped (const char *mime_type)
{
	return gnome_mime_get_value (mime_type, "compose");
}

static gboolean
gnome_mime_flag (const char *mime_type, gchar *key, gchar *flag)
{
	const char *str;
	
	str = gnome_mime_get_value (mime_type, key);
	if (str){
		if (strstr (str, flag) != NULL)
			return TRUE;
	}
	return FALSE;
}

/**
 * gnome_mime_copiousoutput:
 * @mime_type: the mime type
 * @key: the key which stores the flags for a command
 *
 * Returns a boolean value, whether the mime_type open
 * command will produce lots of output
 */
gboolean 
gnome_mime_copiousoutput (const char *mime_type, gchar *key)
{
	return gnome_mime_flag (mime_type, key, "copiousoutput");
}

/**
 * gnome_mime_needsterminal
 * @mime_type: the mime type
 * @key: the key which stores the flags for a command
 *
 * Returns a boolean value, whether the mime_type open
 * command will required a terminal.
 */
gboolean
gnome_mime_needsterminal (const char *mime_type, gchar *key)
{
	return gnome_mime_flag (mime_type, key, "needsterminal");
}

#if 0
int
main ()
{
	GList *keys;

	g_warning ("El lookup de image/* fall!\n");
		
	keys = gnome_mime_get_keys ("image/gif");
	printf ("Dumping keys for image/gif\n");
	for (; keys; keys = keys->next){
		char *value;
		
		value = gnome_mime_type_get_value ("image/gif", keys->data);
		printf ("Key=%s, value=%s\n", (char *) keys->data, value);
	}
	
	return 0;
}
#endif
