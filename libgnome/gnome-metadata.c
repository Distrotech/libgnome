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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#ifdef HAVE_DB_185_H
# include <db_185.h>
#else
# include <db.h>
#endif

#include "libgnomeP.h"
#include "gnome-mime.h"


/* Each key in the database has 3 parts: the space, the object, and
   the "key" (confusingly named, I know).  The space is "type" for
   type maps, "file" for direct file maps, and "regex" for regular
   expressions.  The object is the type name, the file name, or the
   regex.  The key part is the actual metadata key to be used.

   For each file name we keep a special key `filelist\0<name>' whose
   associated data is a list of all the metadata keys we currently
   know about for this file.

   For each metadata key we keep a special db key `regexlist\0<key>'
   whose associated data is a list of all the regular expressions
   associated with that key.

   These special keys let us handle lookups more quickly.  */

/* TO DO:
   Fix all FIXME comments

   Let system provide a fallback database as well.  This would be used
   to let applications set their own icons by default, etc.  This
   would also be how the initial setup is primed

   Think about having a way to allow ordering of regexs

   We need a way to figure out when to remove metadata entries.  That
   is, some kind of GC.  We need this because metadata is kept
   per-user, so when a file is deleted (or whatever), we can't clear
   other users' metadata.  Not sure of the best solution here.

   One suggestion from the list was to have a way to specify a program
   (or "a piece of code") to run to service a metadata request.  This
   is a generalization of the `file' command code that currently
   exists.  I think this is probably a good idea, but I don't plan to
   do it initially.  */

/* PLEASE NOTE: if you fix a bug in this code, please add a test case
   to `metadata.c' in the test suite.  Please write the test case,
   verify that it fails, and then fix the bug.  Thanks in advance.  */



/* The database.  */
static DB *database;

/* The regular expression cache.  */
static GnomeRegexCache *rxcache;

/* The name of the database file.  This should only be set for testing
   or debugging.  It should not be mentioned in any header file.  */
char *gnome_metadata_db_file_name;

/* This is used to allow recursive locking.  */
static int lock_count;

/* The string and length used for `list' entries.  */
#define LIST "list"
#define LISTLEN 4




/* Initialize the database.  Returns 0 on success.  */
static int
init (void)
{
	char *filename;

	/* assert (! database); */
	if (gnome_metadata_db_file_name)
		filename = gnome_metadata_db_file_name;
	else
		filename = gnome_util_home_file ("metadata.db");
	database = dbopen (filename, O_CREAT | O_RDWR, 0700, DB_HASH, NULL);
	if (filename != gnome_metadata_db_file_name)
		g_free (filename);

	return database == NULL;
}

/* Lock the database.  */
static void
lock (void)
{
	if (! lock_count++) {
		int fd = database->fd (database);
		/* flock (fd, LOCK_EX); */
		struct flock lbuf;
		lbuf.l_type = F_WRLCK;
		lbuf.l_whence = SEEK_SET;
		lbuf.l_start = lbuf.l_len = 0L; /* Lock the whole file.  */
		fcntl (fd, F_SETLKW, &lbuf);
	}
}

/* Unlock the database.  */
static void
unlock (void)
{
	if (! --lock_count) {
		int fd;
		struct flock lbuf;
		database->sync (database, 0);
		fd = database->fd (database);
		/* flock (fd, LOCK_UN); */
		lbuf.l_type = F_UNLCK;
		lbuf.l_whence = SEEK_SET;
		lbuf.l_start = lbuf.l_len = 0L; /* Unlock the whole file.  */
		fcntl (fd, F_SETLKW, &lbuf);
	}
}

