/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-stream-vfs.c: Gnome VFS based stream implementation
 *
 * Author:
 *   Michael Meeks <michael@helixcode.com>
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

static CORBA_long
fs_write (BonoboStream *stream, const Bonobo_Stream_iobuf *buffer,
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
		g_warning ("Should signal an exception here");

	return bytes_written;
}

static CORBA_long
fs_read (BonoboStream *stream, CORBA_long count,
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

	if (result != GNOME_VFS_OK)
		g_warning ("Should signal an exception here");

	(*buffer)->_buffer = data;
	(*buffer)->_length = bytes_read;

	return bytes_read;
}

static CORBA_long
fs_seek (BonoboStream *stream,
	 CORBA_long offset, Bonobo_Stream_SeekType whence,
	 CORBA_Environment *ev)
{
	BonoboStreamVfs     *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSSeekPosition pos = GNOME_VFS_SEEK_START;
	GnomeVFSResult       result;
	GnomeVFSFileOffset   where;
	
	switch (whence) {
	case SEEK_CUR:
		pos = GNOME_VFS_SEEK_CURRENT;
		break;
	case SEEK_END:
		pos = GNOME_VFS_SEEK_END;
		break;
	case SEEK_SET:
		pos = GNOME_VFS_SEEK_START;
		break;
	default:
		g_warning ("Seek whence %d unknown; fall back to SEEK_SET",
			   whence);
		break;
	}
	
	result = gnome_vfs_seek (sfs->handle, pos, offset);

	if (result != GNOME_VFS_OK) {
		g_warning ("Seek failure");
		return 0;
	}
       
	result = gnome_vfs_tell (sfs->handle, &where);
	
	if (result != GNOME_VFS_OK) {
		g_warning ("Tell failure");
		return 0;
	}

	return where;
}

static void
fs_truncate (BonoboStream *stream,
	     const CORBA_long new_size, 
	     CORBA_Environment *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	GnomeVFSResult result;

	result = gnome_vfs_truncate_handle (sfs->handle, new_size);
	if (result == GNOME_VFS_OK)
		return;

	g_warning ("Signal truncate exception");
}

static void
fs_copy_to  (BonoboStream *stream,
	     const CORBA_char *dest,
	     const CORBA_long bytes,
	     CORBA_long *read_bytes,
	     CORBA_long *written_bytes,
	     CORBA_Environment *ev)
{
	g_warning ("Implement copyto; can we use xfer ?");
/*	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	CORBA_octet *data;
	CORBA_unsigned_long more = bytes;
	int v, w;
	int fd_out;

#define READ_CHUNK_SIZE 65536

	data = CORBA_sequence_CORBA_octet_allocbuf (READ_CHUNK_SIZE);

	*read_bytes = 0;
	*written_bytes = 0;

	fd_out = creat(dest, 0666);
	if(fd_out == -1)
		return;

	do {
		do {
			v = read (sfs->fd, data, MIN(READ_CHUNK_SIZE, more));
		} while (v == -1 && errno == EINTR);

		if (v != -1) {
			*read_bytes += v;
			more -= v;
			do {
				w = write (fd_out, data, v);
			} while (w == -1 && errno == EINTR);
			if (w != -1)
				*written_bytes += w;
			else if(errno != EINTR) {
*//* should probably do something to signal an error here *//*
				break;
			}
		}
		else if(errno != EINTR) {
							    *//* should probably do something to signal an error here *//*
			break;
		}
	} while(more > 0 && v > 0);

	close(fd_out);*/
}

static void
fs_commit   (BonoboStream *stream,
	     CORBA_Environment *ev)
{
	g_warning ("Implement fs commit");
}

static void
fs_close (BonoboStream *stream,
	  CORBA_Environment *ev)
{
	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);

	if (sfs->handle &&
	    gnome_vfs_close (sfs->handle) != GNOME_VFS_OK)
		g_warning ("Close failed");
	sfs->handle = NULL;
}

