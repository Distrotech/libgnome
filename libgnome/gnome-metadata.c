/* gnome-metadata.c - Metadata implementation.

   Copyright (C) 1998, 1999 Tom Tromey

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

/* compatibility define needed for 'gcc -ansi -pedantic' on db.h */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_DB_185_H
# include <db_185.h>
#else
# include <db.h>
#endif

#include "libgnomeP.h"
#include "gnome-mime.h"
#include "gnome-regex.h"
#ifdef NEED_GNOMESUPPORT_H
#include "gnomesupport.h"
#endif

#define ZERO(Dbt) memset (&(Dbt), sizeof (DBT), 0)

#if !defined getc_unlocked && !defined HAVE_GETC_UNLOCKED
# define getc_unlocked(fp) getc (fp)
#endif

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
   or debugging.  It should not be mentioned in any header file.
   However, it must not be `static'.  */
char *gnome_metadata_db_file_name;

/* This is used to allow recursive locking.  */
static int lock_count;

/* Name of directory to create when locking.  */
static char *lock_directory;

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
	lock_directory = gnome_util_home_file ("metadata.lock");

	return database == NULL;
}

/* Lock the database.  */
static void
lock (void)
{
	struct stat time1, time2;
	
	if (! lock_count++) {
		int attempts;
		
		/* We use a lock directory and not flock/fcntl because
		   we want this to work when the database is on an NFS
		   filesystem.  Sigh.  */

		attempts = 0;
		while (mkdir (lock_directory, 0)) {
			attempts++;
			if (errno != EEXIST) {
				/* Don't know what to do here.  */
				return;
			}
			/* The utter lameness of this is without
			   question.  FIXME: at least use usleep to
			   try to sleep less than a whole second.
			   Typical db access times will be small.  */
			stat (lock_directory, &time1);
			sleep (1);
			stat (lock_directory, &time2);

			if (time1.st_ctime != time2.st_ctime)
				attempts = 0;
			
			if (attempts > 30){
				break;
			}
		}
	}
}

/* Unlock the database.  */
static void
unlock (void)
{
	if (! --lock_count) {
		database->sync (database, 0);
		rmdir (lock_directory);
	}
}

/**
 * gnome_metadata_lock:
 *
 * Locks the metadata system.  Used if you are going to invoke
 * many metadata operations to speed up metadata access.
 */
void
gnome_metadata_lock ()
{
	if (! database)
		init ();
	lock ();
}

/**
 * gnome_metadata_unlock:
 *
 * Unlocks the metadata system.  Used if you are going to invoke
 * many metadata operations to speed up metadata access.
 */
void
gnome_metadata_unlock ()
{
	unlock ();
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

	ZERO (dkey);
	ZERO (value);

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
			char *n = g_malloc (value.size + len);
			memcpy (n, value.data, value.size);
			strcpy (n + value.size, xitem);

			value.data = n;
			value.size += len;

			database->put (database, &dkey, &value, 0);
			g_free (n);
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
				/* Just remove a single item from
				   list.  We have to allocate a new
				   item because reusing a db return
				   value is wrong.  */
				char *n = p + l;
				char *dd = (char *) value.data;
				int delta = value.size - l - (p - dd);
				char *newv = alloca (value.size - l);

				memmove (newv, dd, p - dd);
				memmove (newv + (p - dd), n, delta);

				value.size -= l;
				value.data = newv;

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

	if (!database && init ())
		return GNOME_METADATA_IO_ERROR;
	
	ZERO (key);

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

	ZERO (dkey);
	ZERO (value);

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
		char *n = g_malloc (*size);
		memcpy (n, *buffer, *size);
		*buffer = n;
	}
	return r;
}



/*
 * This section deals with reading the files that the application
 * installs.
 */

/* Last time that application directory was modified.  If 0, then
   assume directory has never been read.  */
static time_t app_dir_mtime;

/* Name of directory holding application-installed metadata.  This
   should never be set except when debugging or testing.  However, it
   must not be static, and it must never be mentioned in any header.  */
char *gnome_metadata_app_dir;

/* A key/value pair.  */
struct kv
{
	char *key;
	char *value;
};

/* A structure of this type is used as the value in the regular
   expression hash.  This is also used as the value in the type hash,
   but in that case the `type_set' entry has no meaning.  */
