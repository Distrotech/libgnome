/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * gnome-stream-vfs.c: Gnome VFS based stream implementation
 *
 * Author:
 *   Michael Meeks <michael@helixcode.com>
 *
 * Copyright 2001, Ximian, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include "bonobo-stream-vfs.h"
#include <errno.h>

static BonoboObjectClass *bonobo_stream_vfs_parent_class;

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
vfs_get_info (PortableServer_Servant          stream,
	      const Bonobo_StorageInfoFields  mask,
	      CORBA_Environment              *ev)
{
	BonoboStreamVfs    *sfs = BONOBO_STREAM_VFS (
		bonobo_object (stream));
	Bonobo_StorageInfo *si;
	GnomeVFSFileInfo   *fi;
	GnomeVFSResult      result;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	fi = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info_from_handle (
		sfs->handle, fi,
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

	bonobo_stream_vfs_storageinfo_from_file_info (si, fi);

	gnome_vfs_file_info_unref (fi);

	return si;
}

static void
vfs_set_info (PortableServer_Servant          stream,
	      const Bonobo_StorageInfo       *info,
	      const Bonobo_StorageInfoFields  mask,
	      CORBA_Environment              *ev)
{
	g_warning ("FIXME: set_info: a curious and not yet implemented API");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_write (PortableServer_Servant     stream,
	   const Bonobo_Stream_iobuf *buffer,
	   CORBA_Environment         *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (
		bonobo_object (stream));
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
vfs_read (PortableServer_Servant stream,
	  CORBA_long             count,
	  Bonobo_Stream_iobuf  **buffer,
	  CORBA_Environment     *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (
		bonobo_object (stream));
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
vfs_seek (PortableServer_Servant stream,
	  CORBA_long             offset,
	  Bonobo_Stream_SeekType whence,
	  CORBA_Environment     *ev)
{
	BonoboStreamVfs     *sfs = BONOBO_STREAM_VFS (
		bonobo_object (stream));
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
vfs_truncate (PortableServer_Servant stream,
	      const CORBA_long       new_size, 
	      CORBA_Environment     *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (
		bonobo_object (stream));
	GnomeVFSResult result;

	result = gnome_vfs_truncate_handle (sfs->handle, new_size);
	if (result == GNOME_VFS_OK)
		return;

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NoPermission, NULL);
}

static void
vfs_commit (PortableServer_Servant stream,
	    CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_revert (PortableServer_Servant stream,
	    CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}
	
static void
vfs_destroy (BonoboObject *object)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (object);

	if (sfs->handle)
		if (gnome_vfs_close (sfs->handle) != GNOME_VFS_OK)
			g_warning ("VFS Close failed");

	sfs->handle = NULL;

	bonobo_stream_vfs_parent_class->destroy (object);
}

static void
bonobo_stream_vfs_class_init (BonoboStreamVfsClass *klass)
{
	POA_Bonobo_Stream__epv *epv = &klass->epv;
	
	bonobo_stream_vfs_parent_class = 
		g_type_class_peek_parent (klass);

	epv->getInfo  = vfs_get_info;
	epv->setInfo  = vfs_set_info;
	epv->write    = vfs_write;
	epv->read     = vfs_read;
	epv->seek     = vfs_seek;
	epv->truncate = vfs_truncate;
        epv->commit   = vfs_commit;
        epv->revert   = vfs_revert;

	((BonoboObjectClass *)klass)->destroy = vfs_destroy;
}

/**
 * bonobo_stream_vfs_get_type:
 *
 * Returns the GtkType for the BonoboStreamVfs class.
 */
GType
bonobo_stream_vfs_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (BonoboStreamVfsClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) bonobo_stream_vfs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (BonoboStreamVfs),
			0, /* n_preallocs */
			(GInstanceInitFunc) NULL
		};

		type = bonobo_type_unique (
			BONOBO_OBJECT_TYPE,
			POA_Bonobo_Stream__init, NULL,
			G_STRUCT_OFFSET (BonoboStreamVfsClass, epv),
			&info, "BonoboStreamVFS");
	}

	return type;
}

/**
 * bonobo_stream_vfs_open:
 * @path: The path to the file to be opened.
 * @mode: The mode with which the file should be opened.
 *
 * Creates a new BonoboStream object for the filename specified by
 * @path.  
 */
BonoboStreamVfs *
bonobo_stream_vfs_open (const char *path, gint flags, gint mode,
			CORBA_Environment *ev)
{
	GnomeVFSResult   result;
	GnomeVFSHandle  *handle;
	GnomeVFSOpenMode vfs_mode = GNOME_VFS_OPEN_NONE;
	BonoboStreamVfs *stream_vfs;

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

	stream_vfs = g_object_new (bonobo_stream_vfs_get_type (), NULL);
	if (!stream_vfs)
		return NULL;
	
	stream_vfs->handle = handle;

	return stream_vfs;
}
