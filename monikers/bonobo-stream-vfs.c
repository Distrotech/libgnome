/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * gnome-stream-vfs.c: Gnome VFS based stream implementation
 *
 * Author:
 *   Michael Meeks <michael@helixcode.com>
 *
 * Copyright 2000, Helix Code Inc.
 */
#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include "bonobo-stream-vfs.h"
#include <errno.h>

static BonoboStreamClass *bonobo_stream_vfs_parent_class;

void
bonobo_stream_vfs_storageinfo_from_file_info (Bonobo_StorageInfo *si,
					      GnomeVFSFileInfo   *fi)
{
	g_return_if_fail (si != NULL);
	g_return_if_fail (fi != NULL);

	si->name = CORBA_string_dup (fi->name);

	if (fi->flags & GNOME_VFS_FILE_INFO_FIELDS_SIZE)
		si->size = fi->size;
	else
		si->size = 0;

	if (fi->flags & GNOME_VFS_FILE_INFO_FIELDS_TYPE &&
	    fi->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		si->type = Bonobo_STORAGE_TYPE_DIRECTORY;
	else
		si->type = Bonobo_STORAGE_TYPE_REGULAR;

	if (fi->flags & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE &&
	    fi->mime_type)
		si->content_type = CORBA_string_dup (fi->mime_type);
	else
		si->content_type = CORBA_string_dup ("");
}

static Bonobo_StorageInfo *
vfs_get_info (BonoboStream                   *stream,
	      const Bonobo_StorageInfoFields  mask,
	      CORBA_Environment              *ev)
{
	BonoboStreamVfs    *sfs = BONOBO_STREAM_VFS (stream);
	Bonobo_StorageInfo *si;
	GnomeVFSFileInfo    fi;
	GnomeVFSResult      result;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	gnome_vfs_file_info_init (&fi);
	result = gnome_vfs_get_file_info_from_handle (
		sfs->handle, &fi,
		(mask & Bonobo_FIELD_CONTENT_TYPE) ?
		GNOME_VFS_FILE_INFO_GET_MIME_TYPE :
		GNOME_VFS_FILE_INFO_DEFAULT);

	if (result != GNOME_VFS_OK) {
		if (result == GNOME_VFS_ERROR_ACCESS_DENIED)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_NoPermission, NULL);
		else
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_IOError, NULL);
		return NULL;
	}

	si = Bonobo_StorageInfo__alloc ();

	bonobo_stream_vfs_storageinfo_from_file_info (si, &fi);

	gnome_vfs_file_info_clear (&fi);

	return si;
}

static void
vfs_set_info (BonoboStream                   *stream,
	      const Bonobo_StorageInfo       *info,
	      const Bonobo_StorageInfoFields  mask,
	      CORBA_Environment              *ev)
{
	g_warning ("FIXME: set_info: a curious and not yet implemented API");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_write (BonoboStream *stream,
	   const Bonobo_Stream_iobuf *buffer,
	   CORBA_Environment *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSResult   result;
	GnomeVFSFileSize bytes_written;

	do {
		result = gnome_vfs_write (sfs->handle, buffer->_buffer,
					  buffer->_length, &bytes_written);
	} while (bytes_written < 1 && result == GNOME_VFS_ERROR_INTERRUPTED);

	if (result != GNOME_VFS_OK)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
}

static void
vfs_read (BonoboStream *stream, CORBA_long count,
	  Bonobo_Stream_iobuf **buffer,
	  CORBA_Environment    *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSResult   result;
	GnomeVFSFileSize bytes_read;
	CORBA_octet     *data;

	*buffer = Bonobo_Stream_iobuf__alloc ();
	CORBA_sequence_set_release (*buffer, TRUE);

	data = CORBA_sequence_CORBA_octet_allocbuf (count);

	do {
		result = gnome_vfs_read (sfs->handle, data,
					 count, &bytes_read);
	} while (bytes_read < 1 && result == GNOME_VFS_ERROR_INTERRUPTED);

	if (result == GNOME_VFS_ERROR_EOF) {
		(*buffer)->_length = 0;
		(*buffer)->_buffer = NULL;
		CORBA_free (data);
	} else if (result != GNOME_VFS_OK) {
		CORBA_free (data);
		CORBA_free (*buffer);
		*buffer = NULL;
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
	} else {
		(*buffer)->_buffer = data;
		(*buffer)->_length = bytes_read;
	}
}

static CORBA_long
vfs_seek (BonoboStream *stream,
	  CORBA_long offset, Bonobo_Stream_SeekType whence,
	  CORBA_Environment *ev)
{
	BonoboStreamVfs     *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSSeekPosition pos;
	GnomeVFSResult       result;
	GnomeVFSFileOffset   where;
	
	switch (whence) {
	case Bonobo_Stream_SEEK_CUR:
		pos = GNOME_VFS_SEEK_CURRENT;
		break;
	case Bonobo_Stream_SEEK_END:
		pos = GNOME_VFS_SEEK_END;
		break;
	case Bonobo_Stream_SEEK_SET:
		pos = GNOME_VFS_SEEK_START;
		break;
	default:
		g_warning ("Seek whence %d unknown; fall back to SEEK_SET",
			   whence);
		pos = GNOME_VFS_SEEK_START;
		break;
	}
	
	result = gnome_vfs_seek (sfs->handle, pos, offset);

	if (result != GNOME_VFS_OK) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
		return -1;
	}
       
	result = gnome_vfs_tell (sfs->handle, &where);
	
	if (result != GNOME_VFS_OK) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
		return -1;
	}

	return where;
}