struct app_ent
{
	gboolean type_set;	/* True if `type' key is in our key
				   list.  */
	GSList *mappings;	/* List of key/value pairs.  */
};

/* This hash table is used to map regular expressions onto key/value
   lists.  */
static GHashTable *app_rx_hash;

/* This hash table is used to map type names onto key/value lists.  */
static GHashTable *app_type_hash;

/* Add a new key/value pair to a hash table.  */
static void
add_hash_entry (GHashTable *hash, char *hashkey, char *key, char *value)
{
	struct app_ent *ent;
	GSList *list;
	struct kv *newpair;

	ent = (struct app_ent *) g_hash_table_lookup (hash, hashkey);
	if (! ent) {
		ent = g_malloc (sizeof (struct app_ent));
		ent->type_set = 0;
		ent->mappings = NULL;
		g_hash_table_insert (hash, hashkey, ent);
	}

	for (list = ent->mappings; list; list = list->next) {
		struct kv *pair = (struct kv *) list->data;
		if (! strcmp (pair->key, key)) {
			g_free (pair->value);
			pair->value = g_strdup (value);
			return;
		}
	}

	newpair = (struct kv *) g_malloc (sizeof (struct kv));
	newpair->key = g_strdup (key);
	newpair->value = g_strdup (value);
	ent->mappings = g_slist_prepend (ent->mappings, newpair);

	if (! strcmp (key, "type"))
		ent->type_set = 1;
}

/* Keys for headers in app files.  */
#define TYPE "type:"
#define TYPE_LEN 5
#define REGEX "regex:"
#define REGEX_LEN 6

/* This is called once per file in the application directory.  It
   reads the file and puts the information into the global
   structures.  */
static int
scan_app_file (struct dirent *ent)
{
	FILE *f;
	int c, comment, column, was_space, skipping, equals;
	static GString *line;
	GHashTable *current_hash = NULL;
	char *current_key = NULL;
	char *filename;

	if (! strcmp (ent->d_name, ".") || ! strcmp (ent->d_name, ".."))
		return 0;

	filename = g_concat_dir_and_file (gnome_metadata_app_dir,
					  ent->d_name);
	f = fopen (filename, "r");
	g_free (filename);
	/* We just don't care about errors.  */
	if (! f)
		return 0;

	if (! line)
		line = g_string_sized_new (100);
	equals = comment = column = was_space = skipping = 0;
	while ((c = getc_unlocked (f)) != EOF) {
		if (c == '\r')
			continue;

		if (c == '\n') {
			if (comment) {
				goto reset;
			}

			if (was_space) {
				/* Had a key=value entry.  */
				if (! current_hash || ! current_key
				    || ! equals) {
					/* FIXME: print an error
					   here.  */
				} else {
					line->str[equals] = '\0';
					add_hash_entry (current_hash,
							current_key,
							line->str,
							&line->str[equals+1]);
				}
			} else {
				/* Had start of new block.  */
				int start;
				GHashTable *save = current_hash;
				if (! strncmp (line->str, TYPE, TYPE_LEN)) {
					current_hash = app_type_hash;
					start = TYPE_LEN;
				} else if (! strncmp (line->str, REGEX,
						      REGEX_LEN)) {
					current_hash = app_rx_hash;
					start = REGEX_LEN;
				} else {
					/* FIXME: print error here.  */
					goto reset;
				}

				while (line->str[start]
				       && isspace (line->str[start])) {
					++start;
				}

				if (line->str[start]) {
					/* This isn't a leak: the keys
					   are kept by the hash.  */
					current_key
						= g_strdup (&line->str[start]);
				} else {
					current_hash = save;
				}
			}

		reset:
			g_string_truncate (line, 0);
			equals = comment = column = was_space = skipping = 0;

		} else if (! comment) {
			if (isspace (c)) {
				if (column == 0) {
					was_space = 1;
					skipping = 1;
					continue;
				} else if (skipping) {
					continue;
				}
			}
			if (column == 0 && c == '#') {
				comment = 1;
			} else {
				if (c == '=' && ! equals) {
					equals = column;
				}
				skipping = 0;
				line = g_string_append_c (line, c);
			}
			++column;
		}
	}

	fclose (f);

	return 0;
}

