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
#include <libgnome/gnome-mime.h>
#include <storage-modules/bonobo-stream-fs.h>
#include <errno.h>

struct _BonoboStreamFSPrivate {
	gchar *mime_type;
};

static BonoboStreamClass *bonobo_stream_fs_parent_class;

static gint
bonobo_mode_to_fs (Bonobo_Storage_OpenMode mode)
{
	gint fs_mode = 0;

	if (mode & Bonobo_Storage_READ)
		fs_mode |= O_RDONLY;
	if (mode & Bonobo_Storage_WRITE)
		fs_mode |= O_RDWR;
	if (mode & Bonobo_Storage_CREATE)
		fs_mode |= O_CREAT | O_RDWR;
	if (mode & Bonobo_Storage_FAILIFEXIST)
		fs_mode |= O_EXCL;

	return fs_mode;
}

static Bonobo_StorageInfo*
fs_get_info (BonoboStream                   *stream,
	     const Bonobo_StorageInfoFields  mask,
	     CORBA_Environment              *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);
	Bonobo_StorageInfo *si;
	struct stat st;

	if (mask & ~(Bonobo_FIELD_CONTENT_TYPE | Bonobo_FIELD_SIZE |
		     Bonobo_FIELD_TYPE)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotSupported, NULL);
		return CORBA_OBJECT_NIL;
	}

	if (fstat (stream_fs->fd, &st) == -1)
		goto get_info_except;
		
	si = Bonobo_StorageInfo__alloc ();

	si->size = st.st_size;
	si->type = Bonobo_STORAGE_TYPE_REGULAR;
	si->name = CORBA_string_dup ("");
	si->content_type = CORBA_string_dup (stream_fs->priv->mime_type);

	return si;

 get_info_except:

	if (errno == EACCES) 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_NoPermission, NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_IOError, NULL);
	

	return CORBA_OBJECT_NIL;
}

static void
fs_set_info (BonoboStream                   *stream,
	     const Bonobo_StorageInfo       *info,
	     const Bonobo_StorageInfoFields  mask,
	     CORBA_Environment              *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_write (BonoboStream *stream, const Bonobo_Stream_iobuf *buffer,
	  CORBA_Environment *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);

	errno = EINTR;
	while ((write (stream_fs->fd, buffer->_buffer, buffer->_length) == -1)
	       && (errno == EINTR));

	if (errno == EINTR) return;

	if ((errno == EBADF) || (errno == EINVAL))
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_NoPermission, NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_IOError, NULL);
}

static void
fs_read (BonoboStream         *stream,
	 CORBA_long            count,
	 Bonobo_Stream_iobuf **buffer,
	 CORBA_Environment    *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);
	CORBA_octet *data;
	int bytes_read;
	
	if (count < 0) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_IOError, NULL);
		return;
	}

	*buffer = Bonobo_Stream_iobuf__alloc ();
	CORBA_sequence_set_release (*buffer, TRUE);
	data = CORBA_sequence_CORBA_octet_allocbuf (count);
	(*buffer)->_buffer = data;
	(*buffer)->_length = 0;

	do {
		bytes_read = read (stream_fs->fd, data, count);
	} while ((bytes_read == -1) && (errno == EINTR));


	if (bytes_read == -1) {
		CORBA_free (*buffer);
		*buffer = NULL;

		if (errno == EACCES) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Stream_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_IOError, NULL);
	} else
		(*buffer)->_length = bytes_read;
}

static CORBA_long
fs_seek (BonoboStream           *stream,
	 CORBA_long              offset, 
	 Bonobo_Stream_SeekType  whence,
	 CORBA_Environment      *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);
	int fs_whence;
	CORBA_long pos;

	if (whence == Bonobo_Stream_SEEK_CUR)
		fs_whence = SEEK_CUR;
	else if (whence == Bonobo_Stream_SEEK_END)
		fs_whence = SEEK_END;
	else
		fs_whence = SEEK_SET;

	if ((pos = lseek (stream_fs->fd, offset, fs_whence)) == -1) {

		if (errno == ESPIPE) 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Stream_NotSupported, 
					     NULL);
		else
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Stream_IOError, NULL);
		return 0;
	}

	return pos;
}

