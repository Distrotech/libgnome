/*
 * Configuration-File Functions.
 *
 *  Copyright 1993, 1994, 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>	/* atoi() */
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include "libgnomeP.h"

#ifndef HAVE_STRNDUP
/* Like strdup, but only copy N chars.  */
extern char *strndup (const char *s, size_t n);
#endif

#define STRSIZE 4096
#define overflow (next == &CharBuffer [STRSIZE-1])

enum {
	FirstBrace,
	OnSecHeader,
	IgnoreToEOL,
	KeyDef,
	KeyDefOnKey,
	KeyValue
};

typedef struct {
	int type;
	void *value;
} iterator_type;

typedef enum {
	LOOKUP,
	SET
} access_type;

typedef struct TKeys {
	char *key_name;
	char *value;
	struct TKeys *link;
} TKeys;

typedef struct TSecHeader {
	char *section_name;
	TKeys *keys;
	struct TSecHeader *link;
} TSecHeader;

typedef struct TProfile {
	char *filename;
	time_t mtime;
	TSecHeader *section;
	struct TProfile *link;
} TProfile;

typedef struct {
	char *file, *section, *key, *def;
	char *path, *opath;
} ParsedPath;

/*
 * Prefix for all the configuration operations
 * iff the path does not begin with / or with #
 */

#define prefix (prefix_list ? prefix_list->data : NULL)

static GSList *prefix_list;

static TProfile *Current = 0;

/*
 * This one keeps track of all of the opened files
 */
static TProfile *Base = 0;

/* a set handler is a function which is called every time a gnome_config_set_*
   function is called, this can be used by the app to say guarantee a sync,
   apps using libgnomeui should not call this, libgnomeui already provides
   this, this would be for non-gui apps and apps that use a different toolkit
   then gtk*/
static void (*set_handler)(void *data) = NULL;
static void *set_handler_data;

#define CALL_SET_HANDLER() { if(set_handler) (*set_handler)(set_handler_data); }

/* same as above for a "sync" handler */
static void (*sync_handler)(void *data) = NULL;
static void *sync_handler_data;

#define CALL_SYNC_HANDLER() { if(sync_handler) (*sync_handler)(sync_handler_data); }


static void
release_path (ParsedPath *p)
{
	g_free (p->opath);
	g_free (p);
}

static ParsedPath *
parse_path (const char *path, gint priv)
{
	ParsedPath *p = g_malloc (sizeof (ParsedPath));
	char *sep;

	g_assert(path != NULL);
	
	if (*path == '/' || prefix == NULL)
		p->opath = g_strdup (path);
	else
		p->opath = g_copy_strings (prefix, path, NULL);

	p->path = p->opath;

	if (*p->path == '='){
		/* If it is an absolute path name */
		p->path++;
		p->file    = strtok (p->path, "=");
		p->section = strtok (NULL, "/=");
		p->key     = strtok (NULL, "=");
		p->def     = strtok (NULL, "=");
	} else {
		char *end;
		sep = "/=";

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
		if(priv)
			p->file = g_concat_dir_and_file (gnome_user_private_dir,
							 p->file);
		else
			p->file = g_concat_dir_and_file (gnome_user_dir,
							 p->file);
	}
	return p;
}

static int 
is_loaded (const char *filename, TSecHeader **section)
{
	TProfile *p = Base;
	struct stat st;
	
	while (p){
		if (strcasecmp (filename, p->filename) == 0){
			if (stat (filename, &st) == -1)
				st.st_mtime = 0;
			if (p->mtime != st.st_mtime)
				return 0;
			Current = p;
			*section = p->section;
			return 1;
		}
		p = p->link;
	}
	return 0;
}