/* This is called to free a key/value pair.  */
static void
free_mapping (gpointer data, gpointer user_data)
{
	struct kv *m = (struct kv *) data;
	g_free (m->key);
	g_free (m->value);
	g_free (m);
}

/* This is called to free a hash entry.  */
static void
free_hash_entry (gpointer key, gpointer value, gpointer user_data)
{
	struct app_ent *ent = (struct app_ent *) value;
	g_slist_foreach (ent->mappings, free_mapping, NULL);
	g_slist_free (ent->mappings);
	g_free (ent);
	g_free (key);
}

/* If the application install directory has changed, throw away our
   current application data and rescan.  */
static void
maybe_scan_app_dir (void)
{
	struct stat sb;
	struct dirent **list;

	if (! gnome_metadata_app_dir)
		gnome_metadata_app_dir
			= gnome_unconditional_datadir_file ("metadata");

	/* If the stat fails, or if we've read the directory at some
	   point in the past, just return.  */
	if (stat (gnome_metadata_app_dir, &sb)
	    || (app_dir_mtime && sb.st_mtime <= app_dir_mtime)) {
		return;
	}
	app_dir_mtime = sb.st_mtime;

	if (app_rx_hash) {
		g_hash_table_foreach (app_rx_hash, free_hash_entry, NULL);
		g_hash_table_destroy (app_rx_hash);
	}
	app_rx_hash = g_hash_table_new (g_str_hash, g_str_equal);

	if (app_type_hash) {
		g_hash_table_foreach (app_type_hash, free_hash_entry, NULL);
		g_hash_table_destroy (app_type_hash);
	}
	app_type_hash = g_hash_table_new (g_str_hash, g_str_equal);

	if (scandir (gnome_metadata_app_dir, &list, scan_app_file, alphasort) != -1)
		if (list)
			g_free (list);
}

/* This is set if we've already found a suitable regex match.  It lets
   us short-circuit further attempts.  This variable is evil, but not
   as evil as the alternative, which is longjmp.  */
static char *short_circuit = NULL;

/* Another evil global: the name of the key we're searching for.  We
   can only conveniently pass in one variable to the hash foreach
   function.  */
static char *desired_key;

/* The final (I hope) evil global: true if we're searching for the
   `type' key.  We optimize this case a little since we expect it to
   happen frequently.  */
static int type_desired;

/* This is called for each hash table entry.  If the regular
   expression match succeeds, we search for the desired key.  If that
   succeeds, we set SHORT_CIRCUIT to the value.  */
static void
try_one_app_regex (gpointer key, gpointer value, gpointer user_data)
{
	char *rx_text = (char *) key;
	char *filename = (char *) user_data;
	regex_t *rx;
	GSList *list;
	struct app_ent *ent = (struct app_ent *) value;

	static GnomeRegexCache *app_rx_cache = NULL;

	/* Already found the answer, just marking time now.  Or,
	   searching for type and this entry doesn't have it.  */
	if (short_circuit || (type_desired && ! ent->type_set))
		return;

	if (! app_rx_cache)
		app_rx_cache = gnome_regex_cache_new ();

	rx = gnome_regex_cache_compile (app_rx_cache, rx_text, REG_EXTENDED);
	if (! rx || regexec (rx, filename, 0, 0, 0))
		return;

	ent = (struct app_ent *) value;
	for (list = ent->mappings; list; list = list->next) {
		struct kv *pair = (struct kv *) list->data;
		if (! strcmp (pair->key, desired_key)) {
			short_circuit = pair->value;
			return;
		}
	}
}

/* Run all application-specified regular expressions associated with
   KEY.  Return 0 on success.  */
/* FIXME: if we were smarter, we would cache some of the results from
   this call, since the regexps that match might be reused if we have
   to fall back on the type.  We could do this by adding some calls to
   let us manipulate how the regex cache works.  */
static int
try_app_regexs (const char *file, const char *key, int *size, char **buffer)
{
	maybe_scan_app_dir ();

	short_circuit = NULL;
	desired_key = (char *) key;
	type_desired = ! strcmp (key, "type");

	if (app_rx_hash)
		g_hash_table_foreach (app_rx_hash, try_one_app_regex,
				      (gpointer) file);

	if (! short_circuit)
		return GNOME_METADATA_NOT_FOUND;

	*size = strlen (short_circuit) + 1;
	*buffer = g_strdup (short_circuit);
	return 0;
}

