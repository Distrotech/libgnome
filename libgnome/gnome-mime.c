/*
 * Copyright 1997 Paolo Molaro
 * Copyright 1998 Miguel de Icaza.
 *
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <gtk/gtk.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include <string.h>
#include <ctype.h>

static gboolean module_inited = FALSE;

static GHashTable *mime_extensions [2] = { NULL, NULL };
static GList      *mime_regexs     [2] = { NULL, NULL };

typedef struct {
	char    *mime_type;
	regex_t regex;
} RegexMimePair;

typedef struct {
	char *dirname;
	struct stat s;
	unsigned int valid : 1;
	unsigned int system_dir : 1;
} mime_dir_source_t;

/* chached localhostname */
static char localhostname[1024];
static gboolean got_localhostname = FALSE;

/* These ones are used to automatically reload mime-types on demand */
static mime_dir_source_t gnome_mime_dir, user_mime_dir;
static time_t last_checked;

#ifdef G_THREADS_ENABLED

/* We lock this mutex whenever we modify global state in this module.  */
G_LOCK_DEFINE_STATIC (mime_mutex);

#endif /* G_LOCK_DEFINE_STATIC */



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

static gint
list_find_type (gconstpointer value, gconstpointer type)
{
	return (g_strcasecmp( (const gchar*) value, (const gchar*) type ));
}

static void
add_to_key (char *mime_type, char *def)
{
	int priority = 1;
	char *s, *p, *ext;
	GList *list = NULL;

	if (strncmp (def, "ext", 3) == 0){
		char *tokp;

		def += 3;
		def = get_priority (def, &priority);
		s = p = g_strdup (def);

		while ((ext = strtok_r (s, " \t\n\r,", &tokp)) != NULL){
			list = (GList*) g_hash_table_lookup (mime_extensions [priority], ext);
		    if (!g_list_find_custom (list, (gpointer) mime_type, list_find_type)) {
				list = g_list_prepend (list, g_strdup (mime_type));
				g_hash_table_insert (mime_extensions [priority], g_strdup (ext), list);
			}
			s = NULL;
		}
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
		mp->mime_type = g_strdup (mime_type);

		mime_regexs [priority] = g_list_prepend (mime_regexs [priority], mp);
	}
}

static void
mime_fill_from_file (const char *filename)
{
	FILE *f;
	char buf [1024];
	char *current_key;

	g_assert (filename != NULL);

	f = fopen (filename, "r");

	if (!f)
		return;

	current_key = NULL;
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
			}
		} else {
			if (current_key)
				g_free (current_key);

			current_key = g_strdup (buf);
			if (current_key [strlen (current_key)-1] == ':')
				current_key [strlen (current_key)-1] = 0;
		}
	}

	if (current_key)
		g_free (current_key);

	fclose (f);
}

static void
mime_load (mime_dir_source_t *source)
{
	DIR *dir;
	struct dirent *dent;
	const int extlen = sizeof (".mime") - 1;
	char *filename;

	g_return_if_fail (source != NULL);
	g_return_if_fail (source->dirname != NULL);

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
		filename = g_concat_dir_and_file (source->dirname, "gnome.mime");
		mime_fill_from_file (filename);
		g_free (filename);
	}

	while ((dent = readdir (dir)) != NULL){

		int len = strlen (dent->d_name);

		if (len <= extlen)
			continue;
		if (strcmp (dent->d_name + len - extlen, ".mime"))
			continue;

		if (source->system_dir && !strcmp (dent->d_name, "gnome.mime"))
			continue;
		if (!source->system_dir && !strcmp (dent->d_name, "user.mime"))
			continue;

		filename = g_concat_dir_and_file (source->dirname, dent->d_name);

		mime_fill_from_file (filename);
		g_free (filename);
	}
	closedir (dir);

	if (!source->system_dir) {
		filename = g_concat_dir_and_file (source->dirname, "user.mime");
		mime_fill_from_file (filename);
		g_free (filename);
	}
}

static gboolean
mime_hash_func (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	g_list_free (value);

	return TRUE;
}

static void
maybe_reload (void)
{
	time_t now = time (NULL);
	gboolean need_reload = FALSE;
	struct stat s;
	int i;

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

	for (i = 0; i < 2; i++){
		GList *l;

		g_hash_table_foreach_remove (mime_extensions [i], mime_hash_func, NULL);

		for (l = mime_regexs [i]; l; l = l->next){
			RegexMimePair *mp = l->data;

			g_free (mp->mime_type);
			regfree (&mp->regex);
			g_free (mp);
		}
		g_list_free (mime_regexs [i]);
		mime_regexs [i] = NULL;
	}

	mime_load (&gnome_mime_dir);
	mime_load (&user_mime_dir);

	last_checked = time (NULL);
}

