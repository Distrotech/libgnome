/*
 * Configuration-File Functions.
 *
 *  Copyright 1993, 1994, 1997 The Free Software Foundation
 *
 * Authors: Miguel de Icaza

   This program is g_free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the G_Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the G_Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   DOC: gnome configuration routines.
   
   All of the routines receive a pathname, the pathname has the following
   form:

        /filename/section/key[=default]

   This format reprensents: a filename relative to the Gnome config
   directory called filename (ie, ~/.gnome/filename), in that file there
   is a section called [section] and key is the left handed side of the
   values.

   If default is provided, it cane be used to return a default value
   if none is specified on the config file.

   Examples:
   
   /gmix/Balance/Ratio=0.5
   /filemanager/Panel Display/html=1

   If the pathname starts with '=', then instead of being a ~/.gnome relative
   file, it is an abolute pathname, example:

   =/home/miguel/.mc.ini=/Left Panel/reverse=1

   This reprensents the config file: /home/miguel/.mc.ini, section [Left Panel],
   variable reverse.
   
   */

/* #include <config.h> */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>	/* For g_free() and atoi() */
#include <sys/types.h>
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
static char *prefix;

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
parse_path (char *path)
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
		sep = "=";
		p->path++;
	} else
		sep = "/=";
	
	p->file    = strtok (p->path, sep);
	p->section = strtok (NULL, "/=");
	p->key     = strtok (NULL, "=");
	p->def     = strtok (NULL, "=");

	/* Was it a Gnome-relative pathname? */
	if (*sep == '/'){
		char *f = g_concat_dir_and_file (gnome_user_dir, p->file);
		p->file = f;
	}
	return p;
}

static int 
is_loaded (char *filename, TSecHeader **section)
{
	TProfile *p = Base;
	
	while (p){
		if (!strcasecmp (filename, p->filename)){
			Current = p;
			*section = p->section;
			return 1;
		}
		p = p->link;
	}
	return 0;
}

static TSecHeader *
load (char *file)
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
				SecHeader->keys->value = strdup (CharBuffer);
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
		SecHeader->keys->value = strdup (CharBuffer);
	}
	fclose (f);
	return SecHeader;
}

static void 
new_key (TSecHeader *section, char *key_name, char *value)
{
	TKeys *key;
    
	key = (TKeys *) g_malloc (sizeof (TKeys));
	key->key_name = strdup (key_name);
	key->value   = strdup (value);
	key->link = section->keys;
	section->keys = key;
}

static char *
access_config (access_type mode, char *section_name, char *key_name, 
		   char *def, char *filename, int *def_used)
{
    
	TProfile   *New;
	TSecHeader *section;
	TKeys      *key;

	if (def_used)
		*def_used = 0;
	if (!is_loaded (filename, &section)){
		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = strdup (filename);
		New->section = load (filename);
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
	if (*p->key_name) 
		fprintf (profile, "%s=%s\n", p->key_name, p->value);
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
gnome_config_clean_file (char *path)
{
	TProfile *p;
	ParsedPath *pp;
	
	if (!path)
		return;

	pp = parse_path (path);
	
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
gnome_config_init_iterator (char *path)
{
	TProfile   *New;
	TSecHeader *section;
	ParsedPath *pp;

	pp = parse_path (path);
	
	if (!is_loaded (pp->file, &section)){
		New = (TProfile *) g_malloc (sizeof (TProfile));
		New->link = Base;
		New->filename = strdup (pp->file);
		New->section = load (pp->file);
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
gnome_config_clean_section (char *path)
	/* *section_name, char *file */
{
	TSecHeader *section;
	ParsedPath *pp;

	pp = parse_path (path);
	
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
gnome_config_clean_key (char *path)
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
gnome_config_has_section (char *path)
	/* char *section_name, char *profile */
{
	TSecHeader *section;
	ParsedPath *pp;

	pp = parse_path (path);
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
gnome_config_get_int_with_default (char *path, int *def)
{
	ParsedPath *pp;
	char *r;
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
gnome_config_get_string_with_default (char *path, int *def)
{
	ParsedPath *pp;
	char *r;
	
	pp = parse_path (path);
	r = access_config (LOOKUP, pp->section, pp->key, pp->def, pp->file,
			   def);
	if (r)
		r = strdup (r);
	release_path (pp);
	return r;
}

int
gnome_config_get_bool_with_default (char *path, int *def)
{
	ParsedPath *pp;
	char *r;
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
gnome_config_set_string (char *path, char *new_value)
{
	ParsedPath *pp;
	char *r;
	
	pp = parse_path (path);
	r = access_config (SET, pp->section, pp->key, new_value, pp->file,
			   NULL);
	release_path (pp);
}

void
gnome_config_set_int (char *path, int new_value)
{
	ParsedPath *pp;
	char intbuf [40];
	char *r;
	
	pp = parse_path (path);
	sprintf (intbuf, "%d", new_value);
	r = access_config (SET, pp->section, pp->key, intbuf, pp->file,
			   NULL);
	release_path (pp);
}

void
gnome_config_set_bool (char *path, int new_value)
{
	ParsedPath *pp;
	char *r;
	
	pp = parse_path (path);
	r = access_config (SET, pp->section, pp->key,
			   new_value ? "true" : "false", pp->file, NULL);
	release_path (pp);
}

void
gnome_config_set_prefix (char *path)
{
	prefix = strdup (path);
}

void
gnome_config_drop_prefix (void)
{
	if (!prefix)
		return;
	free (prefix);
	prefix = 0;
}