static char *
decode_string_and_dup (char *s)
{
	char *p = g_malloc (strlen (s) + 1);
	char *q = p;

	do {
		if (*s == '\\'){
			switch (*(++s)){
			case 'n':
				*p++ = '\n';
				break;
			case '\\':
				*p++ = '\\';
				break;
			case 'r':
				*p++ = '\r';
				break;
			default:
				*p++ = '\\';
				*p++ = *s;
			}
		} else
			*p++ = *s;
	} while (*s++);
	return q;
}

static char *
escape_string_and_dup (char *s)
{
	char *return_value, *p = s;
	int len = 0;

	if(!s)
		return g_strdup("");
	
	while (*p){
		len++;
		if (*p == '\n' || *p == '\\' || *p == '\r' || *p == '\0')
			len++;
		p++;
	}
	return_value = p = (char *) g_malloc (len + 1);
	if (!return_value)
		return 0;
	do {
		switch (*s){
		case '\n':
			*p++ = '\\';
			*p++ = 'n';
			break;
		case '\r':
			*p++ = '\\';
			*p++ = 'r';
			break;
		case '\\':
			*p++ = '\\';
			*p++ = '\\';
			break;
		default:
			*p++ = *s;
		}
	} while (*s++);
	return return_value;
}

static TSecHeader *
load (const char *file)
{
	FILE *f;
	int state;
	TSecHeader *SecHeader = 0;
	char CharBuffer [STRSIZE];
	char *next = "";		/* Not needed */
	int c;
	
	if ((f = fopen (file, "r"))==NULL)
		return NULL;
	
	state = FirstBrace;
	while ((c = getc (f)) != EOF){
		if (c == '\r')		/* Ignore Carriage Return */
			continue;
		
		switch (state){
			
		case OnSecHeader:
			if (c == ']' || overflow){
				*next = '\0';
				next = CharBuffer;
				SecHeader->section_name = g_strdup (CharBuffer);
				state = IgnoreToEOL;
			} else
				*next++ = c;
			break;

		case IgnoreToEOL:
			if (c == '\n'){
				state = KeyDef;
				next = CharBuffer;
			}
			break;

		case FirstBrace:
		case KeyDef:
		case KeyDefOnKey:
			if (c == '[' && state != KeyDefOnKey){
				TSecHeader *temp;
		
				temp = SecHeader;
				SecHeader = (TSecHeader *) g_malloc (sizeof (TSecHeader));
				SecHeader->link = temp;
				SecHeader->keys = 0;
				state = OnSecHeader;
				next = CharBuffer;
				break;
			}
			/* On first pass, don't allow dangling keys */
			if (state == FirstBrace)
				break;
	    
			if ((c == ' ' && state != KeyDefOnKey) || c == '\t')
				break;
	    
			if (c == '\n' || overflow) { /* Abort Definition */
				next = CharBuffer;
                                break;
                        }
	    
			if (c == '=' || overflow){
				TKeys *temp;

				temp = SecHeader->keys;
				*next = '\0';
				SecHeader->keys = (TKeys *) g_malloc (sizeof (TKeys));
				SecHeader->keys->link = temp;
				SecHeader->keys->key_name = g_strdup (CharBuffer);
				state = KeyValue;
				next = CharBuffer;
			} else {
				*next++ = c;
				state = KeyDefOnKey;
			}
			break;

		case KeyValue:
			if (overflow || c == '\n'){
				*next = '\0';
				SecHeader->keys->value = decode_string_and_dup (CharBuffer);
				state = c == '\n' ? KeyDef : IgnoreToEOL;
				next = CharBuffer;
#ifdef GNOME_ENABLE_DEBUG
				printf ("[%s] (%s)=%s\n", SecHeader->section_name,
					SecHeader->keys->key_name, SecHeader->keys->value);
#endif
			} else
				*next++ = c;
			break;
	    
		} /* switch */
	
	} /* while ((c = getc (f)) != EOF) */
	if (c == EOF && state == KeyValue){
		*next = '\0';
		SecHeader->keys->value = decode_string_and_dup (CharBuffer);
	}
	fclose (f);
	return SecHeader;
}

