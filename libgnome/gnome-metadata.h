/* gnome-metadata.h - Declarations for metadata module.

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

#ifndef GNOME_METADATA_H
#define GNOME_METADATA_H

BEGIN_GNOME_DECLS


/* We have a fairly limited notion of what an error is.  We could pass
   on errors that the underlying database reports to us, but I'd
   prefer to hide this information in case we change the
   implementation later.  We probably don't need very sophisticated
   error handling here anyway.  */
typedef enum
{
	GNOME_METADATA_OK = 0,		   /* No error. */
	GNOME_METADATA_IO_ERROR,	   /* IO or other low-level
					      communications/storage
					      error.  */
	GNOME_METADATA_NOT_FOUND	   /* Information not found.  */
} GnomeMetadataError_t;

/* Set metadata associated with FILE.  Returns 0 on success, or an
   error code.  */
int gnome_metadata_set (const char *file, const char *name,
			int size, const char *data);

/* Remove a piece of metadata associated with FILE.  Returns 0 on
   success, or an error code.  */
int gnome_metadata_remove (const char *file, const char *name);

/* Return an array of all metadata keys associated with FILE.  The
   array is NULL terminated.  The result value can be freed with
   g_strfreev.  This only returns keys for which there is
   a particular association with FILE.  It will not return keys for
   which a regex or other match succeeds.  */
char **gnome_metadata_list (const char *file);

/* Get a piece of metadata associated with FILE.  SIZE and BUFFER are
   result parameters.  *BUFFER is malloc()d.  Returns 0, or an error
   code.  On error *BUFFER will be set to NULL.  */
int gnome_metadata_get (const char *file, const char *name,
			int *size, char **buffer);

/* Like gnome_metadata_get, but won't run the `file' command to
   characterize the file type.  Returns 0, or an error code.  */
int gnome_metadata_get_fast (const char *file, const char *name,
			     int *size, char **buffer);


/* Convenience function.  Call this when a file is renamed.  Returns 0
   on success, or an error code.  */
int gnome_metadata_rename (const char *from, const char *to);

/* Convenience function.  Call this when a file is copied.  Returns 0
   on success, or an error code.  */
int gnome_metadata_copy (const char *from, const char *to);

/* Convenience function.  Call this when a file is deleted.  Returns 0
   on success, or an error code.  */
int gnome_metadata_delete (const char *file);


/* Add a regular expression to the internal list.  This regex is used
   when matching requests for the metadata KEY.  */
void gnome_metadata_regex_add (const char *regex, const char *key,
			       int size, const char *data);

/* Remove a regular expression from the internal list.  */
void gnome_metadata_regex_remove (const char *regex, const char *key);


/* Add a file type to the internal list.  */
void gnome_metadata_type_add (const char *type, const char *key,
			      int size, const char *data);

/* Remove a file type from the internal list.  */
void gnome_metadata_type_remove (const char *type, const char *key);

void gnome_metadata_lock (void);
void gnome_metadata_unlock (void);
END_GNOME_DECLS

#endif /* GNOME_METADATA_H */
