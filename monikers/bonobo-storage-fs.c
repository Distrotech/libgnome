/*
 * gnome-storage-fs.c: Sample file-system based Storage implementation
 *
 * This is just a sample file-system based Storage implementation.
 * it is only used for debugging purposes
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 */
#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <bonobo/gnome-storage-fs.h>
#include <bonobo/gnome-stream-fs.h>

static GnomeStorageClass *gnome_storage_fs_parent_class;

static void
gnome_storage_fs_destroy (GtkObject *object)
{
	GnomeStorageFS *storage_fs = GNOME_STORAGE_FS (object);

	g_free (storage_fs->path);
}

static GnomeStream *
fs_create_stream (GnomeStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	GnomeStorageFS *storage_fs = GNOME_STORAGE_FS (storage);
	GnomeStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	stream = gnome_stream_fs_create (storage_fs, path);
	g_free (full);

	return stream;
}

static GnomeStream *
fs_open_stream (GnomeStorage *storage, const CORBA_char *path, GNOME_Storage_OpenMode mode, CORBA_Environment *ev)
{
	GnomeStorageFS *storage_fs = GNOME_STORAGE_FS (storage);
	GnomeStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	stream = gnome_stream_fs_open (GNOME_STORAGE_FS (storage), path, mode);
	g_free (full);

	return stream;
}

static GnomeStorage *
fs_create_storage (GnomeStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	GnomeStorageFS *storage_fs = GNOME_STORAGE_FS (storage);
	GnomeStorage *new_storage;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	new_storage = gnome_storage_fs_create (GNOME_STORAGE_FS (storage), path);
	g_free (full);

	return new_storage;
}

static void
fs_copy_to (GnomeStorage *storage, GNOME_Storage target, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_rename (GnomeStorage *storage, const CORBA_char *path, const CORBA_char *new_path, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_commit (GnomeStorage *storage, CORBA_Environment *ev)
{
}

static GNOME_Storage_directory_list *
fs_list_contents (GnomeStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	g_error ("Not yet implemented");

	return NULL;
}

static void
gnome_storage_fs_class_init (GnomeStorageFSClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	GnomeStorageClass *sclass = GNOME_STORAGE_CLASS (class);
	
	gnome_storage_fs_parent_class = gtk_type_class (gnome_storage_get_type ());

	sclass->create_stream  = fs_create_stream;
	sclass->open_stream    = fs_open_stream;
	sclass->create_storage = fs_create_storage;
	sclass->copy_to        = fs_copy_to;
	sclass->rename         = fs_rename;
	sclass->commit         = fs_commit;
	sclass->list_contents  = fs_list_contents;
	
	object_class->destroy = gnome_storage_fs_destroy;
}

static void
gnome_storage_init (GnomeUnknown *object)
{
}

GtkType
gnome_storage_fs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StorageFS:1.0",
			sizeof (GnomeStorageFS),
			sizeof (GnomeStorageFSClass),
			(GtkClassInitFunc) gnome_storage_fs_class_init,
			(GtkObjectInitFunc) gnome_storage_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_storage_get_type (), &info);
	}

	return type;
}

GnomeStorage *
gnome_storage_fs_construct (GnomeStorageFS *storage,
			    GNOME_Storage corba_storage,
			    const char *path, const char *open_mode)
{
	g_return_val_if_fail (storage != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_STORAGE (storage), NULL);
	g_return_val_if_fail (corba_storage != CORBA_OBJECT_NIL, NULL);

	gnome_storage_construct (
		GNOME_STORAGE (storage), corba_storage);

	return GNOME_STORAGE (storage);
}

static GNOME_Storage
create_gnome_storage_fs (GnomeUnknown *object)
{
	POA_GNOME_Storage *servant;
	CORBA_Object o;

	servant = (POA_GNOME_Storage *) g_new0 (GnomeUnknownServant, 1);
	servant->vepv = &gnome_storage_vepv;
	POA_GNOME_Storage__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
                return CORBA_OBJECT_NIL;
        }

	return (GNOME_Storage) gnome_unknown_activate_servant (object, servant);
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static GNOME_Storage
do_gnome_storage_fs_create (char *path)
{
	GnomeStorageFS *storage_fs;
	GNOME_Storage corba_storage;

	storage_fs = gtk_type_new (gnome_storage_fs_get_type ());
	storage_fs->path = g_strdup (path);
	
	corba_storage = create_gnome_storage_fs (
		GNOME_UNKNOWN (storage_fs));
	if (corba_storage == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (storage_fs));
		return NULL;
	}

	return (GNOME_Storage) gnome_storage_construct (GNOME_STORAGE (storage_fs), corba_storage);
}

/** 
 * gnome_storage_fs_open:
 * @path: path to existing directory that represents the storage
 * @mode: open mode for the storage
 *
 * Returns a GnomeStorage object that represents the storage at @path
 */
GnomeStorage *
gnome_storage_fs_open (const char *path, GNOME_Storage_OpenMode mode)
{
	struct stat s;
	int v;
	
	g_return_val_if_fail (path != NULL, NULL);

	v = stat (path, &s);

	if (mode == GNOME_Storage_READ){
		if (v == -1)
			return NULL;
		
		if (!S_ISDIR (s.st_mode))
			return NULL;

	} else if (mode == GNOME_Storage_WRITE){
		if (v == -1){
			if (mkdir (path, 0777) == -1)
				return NULL;
		} else {
			if (!S_ISDIR (s.st_mode))
				return NULL;
		}
	}

	return GNOME_STORAGE (do_gnome_storage_fs_create (g_strdup (path)));
}

/** 
 * gnome_storage_fs_create:
 * @storage_fs: parent storage
 * @path: path inside the storage to create
 *
 * Creates a new GnomeStorage object whose parent is @storage_fs and
 * whose path name is @path.  The storage created is an activated
 * CORBA server for this storage.
 *
 * if @storage_fs is NULL, it creates a toplevel storage.
 *
 * Returns the GnomeStorage GTK object.
 */
GnomeStorage *
gnome_storage_fs_create (GnomeStorageFS *storage_fs, const CORBA_char *path)
{
	char *full;

	g_return_val_if_fail (path != NULL, NULL);
	if (storage_fs != NULL){
		g_return_val_if_fail (GNOME_IS_STORAGE_FS (storage_fs), NULL);
	}
	
	if (!path || !storage_fs)
	        full = g_strdup (path);
	else
		full = g_concat_dir_and_file (storage_fs->path, path);
	
	if (mkdir (full, 0777) == -1){
		g_free (full);
		return NULL;
	}
	g_free (full);

	return GNOME_STORAGE (do_gnome_storage_fs_create (g_strdup (path)));
}