static CORBA_boolean
fs_eos (BonoboStream *stream,
	CORBA_Environment *ev)
{
/*	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	off_t offset;
	struct stat st;

	if (fstat (sfs->fd, &st)) {
		g_warning ("fstat failed");
		return 1;
	}

	offset = lseek (sfs->fd, 0, SEEK_CUR);

	if (offset == -1) {
		g_warning ("seek failed");
		return 1;
	}

	if (offset == st.st_size)
		return 1;
*/
	g_warning ("Implement me");
	return 0;
}
	
static CORBA_long
fs_length (BonoboStream *stream,
	   CORBA_Environment *ev)
{
/*	BonoboStreamVfs *sfs = BONOBO_STREAM_VFS (stream);
	struct stat st;

	if (fstat (sfs->fd, &st)) {
		g_warning ("Fstat failed");
		return 0;
	} else 
	return st.st_size;*/
	g_warning ("Implement me");
	return 0;
}

static void
bonobo_stream_vfs_class_init (BonoboStreamVfsClass *klass)
{
	BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	
	bonobo_stream_vfs_parent_class = gtk_type_class (bonobo_stream_get_type ());

	sclass->write    = fs_write;
	sclass->read     = fs_read;
	sclass->seek     = fs_seek;
	sclass->truncate = fs_truncate;
	sclass->copy_to  = fs_copy_to;
	sclass->commit   = fs_commit;
	sclass->close    = fs_close;
	sclass->eos      = fs_eos;
	sclass->length   = fs_length;
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

/**
 * bonobo_stream_vfs_construct:
 * @stream: The BonoboStreamVfs object to initialize.
 * @corba_stream: The CORBA server which implements the BonoboStreamVfs service.
 *
 * This function initializes an object of type BonoboStreamVfs using the
 * provided CORBA server @corba_stream.
 *
 * Returns the constructed BonoboStreamVfs @stream.
 */
BonoboStream *
bonobo_stream_vfs_construct (BonoboStreamVfs *stream,
			     Bonobo_Stream corba_stream)
{
	g_return_val_if_fail (stream != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_STREAM (stream), NULL);
	g_return_val_if_fail (corba_stream != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (
		BONOBO_OBJECT (stream), corba_stream);

	return BONOBO_STREAM (stream);
}

static Bonobo_Stream
create_bonobo_stream_vfs (BonoboObject *object)
{
	POA_Bonobo_Stream *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_Stream *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_stream_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_Stream__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
		CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

	CORBA_exception_free (&ev);
	return (Bonobo_Stream) bonobo_object_activate_servant (object, servant);
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
	
	corba_stream = create_bonobo_stream_vfs (
		BONOBO_OBJECT (stream_vfs));

	if (corba_stream == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (stream_vfs));
		return NULL;
	}

	return bonobo_stream_vfs_construct (stream_vfs, corba_stream);
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
bonobo_stream_vfs_open (const CORBA_char *path, Bonobo_Storage_OpenMode mode)
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

/**
 * bonobo_stream_vfs_create:
 * @path: The path to the file to be opened.
 *
 * Creates a new BonoboStreamVfs object which is bound to the file
 * specified by @path.
 *
 * When data is read out of or written into the returned BonoboStream
 * object, the read() and write() operations are mapped to the
 * corresponding operations on the specified file.
 *
 * Returns: the constructed BonoboStream object which is bound to the specified file.
 */
BonoboStream *
bonobo_stream_vfs_create (const CORBA_char *path)
{
	GnomeVFSResult  result;
	GnomeVFSHandle *handle;

	g_return_val_if_fail (path != NULL, NULL);
	
	result = gnome_vfs_create (&handle, path, GNOME_VFS_OPEN_READ |
				   GNOME_VFS_OPEN_WRITE | GNOME_VFS_OPEN_RANDOM,
				   FALSE, S_IRUSR | S_IWUSR);

	if (result != GNOME_VFS_OK)
		return NULL;

	return bonobo_stream_create (handle);
}