static void 
new_key (TSecHeader *section, const char *key_name, const char *value)
{
	TKeys *key;
    
	key = (TKeys *) g_malloc (sizeof (TKeys));
	key->key_name = g_strdup (key_name);
	key->value   = g_strdup (value);
	key->link = section->keys;
	section->keys = key;
}

static const char *
access_config (access_type mode, const char *section_name,
	       const char *key_name, const char *def, const char *filename,
	       gboolean *def_used)
{
    
	TProfile   *New;
	TSecHeader *section;
	TKeys      *key;

	if (def_used)
		*def_used = FALSE;
	if (!is_loaded (filename, &section)){
		struct stat st;
		if (stat (filename, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (filename);
		New->section = load (filename);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
    
	/* Start search */
	for (; section; section = section->link){
		if (section->section_name == 0)
			continue;
		if (strcasecmp (section->section_name, section_name))
			continue;
		
		for (key = section->keys; key; key = key->link){
			if (strcasecmp (key->key_name, key_name))
				continue;
			if (mode == SET){
				g_free (key->value);
				key->value = g_strdup (def);
			}
			return key->value;
		}

		/* No key found */
		if (mode == SET){
			new_key (section, key_name, def);
			return 0;
		}
	}
    
	/* Non existent section */
	if ((mode == SET) && def){
		section = (TSecHeader *) g_malloc (sizeof (TSecHeader));
		section->section_name = g_strdup (section_name);
		section->keys = 0;
		new_key (section, key_name, def);
		section->link = Current->section;
		Current->section = section;
	} 
	if (def_used)
		*def_used = TRUE;
	return def;
}

static void 
dump_keys (FILE *profile, TKeys *p)
{
	if (!p)
		return;
	dump_keys (profile, p->link);
	if (*p->key_name) {
		char *t = escape_string_and_dup (p->value);
		fprintf (profile, "%s=%s\n", p->key_name, t);
		g_free (t);
	}
}

static void 
dump_sections (FILE *profile, TSecHeader *p)
{
	if (!p)
		return;
	dump_sections (profile, p->link);
	if (p->section_name && p->section_name [0]){
		fprintf (profile, "\n[%s]\n", p->section_name);
		dump_keys (profile, p->keys);
	}
}

/*check the path and if we need to create directories create them with
  mode newmode, it needs an absolute path name or it will fail, it
  needs to be passed the dir and the filename since it will take the
  filename off*/
static gint
check_path(char *path, mode_t newmode)
{
	gchar *dirpath;
	gchar *p;
	GString *newpath;
	struct stat s;


	g_return_val_if_fail(path!=NULL, FALSE);

	if(strchr(path,'/')==NULL)
		return FALSE;
	dirpath = g_strdup(path);
	if(!dirpath)
		return FALSE;

	p = strrchr(dirpath,'/');
		*p='\0';

	if(*dirpath == '\0') {
		g_free(dirpath);
		return FALSE;
	}

	/*not absolute, we refuse to work*/
	if(dirpath[0]!='/') {
		g_free(dirpath);
		return FALSE;
	}

	/*special case if directory exists, this is probably gonna happen
	  a lot so we don't want to go though checking it part by part*/
	if(stat(dirpath,&s)==0) {
		g_free(dirpath);
		/*check if a directory*/
		if(!S_ISDIR(s.st_mode))
			return FALSE;
		else
			return TRUE;
	}


	/*skip leading '/'*/
	p = dirpath;
	while(*p == '/')
		p++;

	p=strtok(p,"/");
	newpath = g_string_new("");
	do {
		newpath = g_string_append_c(newpath,'/');
		newpath = g_string_append(newpath,p);
		if(stat(newpath->str,&s)==0) {
			/*check if a directory*/
			if(!S_ISDIR(s.st_mode)) {
				g_free(dirpath);
				g_string_free(newpath,TRUE);
				return FALSE;
			}
		} else {
			/*we couldn't stat it .. let's try making the
			  directory*/
			if(mkdir(newpath->str,newmode)!=0) {
				/*error, return false*/
				g_free(dirpath);
				g_string_free(newpath,TRUE);
				return FALSE;
			}
		}

	} while((p=strtok(NULL,"/"))!=NULL);

	g_free(dirpath);
	g_string_free(newpath,TRUE);

	return TRUE;
}

static void 
dump_profile (TProfile *p)
{
	FILE *profile;
    
	if (!p)
		return;
	dump_profile (p->link);

	/* .ado: p->filename can be empty, it's better to jump over */
	if (p->filename[0] != (char) 0)
		if (check_path(p->filename,0755) &&
		    (profile = fopen (p->filename, "w")) != NULL){
			dump_sections (profile, p->section);
			fclose (profile);
		}
}

/*
 * Must be called at the end.
*/
void 
gnome_config_sync (void)
{
	dump_profile (Base);
	CALL_SYNC_HANDLER();
}

static void 
free_keys (TKeys *p)
{
	if (!p)
		return;
	free_keys (p->link);
	g_free (p->key_name);
	g_free (p->value);
	g_free (p);
}

static void 
free_sections (TSecHeader *p)
{
	if (!p)
		return;
	free_sections (p->link);
	free_keys (p->keys);
	g_free (p->section_name);
	p->link = 0;
	p->keys = 0;
	g_free (p);
}

static void 
free_profile (TProfile *p)
{
	if (!p)
		return;
	free_profile (p->link);
	free_sections (p->section);
	g_free (p->filename);
	g_free (p);
}

void 
_gnome_config_clean_file (const char *path, gint priv)
{
	TProfile *p;
	ParsedPath *pp;
	char *fake_path;
	
	if (!path)
		return;

	fake_path = g_copy_strings (path, "/section/key", NULL);
	pp = parse_path (fake_path, priv);
	g_free (fake_path);
	
	for (p = Base; p; p = p->link){
		if (strcmp (pp->file, p->filename) != 0)
			continue;
		
		free_sections (p->section);
		p->section = 0;
		p->filename [0] = 0;
		release_path (pp);
		return;
	}
	release_path (pp);
}

void *
_gnome_config_init_iterator (const char *path, gint priv)
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;
	iterator_type *iter;


	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path, priv);
	g_free (fake_path);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		if (stat (pp->file, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	for (; section; section = section->link){
		if (strcasecmp (section->section_name, pp->section))
			continue;
		iter = g_new (iterator_type, 1);
		iter->type = 0;
		iter->value = section->keys;
		release_path (pp);
		return iter;
	}
	release_path (pp);
	return 0;
}

void *
_gnome_config_init_iterator_sections (const char *path, gint priv)
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;
	iterator_type *iter;


	fake_path = g_copy_strings (path, "/section/key", NULL);
	pp = parse_path (fake_path, priv);
	g_free (fake_path);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		if (stat (pp->file, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	iter = g_new (iterator_type, 1);
	iter->type = 1;
	iter->value = section;
	release_path (pp);
	return iter;
}

void *
gnome_config_iterator_next (void *s, char **key, char **value)
{
	iterator_type *iter = s;
	
	if (iter->type == 0){
		TKeys *keys;
		keys = iter->value;
		if (keys){
			*key   = g_strdup (keys->key_name);
			*value = g_strdup (keys->value);
			keys   = keys->link;
			iter->value = keys;
			return iter;
		} else {
			g_free (iter);
			return 0;
		}
	} else {
		TSecHeader *section;
		section = iter->value;

		if (section){
			*key = g_strdup (section->section_name);
			section = section->link;
			iter->value = section;
			return iter;
		} else {
			g_free (iter);
			return 0;
		}
	}
}

void 
_gnome_config_clean_section (const char *path, gint priv)
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;

	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path, priv);
	g_free (fake_path);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		if (stat (pp->file, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	/* We only disable the section, so it will still be g_freed, but it */
	/* won't be find by further walks of the structure */

	for (; section; section = section->link){
		if (strcasecmp (section->section_name, pp->section))
			continue;
		section->section_name [0] = 0;
	}
	release_path (pp);
}

void 
_gnome_config_clean_key (const char *path, gint priv)
	/* *section_name, char *file */
{
	TProfile   *New;
	TSecHeader *section;
	TKeys *key;
	ParsedPath *pp;
	
	pp = parse_path (path, priv);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		if (stat (pp->file, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	for (; section; section = section->link){
	        if (strcasecmp (section->section_name, pp->section))
		        continue;
		for (key = section->keys; key; key = key->link){
			if (strcasecmp (key->key_name, pp->key))
				continue;
			key->key_name [0] = 0;
		}
	}
	release_path (pp);
}

gboolean 
_gnome_config_has_section (const char *path, gint priv)
	/* char *section_name, char *profile */
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;

	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path,priv);
	g_free (fake_path);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		if (stat (pp->file, &st) == -1) st.st_mtime = 0;

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = g_strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	for (; section; section = section->link){
		if (strcasecmp (section->section_name, pp->section))
			continue;
		release_path (pp);
		return 1;
	}
	release_path (pp);
	return 0;
}

void 
gnome_config_drop_all (void)
{
	free_profile (Base);
	Base = 0;
}

gint
_gnome_config_get_int_with_default (const char *path, gboolean *def, gint priv)
{
	ParsedPath *pp;
	const char *r;
	int  v;
	
	pp = parse_path (path, priv);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);

	g_return_val_if_fail(r != NULL, 0);

	v = atoi (r);
	release_path (pp);
	return v;
}

gdouble
_gnome_config_get_float_with_default (const char *path, gboolean *def, gint priv)
{
	ParsedPath *pp;
	const char *r;
	gdouble v;
	
	pp = parse_path (path, priv);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);

	g_return_val_if_fail(r != NULL, 0);

	v = strtod(r, NULL);
	release_path (pp);
	return v;
}

char *
_gnome_config_get_translated_string_with_default (const char *path,
						  gboolean *def,
						  gint priv)
{
	GList *language_list;

	char *value= NULL;

	language_list= gnome_i18n_get_language_list ("LC_ALL");

	while (!value && language_list) {
		const char *lang= language_list->data;

		if (strcmp (lang, "C") == 0) {
			value = _gnome_config_get_string_with_default (path, def, priv);
			if (value)
				return value;
			else {
				/* FIXME: this "else" block should go away in some time.
				 * We use it to handle the old broken files that were
				 * written by the old buggy set_translated_string
				 * function, which *did* append the "[C]" suffix.
				 */

				gchar *tkey;

				tkey = g_copy_strings (path, "[C]", NULL);
				value = _gnome_config_get_string_with_default (tkey, def, priv);
				g_free (tkey);

				if (!value || *value == '\0') {
					g_free (value);
					value = NULL;
				}

				return value;
			}
		} else {
			gchar *tkey;

			tkey= g_copy_strings (path, "[", lang, "]", NULL);
			value= _gnome_config_get_string_with_default (tkey, def, priv);
			g_free (tkey);

			if (!value || *value == '\0') {
				size_t n;

				g_free (value);
				value= NULL;

				/* Sometimes the locale info looks
				   like `pt_PT@verbose'.  In this case
				   we want to try `pt' as a backup.  */
				n = strcspn (lang, "@_");
				if (lang[n]) {
					char *copy = strndup (lang, n);
					tkey = g_copy_strings (path, "[",
							       copy, "]",
							       NULL);
					value = _gnome_config_get_string_with_default (tkey, def, priv);
					g_free (tkey);
					g_free (copy);
					if (! value || *value == '\0') {
						g_free (value);
						value = NULL;
					}
				}
			}
		}
		language_list= language_list->next;
	}

	return value;
}

char *
_gnome_config_get_string_with_default (const char *path, gboolean *def,
				       gint priv)
{
	ParsedPath *pp;
	const char *r;
	char *ret = NULL;
	
	pp = parse_path (path, priv);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);
	if (r)
		ret = g_strdup (r);
	release_path (pp);
	return ret;
}

gboolean
_gnome_config_get_bool_with_default (const char *path, gboolean *def,
				     gint priv)
{
	ParsedPath *pp;
	const char *r;
	int  v;
	
	pp = parse_path (path, priv);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);

	g_return_val_if_fail(r != NULL, 0);

	if (!strcasecmp (r, "true")){
		v = 1;
	} else if (!strcasecmp (r, "false")){
		v = 0;
	} else {
	        /* FIXME: what to return?  */
	        v = 0;
	}
	release_path (pp);
	return v;
}