static void
vfs_truncate (BonoboStream *stream,
	      const CORBA_long new_size, 
	      CORBA_Environment *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSResult result;

	result = gnome_vfs_truncate_handle (sfs->handle, new_size);
	if (result == GNOME_VFS_OK)
		return;

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NoPermission, NULL);
}

static void
vfs_copy_to  (BonoboStream      *stream,
	      const CORBA_char  *dest,
	      const CORBA_long  bytes,
	      CORBA_long       *read_bytes,
	      CORBA_long       *written_bytes,
	      CORBA_Environment *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
#define READ_CHUNK_SIZE 65536
	CORBA_octet      data [READ_CHUNK_SIZE];
	CORBA_unsigned_long more = bytes;
	GnomeVFSHandle  *fd_out;
	GnomeVFSResult   res;
	GnomeVFSFileSize rsize, wsize;

	*read_bytes = 0;
	*written_bytes = 0;

	res = gnome_vfs_create (&fd_out, dest, GNOME_VFS_OPEN_WRITE, FALSE, 0666);
	if (res != GNOME_VFS_OK) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_NoPermission, NULL);
		return;
	}

	do {
		if (bytes == -1) 
			more = READ_CHUNK_SIZE;

		res = gnome_vfs_read (sfs->handle, data,
				      MIN (READ_CHUNK_SIZE, more), &rsize);

		if (res == GNOME_VFS_OK) {
			*read_bytes += rsize;
			more -= rsize;
			res = gnome_vfs_write (fd_out, data, rsize, &wsize);
			if (res == GNOME_VFS_OK)
				*written_bytes += wsize;
			else
				break;
		} else
			rsize = 0;

	} while ((more > 0 || bytes == -1) && rsize > 0);

	if (res != GNOME_VFS_OK)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);

	gnome_vfs_close (fd_out);
}

static void
vfs_commit (BonoboStream *stream,
	    CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_revert (BonoboStream *stream,
	    CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}
	
static void
bonobo_stream_vfs_destroy (GtkObject *object)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (object);

	if (sfs->handle)
		if (gnome_vfs_close (sfs->handle) != GNOME_VFS_OK)
			g_warning ("VFS Close failed");

	sfs->handle = NULL;

	GTK_OBJECT_CLASS (bonobo_stream_vfs_parent_class)->destroy (object);	
}

static void
bonobo_stream_vfs_class_init (BonoboStreamVfsClass *klass)
{
	BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	GtkObjectClass    *object_class = GTK_OBJECT_CLASS (klass);
	
	bonobo_stream_vfs_parent_class = gtk_type_class (bonobo_stream_get_type ());

	sclass->get_info = vfs_get_info;
	sclass->set_info = vfs_set_info;
	sclass->write    = vfs_write;
	sclass->read     = vfs_read;
	sclass->seek     = vfs_seek;
	sclass->truncate = vfs_truncate;
	sclass->copy_to  = vfs_copy_to;
	sclass->commit   = vfs_commit;
	sclass->commit   = vfs_revert;

	object_class->destroy = bonobo_stream_vfs_destroy;
}

/**
 * bonobo_stream_vfs_get_type:
 *
 * Returns the GtkType for the BonoboStreamVfs class.
 */
GtkType
bonobo_stream_vfs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboStreamVfs",
			sizeof (BonoboStreamVfs),
			sizeof (BonoboStreamVfsClass),
			(GtkClassInitFunc) bonobo_stream_vfs_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_stream_get_type (), &info);
	}

	return type;
}

static BonoboStream *
bonobo_stream_create (GnomeVFSHandle *handle)
{
	BonoboStreamVfs *stream_vfs;
	Bonobo_Stream corba_stream;

	stream_vfs = gtk_type_new (bonobo_stream_vfs_get_type ());
	if (stream_vfs == NULL)
		return NULL;
	
	stream_vfs->handle = handle;
	
	corba_stream = bonobo_stream_corba_object_create (
		BONOBO_OBJECT (stream_vfs));

	if (corba_stream == CORBA_OBJECT_NIL){
		bonobo_object_unref (BONOBO_OBJECT (stream_vfs));
		return NULL;
	}

	return BONOBO_STREAM (
		bonobo_object_construct (
			BONOBO_OBJECT (stream_vfs), corba_stream));
}


/**
 * bonobo_stream_vfs_open:
 * @path: The path to the file to be opened.
 * @mode: The mode with which the file should be opened.
 *
 * Creates a new BonoboStream object for the filename specified by
 * @path.  
 */
BonoboStream *
bonobo_stream_vfs_open (const char *path, gint flags, gint mode,
			CORBA_Environment *ev)
{
	GnomeVFSResult   result;
	GnomeVFSHandle  *handle;
	GnomeVFSOpenMode vfs_mode = GNOME_VFS_OPEN_NONE;

	g_return_val_if_fail (path != NULL, NULL);

	if (mode == Bonobo_Storage_READ)
		vfs_mode |= GNOME_VFS_OPEN_READ;

	else if (mode == Bonobo_Storage_WRITE)
		vfs_mode |= GNOME_VFS_OPEN_WRITE;
	
	else {
		g_warning ("Unhandled open mode %d", mode);
		return NULL;
	}
	
	result = gnome_vfs_open (&handle, path, vfs_mode);
	if (vfs_mode & GNOME_VFS_OPEN_WRITE &&
	    result == GNOME_VFS_ERROR_NOT_FOUND)
		result = gnome_vfs_create (&handle, path, vfs_mode,
					   FALSE, S_IRUSR | S_IWUSR);
	
	if (result != GNOME_VFS_OK)
		return NULL;

	return bonobo_stream_create (handle);
}
