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
#include <bonobo/gnome-stream-fs.h>
#include <bonobo/gnome-storage-fs.h>

static GnomeStorageClass *gnome_stream_fs_parent_class;

static CORBA_long
fs_write (GnomeStream *stream, long count,
	  const CORBA_char *buffer,
	  CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	while (write (sfs->fd, buffer, count) == -1 && errno == EINTR)
		;
	if (errno != EINTR){
		g_warning ("Should signal an exception here");
		return 0;
	}
	return count;
}

static CORBA_long
fs_read (GnomeStream *stream, long count,
	 const CORBA_char **buffer,
	 CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	g_warning ("WEEE!  How do I fill this?");
}

static void
fs_seek (GnomeStream *stream,
	 CORBA_long offset, CORBA_long whence,
	 CORBA_Environment *ev)
{
	GnomeStreamFS *sfs = GNOME_STREAM_FS (stream);

	if (lseek (sfs->fd, offset, whence) != -1){
		return;
	}
	g_warning ("Signal exception");
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
	     CORBA_long *read,
	     CORBA_long *written,
	     CORBA_Environment *ev)
{
	g_warning ("Implement me");
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

	gnome_stream_fs_parent_class = gtk_type_class (gnome_stream_get_type ());

	class->create_stream  = fs_write;
	class->open_stream    = fs_read;
	class->create_stream  = fs_seek;
	class->copy_to        = fs_truncate;
	class->rename         = fs_copy_to;
	class->commit         = fs_commit;
}

GtkType
gnome_stream_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StreamFS:1.0",
			sizeof (GnomeStream),
			sizeof (GnomeStreamClass),
			(GtkClassInitFunc) gnome_stream_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
	}

	return type;
}

GnomeStream *
gnome_stream_fs_construct (GnomeStreamFS *stream,
			   GNOME_Stream corba_stream,
			   const char *path, const char *open_mode)
{
	g_return_val_if_fail (stream != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_STREAM (stream), NULL);
	g_return_val_if_fail (corba_stream != CORBA_OBJECT_NIL, NULL);

	gnome_stream_construct (
		stream, corba_stream, path, open_mode);

	return stream;
}

GNOME_Stream *
create_gnome_stream_fs (GnomeObject *stream_fs)
{
	POA_GNOME_Stream *servant;
	CORBA_Object o;

	servant = g_new0 (POA_GNOME_Stream, 1);
	servant->vepv = gnome_stream_vepv;
	POA_GNOME_Stream__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
                return CORBA_OBJECT_NIL;
        }

	return gnome_object_activate_servant (object, servant);
}

GnomeStream *
gnome_stream_fs_open (GnomeStorage *parent, const char *path, const char *open_mode)
{
	GnomeStreamFS *stream_fs;
	GNOME_Stream corba_stream;
	struct stat s;
	int v;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_STORAGE_FS (parent), NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (open_mode != NULL, NULL);

	v = stat (path, &s);

	if (*open_mode == 'r'){
		if (v == -1)
			return NULL;
		
		if (!S_ISDIR (s.st_mode))
			return NULL;

	} else if (*open_mode == 'w'){
		if (v == -1){
			if (mkdir (path, 0777) == -1)
				return NULL;
		} else {
			if (!S_ISDIR (s.st_mode))
				return NULL;
			if (open_mode [1] != '+'){
				g_warning ("Shoudl remove directory %s\n", path);
			}
		}
	}
	
	stream_fs = gtk_type_new (gnome_stream_fs_get_type ());
	stream_fs->path = g_strdup (path);
	
	corba_stream = create_gnome_stream_fs (
		GNOME_OBJECT (stream));
	if (corba_stream == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (stream));
		return NULL;
	}

	return gnome_stream_construct (stream, corba_stream);
}
