/* 
 * Copyright 1997 Paolo Molaro
 * Copyright 1998 Miguel de Icaza.
 *
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <gtk/gtk.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include <string.h>
#include <ctype.h>

static GHashTable *mime_extensions [2] = { NULL, NULL };
static GList      *mime_regexs     [2] = { NULL, NULL };

typedef struct {
	regex_t regex;
	char    *mime_type;
} RegexMimePair;

static gboolean module_inited = FALSE;

static char *
get_priority (char *def, int *priority)
{
	*priority = 0;
	
	if (*def == ','){
		def++;
		if (*def == '1'){
			*priority = 0;
			def++;
		} else if (*def == '2'){
			*priority = 1;
			def++;
		}
	}

	while (*def && *def == ':')
		def++;

	return def;
}

static void
add_to_key (char *mime_type, char *def)
{
	int priority = 1;
	char *s, *p, *ext;
	int used;
	
	if (strncmp (def, "ext", 3) == 0){
		def += 3;
		def = get_priority (def, &priority);
		s = p = g_strdup (def);

		used = 0;
		
		while ((ext = strtok (s, " \t\n\r,")) != NULL){
			g_hash_table_insert (mime_extensions [priority], ext, mime_type);
			used = 1;
			s = NULL;
		}
		if (!used)
			g_free (p);
	}

	if (strncmp (def, "regex", 5) == 0){
		RegexMimePair *mp;
		def += 5;
		def = get_priority (def, &priority);

		while (*def && isspace (*def))
			def++;

		if (!*def)
			return;
		
		mp = g_new (RegexMimePair, 1);
		if (regcomp (&mp->regex, def, REG_EXTENDED | REG_NOSUB)){
			g_free (mp);
			return;
		}
		mp->mime_type = mime_type;
		mime_regexs [priority] = g_list_prepend (mime_regexs [priority], mp);
	}
}

static void
mime_fill_from_file (char *filename)
{
	FILE *f;
	char buf [1024];
	char *current_key;
	gboolean used;
	
	g_assert (filename != NULL);

	f = fopen (filename, "r");
	if (!f)
		return;

	current_key = NULL;
	used = FALSE;
	while (fgets (buf, sizeof (buf), f)){
		char *p;

		if (buf [0] == '#')
			continue;

		/* Trim trailing spaces */
		for (p = buf + strlen (buf) - 1; p >= buf; p--){
			if (isspace (*p) || *p == '\n')
				*p = 0;
			else
				break;
		}
		
		if (!buf [0])
			continue;
		
		if (buf [0] == '\t' || buf [0] == ' '){
			if (current_key){
				char *p = buf;

				while (*p && isspace (*p))
					p++;

				if (*p == 0)
					continue;
				
				add_to_key (current_key, p);
				used = TRUE;
			}
		} else {
			if (!used && current_key)
				g_free (current_key);
			current_key = g_strdup (buf);
			used = FALSE;
		}
	}
	fclose (f);
}

static void
mime_load_from_dir (char *mime_info_dir)
{
	DIR *dir;
	struct dirent *dent;
	const int extlen = sizeof (".mime") - 1;
	
	dir = opendir (mime_info_dir);
	if (!dir)
		return;

	while ((dent = readdir (dir)) != NULL){
		char *filename;
		
		int len = strlen (dent->d_name);

		if (len <= extlen)
			continue;
		
		if (strcmp (dent->d_name + len - extlen, ".mime"))
			continue;

		filename = g_concat_dir_and_file (mime_info_dir, dent->d_name);
		mime_fill_from_file (filename);
		g_free (filename);
	}
	closedir (dir);
}

static void
mime_init (void)
{
	char *mime_info_dir;
	
	mime_extensions [0] = g_hash_table_new (g_str_hash, g_str_equal);
	mime_extensions [1] = g_hash_table_new (g_str_hash, g_str_equal);

	mime_info_dir = gnome_unconditional_datadir_file ("mime-info");
	mime_load_from_dir (mime_info_dir);
	g_free (mime_info_dir);

	mime_info_dir = g_concat_dir_and_file (gnome_util_user_home (), ".gnome/mime-info");
	mime_load_from_dir (mime_info_dir);
	g_free (mime_info_dir);

	module_inited = TRUE;
}

/**
 * gnome_mime_type_or_default:
 * @filename: A filename (the file does not necesarily exist).
 * @defaultv: A default value to be returned if no match is found
 *
 * This routine tries to determine the mime-type of the filename
 * only by looking at the filename from the GNOME database of mime-types.
 *
 * Returns the mime-type of the @filename.  If no value could not be
 * determined, it will return @defaultv.
 */
