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
   along with this program; if not, write to the G_Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* #include <config.h> */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>	/* For g_free() and atoi() */
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include "libgnome.h"

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

struct _prefix_list {
	char *p_prefix;
	struct _prefix_list *p_back;
};

typedef struct _prefix_list prefix_list_t;

/*
 * Prefix for all the configuration operations
 * iff the path does not begin with / or with #
 */

#define prefix (prefix_list ? prefix_list->p_prefix : NULL)

static prefix_list_t *prefix_list;

static TProfile *Current = 0;

/*
 * This one keeps track of all of the opened files
 */
static TProfile *Base = 0;

static void
release_path (ParsedPath *p)
{
	g_free (p->opath);
	g_free (p);
}

static ParsedPath *
parse_path (const char *path)
{
	ParsedPath *p = g_malloc (sizeof (ParsedPath));
	char *sep;

	g_assert(path != NULL);
	
	if (*path == '/' || prefix == NULL)
		p->opath = strdup (path);
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
		p->file = g_concat_dir_and_file (gnome_user_dir, p->file);
	}
	return p;
}

static int 
is_loaded (const char *filename, TSecHeader **section)
{
	TProfile *p = Base;
	struct stat st;
	
	while (p){
		if (!strcasecmp (filename, p->filename)){
			stat (filename, &st);
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
				SecHeader->section_name = strdup (CharBuffer);
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
			if (c == '['){
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
	    
			if (c == '\n' || overflow) /* Abort Definition */
				next = CharBuffer;
	    
			if (c == '=' || overflow){
				TKeys *temp;

				temp = SecHeader->keys;
				*next = '\0';
				SecHeader->keys = (TKeys *) g_malloc (sizeof (TKeys));
				SecHeader->keys->link = temp;
				SecHeader->keys->key_name = strdup (CharBuffer);
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
#ifdef DEBUG
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
	key->key_name = strdup (key_name);
	key->value   = strdup (value);
	key->link = section->keys;
	section->keys = key;
}

static const char *
access_config (access_type mode, const char *section_name,
	       const char *key_name, const char *def, const char *filename,
	       int *def_used)
{
    
	TProfile   *New;
	TSecHeader *section;
	TKeys      *key;

	if (def_used)
		*def_used = 0;
	if (!is_loaded (filename, &section)){
		struct stat st;
		stat (filename, &st);

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = strdup (filename);
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
				key->value = strdup (def);
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
		section->section_name = strdup (section_name);
		section->keys = 0;
		new_key (section, key_name, def);
		section->link = Current->section;
		Current->section = section;
	} 
	if (def_used)
		*def_used = 1;
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
		free (t);
	}
}

static void 
dump_sections (FILE *profile, TSecHeader *p)
{
	if (!p)
		return;
	dump_sections (profile, p->link);
	if (p->section_name [0]){
		fprintf (profile, "\n[%s]\n", p->section_name);
		dump_keys (profile, p->keys);
	}
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
		if ((profile = fopen (p->filename, "w")) != NULL){
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
gnome_config_clean_file (const char *path)
{
	TProfile *p;
	ParsedPath *pp;
	char *fake_path;
	
	if (!path)
		return;

	fake_path = g_copy_strings (path, "/section/key", NULL);
	pp = parse_path (fake_path);
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
gnome_config_init_iterator (const char *path)
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;

	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path);
	g_free (fake_path);
	
	if (!is_loaded (pp->file, &section)){
		struct stat st;
		stat (pp->file, &st);

		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = strdup (pp->file);
		New->section = load (pp->file);
		New->mtime = st.st_mtime;
		Base = New;
		section = New->section;
		Current = New;
	}
	for (; section; section = section->link){
		if (strcasecmp (section->section_name, pp->section))
			continue;
		return section->keys;
	}
	release_path (pp);
	return 0;
}

void *
gnome_config_iterator_next (void *s, char **key, char **value)
{
	TKeys *keys = (TKeys *) s;

	if (keys){
		*key   = g_strdup (keys->key_name);
		*value = g_strdup (keys->value);
		keys   = keys->link;
	}
	return keys;
}

void 
gnome_config_clean_section (const char *path)
{
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;

	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path);
	g_free (fake_path);
	
	/* We assume the user has called one of the other initialization funcs */
	if (!is_loaded (pp->file, &section)){
		fprintf (stderr,"Warning: profile_clean_section called before init\n");
		release_path (pp);
		return;
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
gnome_config_clean_key (const char *path)
	/* *section_name, char *file */
{
	TSecHeader *section;
	TKeys *key;
	ParsedPath *pp;
	
	pp = parse_path (path);
	
	/* We assume the user has called one of the other initialization funcs */
	if (!is_loaded (pp->file, &section)){
		fprintf (stderr,"Warning: profile_clean_section called before init\n");
		release_path (pp);
		return;
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

int 
gnome_config_has_section (const char *path)
	/* char *section_name, char *profile */
{
	TSecHeader *section;
	ParsedPath *pp;
	char *fake_path;

	fake_path = g_copy_strings (path, "/key", NULL);
	pp = parse_path (fake_path);
	g_free (fake_path);
	
	/* We assume the user has called one of the other initialization funcs */
	if (!is_loaded (pp->file, &section)){
		release_path (pp);
		return 0;
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
}

int
gnome_config_get_int_with_default (const char *path, int *def)
{
	ParsedPath *pp;
	const char *r;
	int  v;
	
	pp = parse_path (path);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);

	g_return_val_if_fail(r != NULL, 0);

	v = atoi (r);
	release_path (pp);
	return v;
}

char *
gnome_config_get_string_with_default (const char *path, int *def)
{
	ParsedPath *pp;
	const char *r;
	char *ret = NULL;
	
	pp = parse_path (path);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);
	if (r)
		ret = strdup (r);
	release_path (pp);
	return ret;
}

int
gnome_config_get_bool_with_default (const char *path, int *def)
{
	ParsedPath *pp;
	const char *r;
	int  v;
	
	pp = parse_path (path);
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
gnome_config_set_string (const char *path, const char *new_value)
{
	ParsedPath *pp;
	const char *r;
	
	pp = parse_path (path);
	r = access_config (SET, pp->section, pp->key, new_value, pp->file,
			   NULL);
	release_path (pp);
}

void
gnome_config_set_int (const char *path, int new_value)
{
	ParsedPath *pp;
	char intbuf [40];
	const char *r;
	
	pp = parse_path (path);
	sprintf (intbuf, "%d", new_value);
	r = access_config (SET, pp->section, pp->key, intbuf, pp->file,
			   NULL);
	release_path (pp);
}

void
gnome_config_set_bool (const char *path, int new_value)
{
	ParsedPath *pp;
	const char *r;
	
	pp = parse_path (path);
	r = access_config (SET, pp->section, pp->key,
			   new_value ? "true" : "false", pp->file, NULL);
	release_path (pp);
}

void
gnome_config_push_prefix (const char *path)
{
	prefix_list_t *p = g_malloc (sizeof (prefix_list_t));

	p->p_back = prefix_list;
	p->p_prefix = strdup (path);
	prefix_list = p;
}

void
gnome_config_pop_prefix (void)
{
	prefix_list_t *p = prefix_list;
	
	if (!p)
		return;

	free (p->p_prefix);
	prefix_list = p->p_back;
	free (p);
}

#ifdef TEST

static
x (char *str, char *file, char *sec, char *key, char *val)
{
	ParsedPath *pp;

	printf ("%s\n", str);
	pp = parse_path (str);
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
