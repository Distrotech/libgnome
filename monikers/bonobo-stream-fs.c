/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-stream-fs.c: Sample file-system based Stream implementation
 *
 * This is just a sample file-system based Stream implementation.
 * it is only used for debugging purposes
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 */
#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <storage-modules/bonobo-stream-fs.h>
#include <errno.h>

static BonoboStreamClass *bonobo_stream_fs_parent_class;

static Bonobo_StorageInfo*
fs_get_info (BonoboStream *stream,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	g_warning ("Not implemented");

	return CORBA_OBJECT_NIL;
}

static void
fs_set_info (BonoboStream *stream,
	     const Bonobo_StorageInfo *info,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	g_warning ("Not implemented");
}

static void
fs_write (BonoboStream *stream, const Bonobo_Stream_iobuf *buffer,
	  CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);

	errno = EINTR;
	while (write (sfs->fd, buffer->_buffer, buffer->_length) == -1 
	       && errno == EINTR);

	if (errno != EINTR){
		g_warning ("Should signal an exception here");
		CORBA_exception_set(ev, CORBA_USER_EXCEPTION,
				    ex_Bonobo_Storage_NameExists, NULL);
		return;
	}
	return;
}

static void
fs_read (BonoboStream         *stream,
	 CORBA_long            count,
	 Bonobo_Stream_iobuf **buffer,
	 CORBA_Environment    *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);
	CORBA_octet *data;
	int v;
	
	*buffer = Bonobo_Stream_iobuf__alloc ();
	CORBA_sequence_set_release (*buffer, TRUE);
	data = CORBA_sequence_CORBA_octet_allocbuf (count);

	do {
		v = read (sfs->fd, data, count);
	} while (v == -1 && errno == EINTR);

	if (v != -1){
		(*buffer)->_buffer = data;
		(*buffer)->_length = v;
	} else {
		CORBA_free (data);
		CORBA_free (*buffer);
		*buffer = NULL;
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
	}
}

static CORBA_long
fs_seek (BonoboStream *stream,
	 CORBA_long offset, Bonobo_Stream_SeekType whence,
	 CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);
	int fw;

	if (whence == Bonobo_Stream_SEEK_CUR)
		fw = SEEK_CUR;
	else if (whence == Bonobo_Stream_SEEK_END)
		fw = SEEK_END;
	else
		fw = SEEK_SET;

	return lseek (sfs->fd, offset, fw);
}

static void
fs_truncate (BonoboStream *stream,
	     const CORBA_long new_size, 
	     CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);

	if (ftruncate (sfs->fd, new_size) == 0)
		return;

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NoPermission, NULL);
}

static void
fs_copy_to  (BonoboStream *stream,
	     const CORBA_char *dest,
	     const CORBA_long bytes,
	     CORBA_long *read_bytes,
	     CORBA_long *written_bytes,
	     CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);
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
				/* should probably do something to signal an error here */
				break;
			}
		}
		else if(errno != EINTR) {
			/* should probably do something to signal an error here */
			break;
		}
	} while(more > 0 && v > 0);

	close(fd_out);
}

static void
fs_commit (BonoboStream *stream,
	   CORBA_Environment *ev)
{
        CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                             ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_revert (BonoboStream *stream,
	   CORBA_Environment *ev)
{
        CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                             ex_Bonobo_Stream_NotSupported, NULL);
}


static void
fs_destroy (GtkObject *object)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (object);
	
	if (close (sfs->fd)) g_warning ("Close failed");
	
	sfs->fd = -1;
}

static void
bonobo_stream_fs_class_init (BonoboStreamFSClass *klass)
{
	GtkObjectClass    *oclass = (GtkObjectClass *) klass;
	BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	
	bonobo_stream_fs_parent_class = gtk_type_class (bonobo_stream_get_type ());

	sclass->get_info = fs_get_info;
	sclass->set_info = fs_set_info;
	sclass->write    = fs_write;
	sclass->read     = fs_read;
	sclass->seek     = fs_seek;
	sclass->truncate = fs_truncate;
	sclass->copy_to  = fs_copy_to;
        sclass->commit   = fs_commit;
        sclass->revert   = fs_revert;


	oclass->destroy = fs_destroy;
}

/**
 * bonobo_stream_fs_get_type:
 *
 * Returns the GtkType for the BonoboStreamFS class.
 */
GtkType
bonobo_stream_fs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboStreamFS",
			sizeof (BonoboStreamFS),
			sizeof (BonoboStreamFSClass),
			(GtkClassInitFunc) bonobo_stream_fs_class_init,
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
 * bonobo_stream_fs_construct:
 * @stream: The BonoboStreamFS object to initialize.
 * @corba_stream: The CORBA server which implements the BonoboStreamFS service.
 *
 * This function initializes an object of type BonoboStreamFS using the
 * provided CORBA server @corba_stream.
 *
 * Returns the constructed BonoboStreamFS @stream.
 */
BonoboStream *
bonobo_stream_fs_construct (BonoboStreamFS *stream,
			   Bonobo_Stream corba_stream)
{
	g_return_val_if_fail (stream != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_STREAM (stream), NULL);
	g_return_val_if_fail (corba_stream != CORBA_OBJECT_NIL, NULL);

	bonobo_object_construct (
		BONOBO_OBJECT (stream), corba_stream);

	return BONOBO_STREAM (stream);
}

static BonoboStream *
bonobo_stream_create (int fd)
{
	BonoboStreamFS *stream_fs;
	Bonobo_Stream corba_stream;

	stream_fs = gtk_type_new (bonobo_stream_fs_get_type ());
	if (stream_fs == NULL)
		return NULL;
	
	stream_fs->fd = fd;
	
	corba_stream = bonobo_stream_corba_object_create (
		BONOBO_OBJECT (stream_fs));

	if (corba_stream == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (stream_fs));
		return NULL;
	}

	return bonobo_stream_fs_construct (stream_fs, corba_stream);
}


/**
 * bonobo_stream_fs_open:
 * @path: The path to the file to be opened.
 * @flags: The flags with which the file should be opened.
 *
 * Creates a new BonoboStream object for the filename specified by
 * @path.  
 */
BonoboStream *
bonobo_stream_fs_open (const char *path, gint flags, gint mode,
		       CORBA_Environment *ev)
{
	struct stat s;
	int v, fd;
	gint nflags = 0;

	g_return_val_if_fail (path != NULL, NULL);

	v = stat (path, &s);

	if (v == -1 || S_ISDIR (s.st_mode)) return NULL;
       
	
	if ((flags & Bonobo_Storage_WRITE) ||
	    (flags & Bonobo_Storage_CREATE)) nflags = O_RDWR;
	else nflags = O_RDONLY;
 
	if ((fd = open (path, nflags, mode)) == -1) return NULL;
	    	
	return bonobo_stream_create (fd);
}
