/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-stream-fs.c: Sample file-system based Stream implementation
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
#include <bonobo/bonobo-stream-fs.h>
#include <errno.h>

static BonoboStreamClass *bonobo_stream_fs_parent_class;

static CORBA_long
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
		return 0;
	}
	return buffer->_length;
}

static CORBA_long
fs_read (BonoboStream *stream, CORBA_long count,
	 Bonobo_Stream_iobuf ** buffer,
	 CORBA_Environment *ev)
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
	} else
		CORBA_free (data);

	return v;
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

	g_warning ("Signal exception");
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
fs_commit   (BonoboStream *stream,
	     CORBA_Environment *ev)
{
	g_warning ("Implement fs commit");
}

static void
fs_close (BonoboStream *stream,
	  CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);

	if (close (sfs->fd))
		g_warning ("Close failed");
	sfs->fd = -1;
}

static CORBA_boolean
fs_eos (BonoboStream *stream,
	CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);
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
	
	return 0;
}
	
static CORBA_long
fs_length (BonoboStream *stream,
	   CORBA_Environment *ev)
{
	BonoboStreamFS *sfs = BONOBO_STREAM_FS (stream);
	struct stat st;

	if (fstat (sfs->fd, &st)) {
		g_warning ("Fstat failed");
		return 0;
	} else 
		return st.st_size;
}
	   

static void
bonobo_stream_fs_class_init (BonoboStreamFSClass *klass)
{
	BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	
	bonobo_stream_fs_parent_class = gtk_type_class (bonobo_stream_get_type ());

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

static Bonobo_Stream
create_bonobo_stream_fs (BonoboObject *object)
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
bonobo_stream_create (int fd)
{
	BonoboStreamFS *stream_fs;
	Bonobo_Stream corba_stream;

	stream_fs = gtk_type_new (bonobo_stream_fs_get_type ());
	if (stream_fs == NULL)
		return NULL;
	
	stream_fs->fd = fd;
	
	corba_stream = create_bonobo_stream_fs (
		BONOBO_OBJECT (stream_fs));

	if (corba_stream == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (stream_fs));
		return NULL;
	}

	return bonobo_stream_fs_construct (stream_fs, corba_stream);
}


/**
 * bonobo_stream_fs_open:
 * @path: The path to the file to be opened.
 * @mode: The mode with which the file should be opened.
 *
 * Creates a new BonoboStream object for the filename specified by
 * @path.  
 */
BonoboStream *
bonobo_stream_fs_open (const CORBA_char *path, Bonobo_Storage_OpenMode mode)
{
	struct stat s;
	int v, fd;
	char *full;

	g_return_val_if_fail (path != NULL, NULL);

	full = g_strdup (path);
	
	v = stat (full, &s);

	if (v == -1 || S_ISDIR (s.st_mode)) {
		g_free (full);
		return NULL;
	}
	
	if (mode == Bonobo_Storage_READ) {
		fd = open (full, O_RDONLY);
		if (fd == -1) {
			g_free (full);
			return NULL;
		}
	} else if (mode == Bonobo_Storage_WRITE) {
		fd = open (full, O_RDWR);
		if (fd == -1) {
			g_free (full);
			return NULL;
		}
	} else {
		g_free(full);
		return NULL;
	}
	
	g_free (full);
	
	return bonobo_stream_create (fd);
}

/**
 * bonobo_stream_fs_create:
 * @path: The path to the file to be opened.
 *
 * Creates a new BonoboStreamFS object which is bound to the file
 * specified by @path.
 *
 * When data is read out of or written into the returned BonoboStream
 * object, the read() and write() operations are mapped to the
 * corresponding operations on the specified file.
 *
 * Returns: the constructed BonoboStream object which is bound to the specified file.
 */
BonoboStream *
bonobo_stream_fs_create (const CORBA_char *path)
{
	int fd;

	g_return_val_if_fail (path != NULL, NULL);
	
	fd = open (path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	
	if (fd == -1)
		return NULL;

	return bonobo_stream_create (fd);
}
