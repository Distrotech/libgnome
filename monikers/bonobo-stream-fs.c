/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-stream-fs.c: Sample file-system based Stream implementation
 *
 * This is just a sample file-system based Stream implementation.
 * it is only used for debugging purposes
 *
 * Authors:
 *   Miguel de Icaza (miguel@gnu.org)
 *   Michael Meeks   (michael@ximian.com)
 *
 * Copyright 2001, Ximian, Inc
 */
#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-mime.h>
#include "bonobo-stream-fs.h"

struct _BonoboStreamFSPrivate {
	gchar *mime_type;
};

static BonoboObjectClass *bonobo_stream_fs_parent_class;

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
fs_get_info (PortableServer_Servant         stream,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment             *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (
		bonobo_object (stream));
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
fs_set_info (PortableServer_Servant         stream,
	     const Bonobo_StorageInfo      *info,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment             *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_write (PortableServer_Servant     stream,
	  const Bonobo_Stream_iobuf *buffer,
	  CORBA_Environment         *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (
		bonobo_object (stream));

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
fs_read (PortableServer_Servant stream,
	 CORBA_long             count,
	 Bonobo_Stream_iobuf  **buffer,
	 CORBA_Environment     *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (
		bonobo_object (stream));
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
fs_seek (PortableServer_Servant stream,
	 CORBA_long             offset, 
	 Bonobo_Stream_SeekType whence,
	 CORBA_Environment     *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (
		bonobo_object (stream));
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
fs_truncate (PortableServer_Servant stream,
	     const CORBA_long       new_size, 
	     CORBA_Environment     *ev)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (
		bonobo_object (stream));

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
fs_commit (PortableServer_Servant stream,
	   CORBA_Environment     *ev)
{
        CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                             ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_revert (PortableServer_Servant stream,
	   CORBA_Environment     *ev)
{
        CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                             ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_destroy (BonoboObject *object)
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

	bonobo_stream_fs_parent_class->destroy (object);
}

static void
fs_finalize (GObject *object)
{
	BonoboStreamFS *stream_fs = BONOBO_STREAM_FS (object);
	
	if (stream_fs->priv)
		g_free (stream_fs->priv);
	stream_fs->priv = NULL;

	bonobo_stream_fs_parent_class->parent_class.finalize (object);
}

static void
bonobo_stream_fs_class_init (BonoboStreamFSClass *klass)
{
	GObjectClass    *oclass = (GObjectClass *) klass;
	POA_Bonobo_Stream__epv *epv = &klass->epv;
	
	bonobo_stream_fs_parent_class = 
		g_type_class_peek_parent (klass);

	epv->getInfo  = fs_get_info;
	epv->setInfo  = fs_set_info;
	epv->write    = fs_write;
	epv->read     = fs_read;
	epv->seek     = fs_seek;
	epv->truncate = fs_truncate;
        epv->commit   = fs_commit;
        epv->revert   = fs_revert;

	oclass->finalize = fs_finalize;
	((BonoboObjectClass *)oclass)->destroy = fs_destroy;
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
 * Returns the GType for the BonoboStreamFS class.
 */
GType
bonobo_stream_fs_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (BonoboStreamFSClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) bonobo_stream_fs_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (BonoboStreamFS),
			0, /* n_preallocs */
			(GInstanceInitFunc) bonobo_stream_fs_init
		};

		type = bonobo_type_unique (
			BONOBO_OBJECT_TYPE,
			POA_Bonobo_Stream__init, NULL,
			G_STRUCT_OFFSET (BonoboStreamFSClass, epv),
			&info, "BonoboStreamFS");
	}

	return type;
}

static BonoboStreamFS *
bonobo_stream_create (int fd, const char *path)
{
	BonoboStreamFS *stream_fs;

	if (!(stream_fs = g_object_new (bonobo_stream_fs_get_type (), NULL)))
		return NULL;
	
	stream_fs->fd = fd;
	stream_fs->priv->mime_type = g_strdup (
		gnome_mime_type_of_file (path));

	return stream_fs;
}


/**
 * bonobo_stream_fs_open:
 * @path: The path to the file to be opened.
 * @flags: The flags with which the file should be opened.
 *
 * Creates a new BonoboStream object for the filename specified by
 * @path.  
 */
BonoboStreamFS *
bonobo_stream_fs_open (const char *path, gint flags, gint mode,
		       CORBA_Environment *ev)
{
	BonoboStreamFS *stream;
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
