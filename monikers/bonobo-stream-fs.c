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
#include <fcntl.h>
#include <sys/stat.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <bonobo/gnome-storage-fs.h>
#include <bonobo/gnome-stream-fs.h>
#include <errno.h>

static GnomeStreamClass *gnome_stream_fs_parent_class;

static CORBA_long
fs_write (GnomeStream *stream, const GNOME_Stream_iobuf *buffer,
	  CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	errno = EINTR;
	while (write (sfs->fd, buffer->_buffer, buffer->_length) == -1 
	       && errno == EINTR);

	if (errno != EINTR){
		g_warning ("Should signal an exception here");
		return 0;
	}
	return buffer->_length;
}

static CORBA_long
fs_read (GnomeStream *stream, CORBA_long count,
	 GNOME_Stream_iobuf ** buffer,
	 CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);
	CORBA_octet *data;
	int v;
	
	*buffer = GNOME_Stream_iobuf__alloc ();
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
fs_seek (GnomeStream *stream,
	 CORBA_long offset, CORBA_long whence,
	 CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	return lseek (sfs->fd, offset, whence);
}

static void
fs_truncate (GnomeStream *stream,
	     const CORBA_long new_size, 
	     CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	if (ftruncate (sfs->fd, new_size) == 0)
		return;

	g_warning ("Signal exception");
}

static void
fs_copy_to  (GnomeStream *stream,
	     const CORBA_char *dest,
	     const CORBA_long bytes,
	     CORBA_long *read_bytes,
	     CORBA_long *written_bytes,
	     CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);
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
fs_commit   (GnomeStream *stream,
	     CORBA_Environment *ev)
{
	g_warning ("Implement me");
}

static void
gnome_stream_fs_class_init (GnomeStreamFSClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GnomeStreamClass *sclass = GNOME_STREAM_CLASS (class);
	
	gnome_stream_fs_parent_class = gtk_type_class (gnome_stream_get_type ());

	sclass->write    = fs_write;
	sclass->read     = fs_read;
	sclass->seek     = fs_seek;
	sclass->truncate = fs_truncate;
	sclass->copy_to  = fs_copy_to;
	sclass->commit   = fs_commit;
}

GtkType
gnome_stream_fs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StreamFS:1.0",
			sizeof (GnomeStreamFS),
			sizeof (GnomeStreamFSClass),
			(GtkClassInitFunc) gnome_stream_fs_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_stream_get_type (), &info);
	}

	return type;
}

GnomeStream *
gnome_stream_fs_construct (GnomeStreamFS *stream,
			   GNOME_Stream corba_stream)
{
	g_return_val_if_fail (stream != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_STREAM (stream), NULL);
	g_return_val_if_fail (corba_stream != CORBA_OBJECT_NIL, NULL);

	gnome_unknown_construct (
		GNOME_UNKNOWN (stream), corba_stream);

	return GNOME_STREAM (stream);
}

static GNOME_Stream
create_gnome_stream_fs (GnomeUnknown *object)
{
	POA_GNOME_Stream *servant;
	CORBA_Object o;

	servant = (POA_GNOME_Stream *) g_new0 (GnomeUnknownServant, 1);
	servant->vepv = &gnome_stream_vepv;
	POA_GNOME_Stream__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
                return CORBA_OBJECT_NIL;
        }

	return (GNOME_Stream) gnome_unknown_activate_servant (object, servant);
}

static GnomeStream *
gnome_stream_create (GnomeStorageFS *parent, int fd)
{
	GnomeStreamFS *stream_fs;
	GNOME_Stream corba_stream;

	stream_fs = gtk_type_new (gnome_stream_fs_get_type ());
	if (stream_fs == NULL)
		return NULL;
	
	stream_fs->fd = fd;
	stream_fs->parent = parent;
	
	corba_stream = create_gnome_stream_fs (
		GNOME_UNKNOWN (stream_fs));

	if (corba_stream == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (stream_fs));
		return NULL;
	}

	return gnome_stream_fs_construct (stream_fs, corba_stream);
}

GnomeStream *
gnome_stream_fs_open (GnomeStorageFS *parent, const CORBA_char *path, GNOME_Storage_OpenMode mode)
{
	struct stat s;
	int v, fd;
	char *full;

	if (parent != NULL){
		g_return_val_if_fail (GNOME_IS_STORAGE_FS (parent), NULL);
	}
	g_return_val_if_fail (path != NULL, NULL);

	if (parent)
		full = g_concat_dir_and_file (parent->path, path);
	else
		full = g_strdup (path);
	
	v = stat (full, &s);

  if (v == -1){
    g_free (full);
    return NULL;
  }

	if (mode == GNOME_Storage_READ){
		fd = open (full, O_RDONLY);
		if (fd == -1){
			g_free (full);
			return NULL;
		}
	} else if (mode == GNOME_Storage_WRITE){
		fd = open (full, O_RDWR);
		if (fd == -1){
			g_free (full);
			return NULL;
		}
	} else {
    g_free(full);
		return NULL;
	}

	g_free (full);

	return gnome_stream_create (parent, fd);
}

GnomeStream *
gnome_stream_fs_create (GnomeStorageFS *fs, const CORBA_char *path)
{
	char *full;
	int fd;

	if (fs != NULL){
		g_return_val_if_fail (GNOME_IS_STORAGE_FS (fs), NULL);
	}
	g_return_val_if_fail (path != NULL, NULL);
	
	full = g_concat_dir_and_file (fs->path, path);
	fd = open (full, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	g_free (full);
	
	if (fd == -1)
		return NULL;

	return gnome_stream_create (fs, fd);
}