void
gnome_config_make_vector (const char *rr, int *argcp, char ***argvp)
{
	char *p;
	int count, esc_spcs;
	int space_seen;

	/* Figure out how large to make return vector.  Start at 2
	 * because we want to make NULL-terminated array, and because
	 * the loop doesn't count the final element.
	 */
	count = 2;
	space_seen = 0;
	for (p = (char *) rr; *p; ++p) {
		/* The way that entries are constructed by
		   gnome_config_set_vector ensures we'll never see an
		   unpaired `\' at the end of a string.  So this is
		   safe.  */
	        if (*p == '\\') {
			++p;
		} else if (*p == ' ') {
			space_seen = 1;
		} else if (space_seen){
			count++;
			space_seen = 0;
		}
	}

	*argcp = count - 1;
	*argvp = (char **) g_malloc0 (count * sizeof (char *));

	p = (char *) rr;
	count = 0;
	do {
		char *tmp = p;

		esc_spcs = 0;
		while (*p && (esc_spcs ? 1 : (*p != ' '))){
			esc_spcs = 0;
			if (*p == '\\')
				esc_spcs = 1;
			p++;
		}

 		(*argvp)[count++] = (char *) strndup (tmp, p - tmp);

		while (*p && *p == ' ')
			p++;
	} while (*p);
}

