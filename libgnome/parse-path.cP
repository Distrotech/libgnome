/* This is a -*- C -*- file fragment.  Please don't compile it, however.  */

typedef struct {
	char *file, *section, *key, *def;
	char *path, *opath;
} ParsedPath;

static void
release_path (ParsedPath *p)
{
	g_free (p->file);
	g_free (p->opath);
	g_free (p);
}

static ParsedPath *
parse_path (const char *path, gint priv)
{
	ParsedPath *p = g_malloc (sizeof (ParsedPath));

	g_assert(path != NULL);
	
	if (*path == '/' || prefix == NULL)
		p->opath = g_strdup (path);
	else
		p->opath = g_copy_strings (prefix, path, NULL);

	p->path = p->opath;

	if (*p->path == '='){
		/* If it is an absolute path name */
		p->path++;
		p->file    = g_strdup (strtok (p->path, "="));
		p->section = strtok (NULL, "/=");
		p->key     = strtok (NULL, "=");
		p->def     = strtok (NULL, "=");
	} else {
		char *end;

		p->file    = p->path;
		p->def     = NULL;
		p->section = NULL;
		p->key     = NULL;
		if ((end = strchr (p->path, '='))) {
			*end = 0;
			p->def = end + 1;
		} else 
			end = p->path + strlen (p->path);

		/* Look backwards for a slash, to split key from the filename/section */
		while (end > p->path){
			end--;
			if (*end == '/'){
				*end = 0;
				p->key = end + 1;
				break;
			}
		}

		/* Look backwards for the next slash, to get the section name */
		while (end > p->path){
			end--;
			if (*end == '/'){
				*end = 0;
				p->section = end + 1;
				break;
			}
		}
		if (*p->file == '/')
			p->file++;

		if (priv){
			p->file = g_concat_dir_and_file (gnome_user_private_dir,
							 p->file);
		} else {
			p->file = g_concat_dir_and_file (gnome_user_dir,
							 p->file);
		}
	}
	return p;
}