static void
fs_truncate (BonoboStream      *stream,
	     const CORBA_long   new_size, 
	     CORBA_Environment *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);

	if (ftruncate (stream_fs->fd, new_size) == 0)
		return;

	if (errno == EACCES)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_NoPermission, NULL);
	else 
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Stream_IOError, NULL);
}

static void
fs_copy_to  (BonoboStream      *stream,
	     const CORBA_char  *dest,
	     const CORBA_long   bytes,
	     CORBA_long        *read_bytes,
	     CORBA_long        *written_bytes,
	     CORBA_Environment *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (stream);
	gchar data[4096];
	CORBA_unsigned_long more = bytes;
	int v, w;
	int fd_out;

	*read_bytes = 0;
	*written_bytes = 0;

	if ((fd_out = creat(dest, 0644)) == -1)
		goto copy_to_except;
     
	do {
		if (bytes == -1) 
			more = sizeof (data);

		do {
			v = read (stream_fs->fd, data, 
				  MIN (sizeof (data), more));
		} while ((v == -1) && (errno == EINTR));

		if (v == -1) 
			goto copy_to_except;

		if (v <= 0) 
			break;

		*read_bytes += v;
		more -= v;

		do {
			w = write (fd_out, data, v);
		} while (w == -1 && errno == EINTR);
		
		if (w == -1)
			goto copy_to_except;

		*written_bytes += w;
			
	} while ((more > 0 || bytes == -1) && v > 0);

	close (fd_out);

	return;

 copy_to_except:

	if (fd_out != -1)
		close (fd_out);

	if (errno == EACCES)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_NoPermission, NULL);
	else
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Stream_IOError, NULL);
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
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (object);
	
	if (close (stream_fs->fd)) 
		g_warning ("Close failed");
	stream_fs->fd = -1;

	if (stream_fs->path)
		g_free (stream_fs->path);
	stream_fs->path = NULL;

	if (stream_fs->priv->mime_type)
		g_free (stream_fs->priv->mime_type);
	stream_fs->priv->mime_type = NULL;
}

static void
fs_finalize (GtkObject *object)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (object);
	
	if (stream_fs->priv)
		g_free (stream_fs->priv);
	stream_fs->priv = NULL;
}

static void
bonobo_stream_fs_class_init (BonoboStreamFSClass *klass)
{
	GtkObjectClass    *oclass = (GtkObjectClass *) klass;
	BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	
	bonobo_stream_fs_parent_class = 
		gtk_type_class (bonobo_stream_get_type ());

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
	oclass->finalize = fs_finalize;
}

static void
bonobo_stream_fs_init (BonoboStreamFS *stream_fs)
{
	stream_fs->priv = g_new0 (BonoboStreamFSPrivate,1);
	stream_fs->priv->mime_type = NULL;
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
			(GtkObjectInitFunc) bonobo_stream_fs_init,
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
bonobo_stream_create (int fd, const char *path)
{
	BonoboStreamFS *stream_fs;
	Bonobo_Stream corba_stream;

	stream_fs = gtk_type_new (bonobo_stream_fs_get_type ());
	if (stream_fs == NULL)
		return NULL;
	
	stream_fs->fd = fd;
	stream_fs->priv->mime_type = g_strdup (gnome_mime_type_of_file (path));
	
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
	BonoboStream *stream;
	struct stat st;
	int v, fd;
	gint fs_flags;

	if (!path || !ev) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);
		return NULL;
	}

	if (((v = stat (path, &st)) == -1) && 
	    !(flags & Bonobo_Storage_CREATE)) {
		
		if ((errno == ENOENT) || (errno == ENOTDIR))
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if (errno == EACCES)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else 
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
		return NULL;
	}

	if ((v != -1) && S_ISDIR(st.st_mode)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotStream, 
				     NULL);
		return NULL;
	}


	fs_flags = bonobo_mode_to_fs (flags);
 
	if ((fd = open (path, fs_flags, mode)) == -1) {

		if ((errno == ENOENT) || (errno == ENOTDIR))
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NotFound, 
					     NULL);
		else if (errno == EACCES)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NoPermission, 
					     NULL);
		else if (errno == EEXIST)
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_NameExists, 
					     NULL);
		else
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
					     ex_Bonobo_Storage_IOError, NULL);
		return NULL;
	}

	if (!(stream = bonobo_stream_create (fd, path)))
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_IOError, NULL);

	return stream;
}