void
_gnome_config_get_vector_with_default (const char *path, int *argcp,
				       char ***argvp, gboolean *def, gint priv)
{
	ParsedPath *pp;
	const char *rr;
	
	pp = parse_path (path, priv);
	rr = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file, def);

	if (rr == NULL) {
		*argvp = NULL;
		*argcp = 0;
	} else
		gnome_config_make_vector (rr, argcp, argvp);
	release_path (pp);
}

void
_gnome_config_set_translated_string (const char *path, const char *value,
				     gint priv)
{
	GList *language_list;
	const char *lang;
	char *tkey;

	language_list= gnome_i18n_get_language_list("LC_ALL");

	lang= language_list ? language_list->data : NULL;

	if (lang && (strcmp (lang, "C") != 0)) {
		tkey = g_copy_strings (path, "[", lang, "]", NULL);
		_gnome_config_set_string(tkey, value, priv);
		g_free (tkey);
	} else
		_gnome_config_set_string (path, value, priv);
	CALL_SET_HANDLER();
}

void
_gnome_config_set_string (const char *path, const char *new_value, gint priv)
{
	ParsedPath *pp;
	const char *r;
	
	pp = parse_path (path, priv);
	r = access_config (SET, pp->section, pp->key, new_value, pp->file,
			   NULL);
	release_path (pp);
	CALL_SET_HANDLER();
}

