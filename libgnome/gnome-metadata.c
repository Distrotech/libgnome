/* gnome-metadata.c - Metadata implementation.

   Copyright (C) 1998 Tom Tromey

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

#include <string.h>
#include <db.h>

#include "libgnomeP.h"


/* Each key in the database has 3 parts: the space, the object, and
   the "key" (confusingly named, I know).  The space is "type" for
   type maps, "file" for direct file maps, and "regexp" for regular
   expressions.  The object is the type name, the file name, or the
   regexp.  The key part is the actual metadata key to be used.

   For each file name we keep a special key `filelist\0<name>' whose
   associated data is a list of all the metadata keys we currently
   know about for this file.

   For each metadata key we keep a special db key `regexplist\0<key>'
   whose associated data is a list of all the regular expressions
   associated with that metadata key.

   For each type we keep a special key `typelist\0<key>' whose
   associated data is a list of all the types associated with that
   metadata key.

   These special keys let us handle lookups more quickly.  */

/* TO DO:
   Fix all FIXME comments
   Fix metadata_set to track special values
   Likewise for metadata_remove
   Write try_regexps.  Write regexp cache object and create a new
   one here
   Let system provide a fallback database as well.  This would be used
   to let applications set their own icons by default, etc. ?
   Define error codes
   Think about having a way to allow ordering of regexps
   */



/* The database.  */
static DB *database;



/* Initialize the database.  */
static void
init (void)
{
	/* assert (! database); */
	database = dbopen (FIXME, O_CREATE | O_RDWR, 0700, DB_HASH, NULL);
}

/* Set a piece of metadata.  */
static int
metadata_set (const char *space, const char *object, const char *key,
	      int size, const char *data)
{
	char *dbkey;
	int sl = strlen (space), ol = strlen (object), kl = strlen (key);
	int key_size = sl + ol + kl + 3;
	DBT key, value;

	if (! database)
		init ();

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcpy (dbkey + sl + 1, object);
	strcpy (dbkey + sl + ol + 2, key);

	key.data = dbkey;
	key.size = key_size;

	value.data = data;
	value.size = size;

	/* FIXME: error code.  */
	if (database->put (database, &key, &value, 0))
		return FIXME;

	/* Now update the special list item.  */
	strcpy (dbkey, space);
	strcat (dbkey, "list");
	strcpy (dbkey + sl + 4 + 1, object);

	if (! database->get (....)) {
		/* FIXME: Update list with new info, if required.  */
	} else {
		/* FIXME: set up initial list.  */
	}

	/* Set the special list.  */
	if (database->set (FIXME)) {
		/* Our update must be atomic.  */
		database->del (database, &FIXME, 0);
		return FIXME;
	}

	return 0;
}

/* Remove a piece of metadata.  */
static int
metadata_remove (const char *space, const char *object, const char *key)
{
	char *dbkey;
	int sl = strlen (space), ol = strlen (object), kl = strlen (key);
	int key_size = sl + ol + kl + 3;
	DBT key;

	if (! database)
		init ();

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcpy (dbkey + sl + 1, object);
	strcpy (dbkey + sl + ol + 2, key);

	key.data = dbkey;
	key.size = key_size;

	/* FIXME: error code.  */
	return database->del (database, &key, 0);
}

static int
metadata_get_list (const char *space, const char *object, DBT *value)
{
	char *dbkey;
	int sl = strlen (space), ol = strlen (object);
	int key_size = sl + ol + 4 + 2;
	DBT key;

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcat (dbkey, "list");
	strcpy (dbkey + sl + 4 + 1, object);

	key.data = dbkey;
	key.size = key_size;

	if (database->get (database, &key, value, 0))
		return FIXME;
	return 0;
}

static int
metadata_get_no_dup (const char *space, const char *object, const char *key,
		     int *size, char **buffer)
{
	char *dbkey;
	int sl = strlen (space), ol = strlen (object), kl = strlen (key);
	int key_size = sl + ol + kl + 3;
	DBT key, value;

	if (! database)
		init ();

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcpy (dbkey + sl + 1, object);
	strcpy (dbkey + sl + ol + 2, key);

	key.data = dbkey;
	key.size = key_size;

	/* If we fail we want *BUFFER to be NULL.  */
	*buffer = NULL;
	if (database->get (database, &key, &value, 0))
		return FIXME;

	*size = value.size;
	*buffer = value.data;

	return 0;
}

static int
metadata_get (const char *space, const char *object, const char *key,
	      int *size, char **buffer)
{
	int r = metadata_get_no_dup (space, object, key, size, buffer);
	if (! r) {
		char *n = malloc (*size);
		memcpy (n, *buffer, *size);
		*buffer = n;
	}
	return r;
}



/* Set metadata associated with FILE.  Returns 0 on success, or an
   error code.  */
int
gnome_metadata_set (const char *file, const char *name,
		    int size, const char *data)
{
	return metadata_set ("file", file, name, size, data);
}

/* Remove a piece of metadata associated with FILE.  Returns 0 on
   success, or an error code.  */
int
gnome_metadata_remove (const char *file, const char *name)
{
	return metadata_remove ("file", file, name);
}