/* See if there is some data associated with TYPE and KEY in the
   application database.  */
static int
app_get_by_type (const char *type, const char *key, int *size, char **buffer)
{
	struct app_ent *ent;
	GSList *list;

	maybe_scan_app_dir ();

	if (!app_type_hash)
		return GNOME_METADATA_NOT_FOUND;
	
	ent = (struct app_ent *) g_hash_table_lookup (app_type_hash, type);
	if (! ent)
		return GNOME_METADATA_NOT_FOUND;

	for (list = ent->mappings; list != NULL; list = list->next) {
		struct kv *pair = (struct kv *) list->data;
		if (! strcmp (pair->key, key)) {
			*size = strlen (pair->value) + 1;
			*buffer = g_strdup (pair->value);
			return 0;
		}
	}

	return GNOME_METADATA_NOT_FOUND;
}



/**
 * gnome_metadata_set:
 * @file: File with which metadata will be associated
 * @name: Metadata key.
 * @size: Size in bytes of data
 * @data: Data to be stored.
 *
 * Sets metadata associated with @file and @name.
 *
 * Returns %0 on success or an error code.
 */
int
gnome_metadata_set (const char *file, const char *name,
		    int size, const char *data)
{
	return metadata_set ("file", file, name, size, data);
}

/**
 * gnome_metadata_remove:
 * @file: File name
 * @name: Metadata key.
 *
 * Remove a piece of metadata associated with @file.
 *
 * Returns %0 on success, or an error code.
 */
int
gnome_metadata_remove (const char *file, const char *name)
{
	return metadata_remove ("file", file, name);
}

/**
 * gnome_metadata_list
 * @file: File name.
 *
 * Returns an array of all metadata keys associated with @file.  The
 * array is %NULL terminated.  The result can be freed with
 * g_strfreev().  This only returns keys for which there
 * is a particular association with @file.  It will not return keys
 * for which a regex or other match succeeds.
 */