void
_gnome_config_set_int (const char *path, int new_value, gint priv)
{
	ParsedPath *pp;
	char intbuf [40];
	const char *r;
	
	pp = parse_path (path, priv);
	sprintf (intbuf, "%d", new_value);
	r = access_config (SET, pp->section, pp->key, intbuf, pp->file,
			   NULL);
	release_path (pp);
	CALL_SET_HANDLER();
}

void
_gnome_config_set_float (const char *path, gdouble new_value, gint priv)
{
	ParsedPath *pp;
	char floatbuf [40];
	const char *r;
	
	pp = parse_path (path, priv);
	sprintf (floatbuf, "%.17g", new_value);
	r = access_config (SET, pp->section, pp->key, floatbuf, pp->file,
			   NULL);
	release_path (pp);
	CALL_SET_HANDLER();
}

void
_gnome_config_set_bool (const char *path, gboolean new_value, gint priv)
{
	ParsedPath *pp;
	const char *r;
	
	pp = parse_path (path, priv);
	r = access_config (SET, pp->section, pp->key,
			   new_value ? "true" : "false", pp->file, NULL);
	release_path (pp);
	CALL_SET_HANDLER();
}

char *
gnome_config_assemble_vector (int argc, const char *const argv [])
{
	char *value, *p;
	const char *s;
	int i, len;

	/* Compute length of quoted string.  We cheat and just use
	   twice the sum of the lengths of all the strings.  Sigh.  */
	len = 1;
	for (i = 0; i < argc; ++i) {
		len += 2 * strlen (argv[i]) + 1;
	}

	p = value = g_malloc (len);
	for (i = 0; i < argc; ++i) {
		for (s = argv[i]; *s; ++s) {
			if (*s == ' ' || *s == '\\')
				*p++ = '\\';
			*p++ = *s;
		}
		*p++ = ' ';
	}
	*p = '\0';

	return value;
}