/* Return an array of all metadata keys associated with FILE.  The
   array is NULL terminated.  The result value can be freed with
   gnome_string_array_free.  This only returns keys for which there is
   a particular association with FILE.  It will not return keys for
   which a regexp or other match succeeds.  */
char **
gnome_metadata_list (const char *file)
{
	DBT value;
	int num, i;
	char **result, *p;

	if (metadata_get_list ("file", file, &value))
		return NULL;

	/* Count number of strings in data value.  */
	for (num = i = 0; i < value.size; ++i) {
		if (! value.data[i])
			++num;
	}

	/* Allocate one extra slot for trailing NULL.  */
	result = (char **) malloc ((num + 1) * sizeof (char *));

	p = value.data;
	for (i = 0; i < num; ++i) {
		int len = strlen (p);
		result[i] = strdup (p);
		p += len + 1;
	}
	result[i] = NULL;

	return result;
}



/* Run all regular expressions associated with KEY.  Return 0 on
   success.  */
static int
try_regexps (const char *file, const char *key, int *size, char **buffer)
{
	return 1;
}

/* Run the `file' command and use its output to determine the file's
   type.  */
static char *
run_file (const char *file)
{
	/* FIXME: implement.  */
	return NULL;
}

/* Do all the work for _get and _get_fast.  */
static int
get_worker (const char *file, const char *name, int *size, char **buffer,
	    int is_fast)
{
	int type_size, r;
	char *type;

	/* Phase 1: see if data exists in database.  */
	if (! metadata_get ("file", file, name, size, buffer))
		return 0;

	/* Phase 2: see if a direct regular expression matches the
	   file name.  */
	if (! try_regexps (file, name, size, buffer))
		return 0;

	/* Phase 3: see if `type' is set.  */
	if (metadata_get ("file", file, "type", &type_size, &type)) {
		/* Didn't find type.  See if regular expression can
		   characterize file type.  */
		if (try_regexps (file, "type", &type_size, &type)) {
			/* Nope.  If slow, try `file' command.  */
			if (! is_fast)
				type = run_file (file);
			if (! type)
				return ERROR;
		}
	}

	/* Return lookup based on discovered type.  */
	r = metadata_get ("type", type, name, size, buffer);
	free (type);
	return r;
}

/* Get a piece of metadata associated with FILE.  SIZE and BUFFER are
   result parameters.  *BUFFER is malloc()d.  Returns 0, or an error
   code.  On error *BUFFER will be set to NULL.  */
int
gnome_metadata_get (const char *file, const char *name,
		    int *size, char **buffer)
{
	return get_worker (file, name, size, buffer, 0);
}

/* Like gnome_metadata_get, but won't run the `file' command to
   characterize the file type.  Returns 0, or an error code.  */
int
gnome_metadata_get_fast (const char *file, const char *name,
			 int *size, char **buffer)
{
	return get_worker (file, name, size, buffer, 1);
}



#define RENAME 0
#define COPY   1
#define DELETE 2

/* Do the work to copy, rename, or delete.  */
static int
worker (const char *from, const char *to, int op)
{
	char **keys;
	int i;
	int ret = 0;

	/* FIXME: Use metadata_get_list here.  That will be more
	   efficient.  */

	keys = gnome_metadata_list (from);
	if (! keys)
		return 0;

	for (i = 0; keys[i]; ++i) {
		int size, r;
		char *buffer;

		/* It's more efficient this way.  */
		r = metadata_get_no_dup ("file", from, keys[i],
					 &size, &buffer);
		if (! r) {
			if (op == RENAME || op == COPY) {
				r = metadata_set ("file", to, keys[i],
						  size, buffer);
			}
			if (op == RENAME || op == DELETE) {
				int s = metadata_remove ("file", from,
							 keys[i]);
				if (! r)
					r = s;
			}
		}
		if (r) {
			ret = r;
		}
	}

	gnome_string_array_free (keys);

	return ret;
}

/* Convenience function.  Call this when a file is renamed.  Returns 0
   on success, or an error code.  */
int
gnome_metadata_rename (const char *from, const char *to)
{
	return worker (from, to, RENAME);
}

/* Convenience function.  Call this when a file is copied.  Returns 0
   on success, or an error code.  */
int
gnome_metadata_copy (const char *from, const char *to)
{
	return worker (from, to, COPY);
}

/* Convenience function.  Call this when a file is deleted.  Returns 0
   on success, or an error code.  */
int
gnome_metadata_delete (const char *file)
{
	return worker (file, NULL, DELETE);
}


/* Add a regular expression to the internal list.  This regexp is used
   when matching requests for the metadata KEY.  */
void
gnome_metadata_regexp_add (const char *regexp, const char *key,
			   int size, const char *data)
{
	metadata_set ("regexp", regexp, key, size, data);
}

/* Remove a regular expression from the internal list.  */
void
gnome_metadata_regexp_remove (const char *regexp, const char *key)
{
	metadata_remove ("regexp", regexp, key);
}

/* Add a file type to the internal list.  */
void
gnome_metadata_type_add (const char *type, const char *key,
			 int size, const char *data)
{
	metadata_set ("type", type, key, size, data);
}

/* Remove a file type from the internal list.  */
void
gnome_metadata_type_remove (const char *type, const char *key)
{
	metadata_remove ("type", type, key, size, data);
}