/* Set a piece of metadata.  */
static GnomeMetadataError_t
metadata_set (const char *space, const char *object, const char *key,
	      int size, const char *data)
{
	char *dbkey;
	int sl = strlen (space) + 1;
	int ol = strlen (object) + 1;
	int kl = strlen (key) + 1;
	int key_size = sl + ol + kl;
	int xlen;
	char *xobj, *xitem;
	DBT dkey, value;

	if (! database && init ())
		return GNOME_METADATA_IO_ERROR;

	/* We make it big enough to hold the full key later.  */
	dbkey = alloca (key_size + LISTLEN);
	strcpy (dbkey, space);
	strcpy (dbkey + sl, object);
	strcpy (dbkey + sl + ol, key);

	dkey.data = dbkey;
	dkey.size = key_size;

	value.data = (void *) data;
	value.size = size;

	lock ();
	if (database->put (database, &dkey, &value, 0)) {
		unlock ();
		return GNOME_METADATA_IO_ERROR;
	}

	/* We don't have a special item for `type' entries.  */
	if (! strcmp (space, "type")) {
		unlock ();
		return 0;
	} else if (! strcmp (space, "regex")) {
		xobj = (char *) key;
		xlen = kl;
		xitem = (char *) object;
	} else {
		xobj = (char *) object;
		xlen = ol;
		xitem = (char *) key;
	}

	/* Now update the special list item.  */
	strcpy (dbkey, space);
	strcat (dbkey, LIST);
	strcpy (dbkey + sl + LISTLEN, xobj);

	dkey.data = dbkey;
	dkey.size = sl + LISTLEN + xlen;

	if (! database->get (database, &dkey, &value, 0)) {
		/* Update list with new info, if required.  */
		char *p, *end;

		end = (char *) value.data + value.size;
		p = value.data;
		while (p < end) {
			if (! strcmp (p, xitem))
				break;
			p += strlen (p) + 1;
		}
		if (p == end) {
			/* Not found, so add it.  */
			int len = strlen (xitem) + 1;
			char *n = malloc (value.size + len);
			memcpy (n, value.data, value.size);
			strcpy (n + value.size, xitem);

			value.data = n;
			value.size += len;

			database->put (database, &dkey, &value, 0);
			free (n);
		}
	} else {
		/* Not found, so set initial value.  */
		value.size = strlen (xitem) + 1;
		value.data = (void *) xitem;
		database->put (database, &dkey, &value, 0);
	}

	unlock ();
	return 0;
}

/* Remove a piece of metadata.  */
static GnomeMetadataError_t
metadata_remove (const char *space, const char *object, const char *key)
{
	char *dbkey;
	int sl = strlen (space) + 1;
	int ol = strlen (object) + 1;
	int kl = strlen (key) + 1;
	int key_size = sl + ol + kl;
	int xlen;
	char *xobj, *xitem;
	DBT dkey, value;

	if (! database && init ())
		return GNOME_METADATA_IO_ERROR;

	dbkey = alloca (key_size + LISTLEN);
	strcpy (dbkey, space);
	strcpy (dbkey + sl, object);
	strcpy (dbkey + sl + ol, key);

	dkey.data = dbkey;
	dkey.size = key_size;

	lock ();
	if (database->del (database, &dkey, 0)) {
		unlock ();
		return GNOME_METADATA_IO_ERROR;
	}

	/* We don't have a special item for `type' entries.  */
	if (! strcmp (space, "type")) {
		unlock ();
		return 0;
	} else if (! strcmp (space, "regex")) {
		xobj = (char *) key;
		xlen = kl;
		xitem = (char *) object;
	} else {
		xobj = (char *) object;
		xlen = ol;
		xitem = (char *) key;
	}

	/* Now update the special list item.  */
	strcpy (dbkey, space);
	strcat (dbkey, LIST);
	strcpy (dbkey + sl + LISTLEN, xobj);

	dkey.data = dbkey;
	dkey.size = sl + LISTLEN + xlen;

	if (! database->get (database, &dkey, &value, 0)) {
		/* Found the item, so update it.  */
		char *p, *end;

		p = value.data;
		end = p + value.size;
		while (p < end) {
			if (! strcmp (p, xitem))
				break;
			p += strlen (p) + 1;
		}

		if (p != end) {
			/* Remove it.  */
			int l = strlen (p) + 1;

			if ((int) value.size == l) {
				/* Remove entry entirely.  */
				database->del (database, &dkey, 0);
			} else {
				/* Just remove a single item from list.  */
				char *n = p + l;
				char *dd = (char *) value.data;
				int delta = value.size - l - (p - dd);

				value.size -= l;

				memmove (p, n, delta);
				database->put (database, &dkey, &value, 0);
			}
		}
	}

	unlock ();
	return 0;
}

static GnomeMetadataError_t
metadata_get_list (const char *space, const char *object, DBT *value)
{
	char *dbkey;
	int sl = strlen (space) + 1;
	int ol = strlen (object) + 1;
	int key_size = sl + ol + LISTLEN, r;
	DBT key;

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcat (dbkey, LIST);
	strcpy (dbkey + sl + LISTLEN, object);

	key.data = dbkey;
	key.size = key_size;

	lock ();
	r = database->get (database, &key, value, 0);
	unlock ();
	return r ? GNOME_METADATA_IO_ERROR : 0;
}