void
_gnome_config_set_vector (const char *path, int argc,
			  const char *const argv[],
			  gint priv)
{
	ParsedPath *pp;
	char *s;

	pp = parse_path (path, priv);
	s = gnome_config_assemble_vector (argc, argv);
	access_config (SET, pp->section, pp->key, s, pp->file, NULL);
	g_free (s);
	release_path (pp);
	CALL_SET_HANDLER();
}

void
gnome_config_push_prefix (const char *path)
{
	prefix_list = g_slist_prepend(prefix_list, g_strdup(path));
}

void
gnome_config_pop_prefix (void)
{
	if(prefix_list) {
		g_free(prefix_list->data);
		prefix_list = g_slist_remove_link(prefix_list, prefix_list);
	}
}

void
gnome_config_set_set_handler(void (*func)(void *),void *data)
{
	set_handler = func;
	set_handler_data = data;
}
	
void
gnome_config_set_sync_handler(void (*func)(void *),void *data)
{
	sync_handler = func;
	sync_handler_data = data;
}

#ifdef TEST

static
x (char *str, char *file, char *sec, char *key, char *val)
{
	ParsedPath *pp;

	printf ("%s\n", str);
	pp = parse_path (str, FALSE);
	printf ("   file: %s [%s]\n", pp->file, file);
	printf ("   sect: %s [%s]\n", pp->section, sec);
	printf ("   key:  %s [%s]\n", pp->key, key);
	printf ("   def:  %s [%s]\n", pp->def, val);
}


main ()
{
	gnome_user_dir = "USERDIR";
	x ("=/tmp/file=seccion/llave=valor", "/tmp/file", "seccion", "llave", "valor");
	x ("=/tmp/file=seccion/llave", "/tmp/file", "seccion", "llave", NULL);
	x ("/file/seccion/llave=valor", "USERDIR/file", "seccion", "llave", "valor");
	x ("/file/seccion/llave", "USERDIR/file", "seccion", "llave", NULL);
	x ("/file/archivo/archivo/seccion/llave", "USERDIR/file/archivo/archivo", "seccion", "llave", NULL);
	x ("/file/archivo/archivo/seccion/llave=valor", "USERDIR/file/archivo/archivo", "seccion", "llave", "valor");
	
}
#endif