static void
mime_init (void)
{
	mime_extensions [0] = g_hash_table_new (g_str_hash, g_str_equal);
	mime_extensions [1] = g_hash_table_new (g_str_hash, g_str_equal);

	gnome_mime_dir.dirname = gnome_unconditional_datadir_file ("mime-info");
	gnome_mime_dir.system_dir = TRUE;

	user_mime_dir.dirname  = gnome_util_home_file ("mime-info");
	user_mime_dir.system_dir = FALSE;
	mime_load (&gnome_mime_dir);
	mime_load (&user_mime_dir);

	last_checked = time (NULL);
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
 * Returns the mime-type of the @filename.  If no value could be
 * determined, it will return @defaultv.
 */
const char *
gnome_mime_type_or_default (const gchar *filename, const gchar *defaultv)
{
	const gchar *ext;
	int priority;
	const gchar *result = defaultv;

	G_LOCK (mime_mutex);

	if (!filename)
		goto done;
	ext = strrchr (filename, '.');
	if (ext)
		++ext;

	if (!module_inited)
		mime_init ();

	maybe_reload ();

	for (priority = 1; priority >= 0; priority--){
		GList *l;
		GList *list = NULL ;
		
		if (ext){
			
			list = g_hash_table_lookup (mime_extensions [priority], ext);
			if (list) {
				list = g_list_first( list );
				result = (gchar *) list->data;
				goto done;
			}
		}

		for (l = mime_regexs [priority]; l; l = l->next){
			RegexMimePair *mp = l->data;

			if (regexec (&mp->regex, filename, 0, 0, 0) == 0) {
				result = mp->mime_type;
				goto done;
			}
		}
	}

 done:
	G_UNLOCK (mime_mutex);
	return result;
}

/**
 * gnome_mime_type_list_or_default:
 * @filename: A filename (the file does not necessarily exist).
 * @defaultv: A default value to be returned if no match is found
 *
 * This routine tries to determine the mime-type(s) of the filename
 * only by looking at the filename from the GNOME database of mime-types.
 *
 * Returns a GList that contains private strings with the mime-type(s)
 * associated with the @filename.  If no value could be determined,
 * it will return @defaultv.
 */
 
GList*
gnome_mime_type_list_or_default (const gchar *filename, const gchar *defaultv)
{
	const gchar *ext;
	int priority;
	GList *result = NULL;

	G_LOCK (mime_mutex);

	if (!filename)
		goto done;
	ext = strrchr (filename, '.');
	if (ext)
		++ext;

	if (!module_inited)
		mime_init ();

	maybe_reload ();

	for (priority = 1; priority >= 0; priority--){
		GList *l;
		GList *list = NULL, *resext = NULL, *resreg = NULL;
		
		if (ext){
			
			list = (GList*) g_hash_table_lookup (mime_extensions [priority], ext);
			if (list) {
				resext = g_list_reverse (g_list_copy (g_list_first(list)));
			}
		}
		
		for (l = mime_regexs [priority]; l; l = l->next){
			RegexMimePair *mp = l->data;

			if (regexec (&mp->regex, filename, 0, 0, 0) == 0) {
				resreg = g_list_prepend (resreg, mp->mime_type);
			}
		}
		result = g_list_concat (result, resext);
		result = g_list_concat (result, resreg);
		resext = resreg = NULL;
	}

 done:
	G_UNLOCK (mime_mutex);
	return result;
}

/**
 * gnome_mime_type:
 * @filename: A filename (the file does not necessarily exist).
 *
 * Determined the mime type for @filename.
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_mime_type (const gchar * filename)
{
	return gnome_mime_type_or_default (filename, "text/plain");
}

/**
 * gnome_mime_type_list:
 * @filename: A filename (the file does not necessarily exist).
 *
 * Determined the mime type(s) for @filename.
 *
 * Returns a GList that contains private strings with the mime-type(s)
 * associated with the @filename.
 */
GList *
gnome_mime_type_list (const gchar * filename)
{
	return gnome_mime_type_list_or_default (filename, "text/plain");
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
 * Returns the mime-type of the @existing_filename.  If no value could be
 * determined, it will return @defaultv.
 */
const char *
gnome_mime_type_or_default_of_file (const char *existing_filename,
				    const gchar *defaultv)
{
	char *mime_type;

	mime_type = (char *)gnome_mime_type_from_magic (existing_filename);
	if (mime_type)
		return mime_type;

	return gnome_mime_type_or_default (existing_filename, defaultv);
}

/**
 * gnome_mime_type_list_or_default_of_file:
 * @existing_filename: A filename that points to an existing filename.
 * @defaultv: A default value to be returned if no match is found
 *
 * This routine tries to determine the mime-type(s) of the filename
 * by trying to guess the contents of the file.  If this fails, it will
 * return the mime-type based only on the filename.
 *
 * Returns a GList that contains private strings with the mime-type(s)
 * associated with the @existing_filename.  If no value could be determined,
 * it will return @defaultv.
 */
GList *
gnome_mime_type_list_or_default_of_file (const char *existing_filename,
				    const gchar *defaultv)
{
	char *mime_type;
	GList *mime_list = NULL;

	mime_type = (char *)gnome_mime_type_from_magic (existing_filename);
	if (mime_type) {
		mime_list = g_list_prepend (mime_list, mime_type);
		return mime_list;
	}

	return gnome_mime_type_list_or_default (existing_filename, defaultv);
}

/**
 * gnome_mime_type_of_file:
 * @existing_filename: A filename pointing to an existing file.
 *
 * Determined the mime type for @existing_filename.  It will try
 * to figure this out by looking at the contents of the file; if this
 * fails it will use the filename to figure out a name.
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_mime_type_of_file (const char *existing_filename)
{
	return gnome_mime_type_or_default_of_file (existing_filename, "text/plain");
}

/**
 * gnome_mime_type_list_of_file:
 * @existing_filename: A filename pointing to an existing file.
 *
 * Determined the mime type(s) for @existing_filename.  It will try
 * to figure this out by looking at the contents of the file; if this
 * fails it will use the filename to figure out a name.
 *
 * Returns a GList that contains private strings with the mime-type(s)
 * associated with the @existing_filename.
 */
GList *
gnome_mime_type_list_of_file (const char *existing_filename)
{
	return gnome_mime_type_list_or_default_of_file (existing_filename, "text/plain");
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Extract URIs from a @uri-list and return a list
 *
 * Returns: a GList containing strings allocated with g_malloc.
 * You should use #gnome_uri_list_free_strings to free the
 * returned list
 */
GList*
gnome_uri_list_extract_uris (const gchar* uri_list)
{
	const gchar *p, *q;
	gchar *retval;
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
 * Extract local files from a @uri-list and return a list.  Note
 * that unlike the #gnome_uri_list_extract_uris function, this
 * will only return local files and not any other urls.
 *
 * Returns: a GList containing strings allocated with g_malloc.
 * You should use #gnome_uri_list_free_strings to free the
 * returned list.
 */
GList*
gnome_uri_list_extract_filenames (const gchar* uri_list)
{
	GList *tmp_list, *node, *result;

	g_return_val_if_fail (uri_list != NULL, NULL);

	result = gnome_uri_list_extract_uris (uri_list);

	tmp_list = result;
	while (tmp_list) {
		gchar *s = tmp_list->data;

		node = tmp_list;
		tmp_list = tmp_list->next;

		node->data = gnome_uri_extract_filename(s);

		/* if we didn't get anything, just remove the element */
		if (!node->data) {
			result = g_list_remove_link(result, node);
			g_list_free_1 (node);
		}
		g_free (s);
	}
	return result;
}

/**
 * gnome_uri_extract_filename:
 * @uri: a single URI
 *
 * If the @uri is a local file, return the local filename only
 * without the file:[//hostname] prefix.
 *
 * Returns: a newly allocated string if the @uri, or %NULL
 * if the @uri was not a local file
 */
gchar *
gnome_uri_extract_filename (const gchar* uri)
{
	/* file uri with a hostname */
	if (strncmp(uri, "file://", strlen("file://"))==0) {
		char *hostname = g_strdup(&uri[strlen("file://")]);
		char *p = strchr(hostname,'/');
		char *path;
		/* if we can't find the '/' this uri is bad */
		if(!p) {
			g_free(hostname);
			return NULL;
		}
		/* if no hostname */
		if(p==hostname)
			return hostname;

		path = g_strdup(p);
		*p = '\0';

		/* gel local host name and cache it */
		if(!got_localhostname) {
			G_LOCK (mime_mutex);
			if(gethostname(localhostname,
				       sizeof(localhostname)) < 0) {
				strcpy(localhostname,"");
			}
			got_localhostname = TRUE;
			G_UNLOCK (mime_mutex);
		}

		/* if really local */
		if((localhostname[0] &&
		    g_strcasecmp(hostname,localhostname)==0) ||
		   g_strcasecmp(hostname,"localhost")==0) {
			g_free(hostname);
			return path;
		}
		
		g_free(hostname);
		g_free(path);
		return NULL;

	/* if the file doesn't have the //, we take it containing 
	   a local path */
	} else if (strncmp(uri, "file:", strlen("file:"))==0) {
		const char *path = &uri[strlen("file:")];
		/* if empty bad */
		if(!*path) return NULL;
		return g_strdup(path);
	}
	return NULL;
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