gchar *
gnome_mime_type_or_default (gchar * filename, gchar * defaultv)
{
	gchar *ext;
	gchar *result;
	int priority;
	
	if (!filename)
		return defaultv;
	ext = strrchr (filename, '.');
	if (!ext)
		ext = filename;
	else
		++ext;

	if (!module_inited)
		mime_init ();

	for (priority = 1; priority >= 0; priority--){
		GList *l;
		char *res;
		
		res = g_hash_table_lookup (mime_extensions [priority], ext);
		if (res)
			return res;

		for (l = mime_regexs [priority]; l; l = l->next){
			RegexMimePair *mp = l->data;
			
			if (regexec (&mp->regex, filename, 0, 0, 0) == 0)
				return mp->mime_type;
		}
	}

	return defaultv;
}

/**
 * gnome_mime_type:
 * @filename: A filename (the file does not necesarily exist).
 *
 * Determined the mime type for @filename.
 *
 * Returns the mime-type for this filename.
 */
gchar *
gnome_mime_type (gchar * filename)
{
	return gnome_mime_type_or_default (filename, "text/plain");
}

/**
 * gnome_mime_type_or_default_of_file:
 * @existing_filename: A filename that points to an existing filename.
 * @defaultv: A default value to be returned if no match is found
 *
 * This routine tries to determine the mime-type of the filename
 * by trying to guess the content of the file.  If this fails, it will
 * return the mime-type based only on the filename.
 *
 * Returns the mime-type of the @existing_filename.  If no value could not be
 * determined, it will return @defaultv.
 */
gchar *
gnome_mime_type_or_default_of_file (char *existing_filename, gchar *defaultv)
{
	char *mime_type;

	mime_type = gnome_mime_type_from_magic (existing_filename);
	if (mime_type)
		return mime_type;
	
	return gnome_mime_type_or_default (existing_filename, defaultv);
}

/**
 * gnome_mime_type_of_file:
 * @existing_filename: A filename pointing to an existing file.
 *
 * Determined the mime type for @existing_filename.  It will try
 * to figure this out by looking at the contents of the file, if this
 * fails it will use the filename to figure out a name.
 *
 * Returns the mime-type for this filename.
 */
gchar *
gnome_mime_type_of_file (char *existing_filename)
{
	char *mime_type;

	mime_type = gnome_mime_type_from_magic (existing_filename);
	if (mime_type)
		return mime_type;

	return gnome_mime_type_or_default_of_file (existing_filename, "text/plain");
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList*        
gnome_uri_list_extract_uris (gchar* uri_list)
{
	char *p, *q;
	char *retval;
	GList *result = NULL;
	
	g_return_val_if_fail (uri_list != NULL, NULL);

	p = uri_list;
	
	/* We don't actually try to validate the URI according to RFC
	 * 2396, or even check for allowed characters - we just ignore
	 * comments and trim whitespace off the ends.  We also
	 * allow LF delimination as well as the specified CRLF.
	 */
	while (p) {
		if (*p != '#') {
			while (isspace(*p))
				p++;
			
			q = p;
			while (*q && (*q != '\n') && (*q != '\r'))
				q++;
		  
			if (q > p) {
			        q--;
				while (q > p && isspace(*q))
					q--;
				
				retval = g_malloc (q - p + 2);
				strncpy (retval, p, q - p + 1);
				retval[q - p + 1] = '\0';
				
				result = g_list_prepend (result, retval);
			}
		}
		p = strchr (p, '\n');
		if (p)
			p++;
	}
	
	return g_list_reverse (result);
}

/**
 * gnome_uri_list_extract_filenames:
 * @uri_list: an uri-list in the standard format
 *
 * Returns a GList containing strings allocated with g_malloc
 * that contain the filenames in the uri-list.
 *
 * Note that unlike gnome_uri_list_extract_uris() function, this
 * will discard any non-file uri from the result value.
 */
GList*        
gnome_uri_list_extract_filenames (gchar* uri_list)
{
	GList *tmp_list, *node, *result;
	
	g_return_val_if_fail (uri_list != NULL, NULL);
	
	result = gnome_uri_list_extract_uris (uri_list);

	tmp_list = result;
	while (tmp_list) {
		gchar *s = tmp_list->data;
		
		node = tmp_list;
		tmp_list = tmp_list->next;

		if (!strncmp (s, "file:", 5)) {
			node->data = g_strdup (s+5);
		} else {
			result = g_list_remove_link(result, node);
			g_list_free_1 (node);
		}
		g_free (s);
	}
	return result;
}

/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void          
gnome_uri_list_free_strings      (GList *list)
{
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