static GnomeMetadataError_t
metadata_get_no_dup (const char *space, const char *object, const char *key,
		     int *size, char **buffer)
{
	char *dbkey;
	int sl = strlen (space) + 1;
	int ol = strlen (object) + 1;
	int kl = strlen (key) + 1;
	int key_size = sl + ol + kl, r;
	DBT dkey, value;

	if (! database && init ())
		return GNOME_METADATA_IO_ERROR;

	dbkey = alloca (key_size);
	strcpy (dbkey, space);
	strcpy (dbkey + sl, object);
	strcpy (dbkey + sl + ol, key);

	dkey.data = dbkey;
	dkey.size = key_size;

	/* If we fail we want *BUFFER to be NULL.  */
	*buffer = NULL;
	lock ();
	r = database->get (database, &dkey, &value, 0);
	unlock ();
	if (r)
		return GNOME_METADATA_IO_ERROR;

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
   which a regex or other match succeeds.  */
char **
gnome_metadata_list (const char *file)
{
	DBT value;
	int num, i;
	char **result, *p, *dd;

	if (metadata_get_list ("file", file, &value))
		return NULL;

	/* Count number of strings in data value.  */
	dd = (char *) value.data;
	for (num = i = 0; i < (int) value.size; ++i) {
		if (! dd[i])
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
try_regexs (const char *file, const char *key, int *size, char **buffer)
{
	DBT value;
	char *p, *end;

	if (metadata_get_list ("regex", key, &value))
		return 1;

	if (! rxcache)
		rxcache = gnome_regex_cache_new ();

	p = value.data;
	end = (char *) value.data + value.size;
	while (p < end) {
		regex_t *buf;
		buf = gnome_regex_cache_compile (rxcache, p, REG_EXTENDED);
		if (buf && ! regexec (buf, file, 0, 0, 0)) {
			return metadata_get ("regex", p, key, size, buffer);
		}
		p += strlen (p) + 1;
	}

	return 1;
}

/* Run the `file' command and use its output to determine the file's
   type.  Return NULL if no match, or malloc'd type name on success.  */
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
	if (! try_regexs (file, name, size, buffer))
		return 0;

	/* Phase 3: see if `type' is set.  */
	if (! metadata_get ("file", file, "type", &type_size, &type)) {
		/* Found the type.  */
		goto got_type;
	}

	/* See if regular expression can characterize file type.  */
	if (! try_regexs (file, "type", &type_size, &type)) {
		goto got_type;
	}

	/* See if `mime.types' has the answer.  */
	type = gnome_mime_type_or_default (file, NULL);
	if (type) {
		type = strdup (type);
		goto got_type;
	}

	/* If slow, try `file' command.  Otherwise, we've failed to
	   find the info.  */
	if (! is_fast)
		type = run_file (file);
	if (! type)
		return GNOME_METADATA_NOT_FOUND;

got_type:
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
	int r;
	lock ();
	r = get_worker (file, name, size, buffer, 0);
	unlock ();
	return r;
}

/* Like gnome_metadata_get, but won't run the `file' command to
   characterize the file type.  Returns 0, or an error code.  */
int
gnome_metadata_get_fast (const char *file, const char *name,
			 int *size, char **buffer)
{
	int r;
	lock ();
	r = get_worker (file, name, size, buffer, 1);
	unlock ();
	return r;
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

	lock ();

	/* Could use metadata_get_list here.  That would be more
	   efficient, but perhaps messier.  */

	keys = gnome_metadata_list (from);
	if (! keys)
		return 0;

	for (i = 0; keys[i]; ++i) {
		int size, r;
		char *buffer;

		/* It's more efficient to use _no_dup.  */
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

	unlock ();

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


/* Add a regular expression to the internal list.  This regex is used
   when matching requests for the metadata KEY.  */
void
gnome_metadata_regex_add (const char *regex, const char *key,
			   int size, const char *data)
{
	metadata_set ("regex", regex, key, size, data);
}

/* Remove a regular expression from the internal list.  */
void
gnome_metadata_regex_remove (const char *regex, const char *key)
{
	metadata_remove ("regex", regex, key);
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
	metadata_remove ("type", type, key);
}