char **
gnome_metadata_list (const char *file)
{
	DBT value;
	int num, i;
	char **result, *p, *dd;

	if (! database && init ())
		return NULL;
	
	ZERO (value);

	if (metadata_get_list ("file", file, &value))
		return NULL;

	/* Count number of strings in data value.  */
	dd = (char *) value.data;
	for (num = i = 0; i < (int) value.size; ++i) {
		if (! dd[i])
			++num;
	}

	/* Allocate one extra slot for trailing NULL.  */
	result = (char **) g_malloc ((num + 1) * sizeof (char *));

	p = value.data;
	for (i = 0; i < num; ++i) {
		int len = strlen (p);
		result[i] = g_strdup (p);
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

	ZERO (value);

	if (metadata_get_list ("regex", key, &value))
		return GNOME_METADATA_NOT_FOUND;

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

	return GNOME_METADATA_NOT_FOUND;
}

/* Run the `file' command and use its output to determine the file's
   type.  Return NULL if no match, or g_malloc'd type name on success.  */
static char *
run_file (const char *file)
{
#if 0
  /* This is what the code will look like once the magic functions are
     ready.  */
	const char *type = gnome_mime_type_from_magic (file);
	return type ? g_strdup (type) : NULL;
#else
	return NULL;
#endif
}

/* Do all the work for _get and _get_fast.  */
static int
get_worker (const char *file, const char *name, int *size, char **buffer,
	    int is_fast)
{
	int type_size, r;
	int is_type = 0;
	gchar *type;
	const gchar *type_c;

	/* Phase 1: see if data exists in database.  */
	if (! metadata_get ("file", file, name, size, buffer))
		return 0;

	/* Phase 2: see if a direct regular expression matches the
	   file name.  */
	if (! try_regexs (file, name, size, buffer))
		return 0;

	/* Phase 3: try a regular expression as installed by some
	   application.  */
	if (! try_app_regexs (file, name, size, buffer))
		return 0;

	if (! strcmp (name, "type")) {
		/* We're trying to fetch the type, so there's no point
		   trying the same requests again below.  */

		type_c = gnome_mime_type_or_default (file, NULL);
		if (! type_c) {
			if (is_fast)
				return GNOME_METADATA_NOT_FOUND;
			type = run_file (file);
			if (! type)
				return GNOME_METADATA_NOT_FOUND;
		} else {
			type = g_strdup (type_c);
		}
		*size = strlen (type) + 1;
		*buffer = type;
		return 0;
	}

	/* Phase 4: see if `type' is set.  */
	if (! metadata_get ("file", file, "type", &type_size, &type)) {
		/* Found the type.  */
		goto got_type;
	}

	/* See if regular expression can characterize file type.  */
	if (! try_regexs (file, "type", &type_size, &type)) {
		goto got_type;
	}

	/* Try application-installed information.  */
	if (! try_app_regexs (file, "type", &type_size, &type)) {
		goto got_type;
	}

	/* See if `mime.types' has the answer.  */
	type_c = gnome_mime_type_or_default (file, NULL);
	if (type_c) {
		type = g_strdup (type_c);
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
	if (r) {
		/* Finally, see if there is application-installed data
		   associated with the type.  */
		r = app_get_by_type (type, name, size, buffer);
	}
	g_free (type);
	return r;
}

/**
 * gnome_metadata_get:
 * @file: File name
 * @name: Metadata key
 * @size: Return parameter for size of data
 * @buffer: Return parameter for data
 *
 * Get a piece of metadata associated with @file.  @size and @buffer
 * are result parameters.  *@buffer is g_malloc()d.
 *
 * Returns %0, or an error code.  On error *@buffer will be set to %NULL.
 */
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

/**
 * gnome_metadata_get_fast:
 * @file: File name
 * @name: Metadata key
 * @size: Return parameter for size of data
 * @buffer: Return parameter for data
 *
 * Like gnome_metadata_get(), but won't run the `file' command to
 * characterize the file type.
 * 
 * Returns %0, or an error code.  On error *@buffer will be set to
 * %NULL.
 */
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
	if (! keys){
		unlock ();
		return 0;
	}

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

	g_strfreev (keys);

	return ret;
}

/**
 * gnome_metadata_rename:
 * @from: Source file name
 * @to: Destination file name
 *
 * This function moves metadata associated with file @from to file
 * @to.  It should be called after a file is renamed.
 *
 * Returns %0 on success, or an error code.
 */
int
gnome_metadata_rename (const char *from, const char *to)
{
	return worker (from, to, RENAME);
}

/**
 * gnome_metadata_copy:
 * @from: Source file name
 * @to: Destination file name
 *
 * This function copies metadata associated with file @from to file
 * @to.  It should be called after a file is copied.
 *
 * Returns %0 on success, or an error code.
 */
int
gnome_metadata_copy (const char *from, const char *to)
{
	return worker (from, to, COPY);
}

/**
 * gnome_metadata_delete:
 * @file: File name
 *
 * This function deletes all metadata associated with @file.
 * It should be called after a file is deleted.
 *
 * Returns %0 on success, or an error code.
 */
int
gnome_metadata_delete (const char *file)
{
	return worker (file, NULL, DELETE);
}


/**
 * gnome_metadata_regex_add:
 * @regex: The regular expression.
 * @key: The metadata key.
 * @size: Size of data in bytes.
 * @data: The data.
 *
 * Add a regular expression to the internal list.  This regex is used
 * when matching requests for the metadata @key.
 *
 */
void
gnome_metadata_regex_add (const char *regex, const char *key,
			   int size, const char *data)
{
	metadata_set ("regex", regex, key, size, data);
}

/**
 * gnome_metadata_regex_remove:
 * @regex: The regular expression.
 * @key: The metadata key.
 *
 * Remove the regular expression from the internal list.
 *
 */
void
gnome_metadata_regex_remove (const char *regex, const char *key)
{
	metadata_remove ("regex", regex, key);
}

/**
 * gnome_metadata_type_add:
 * @type: File type
 * @key: The metadata key.
 * @size: Size of data in bytes.
 * @data: The data.
 *
 * Add a file type to the internal list.  This pairing is used
 * when matching requests for the metadata @key.
 *
 */
void
gnome_metadata_type_add (const char *type, const char *key,
			 int size, const char *data)
{
	metadata_set ("type", type, key, size, data);
}

/**
 * gnome_metadata_type_remove:
 * @type: The file type.
 * @key: The metadata key.
 *
 * Remove a type/key pairing from the internal list.
 *
 */
void
gnome_metadata_type_remove (const char *type, const char *key)
{
	metadata_remove ("type", type, key);
}
