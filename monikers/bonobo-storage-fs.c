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
#include <gnome-storage-fs.h>

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
	stream = gnome_stream_create (path);
	g_free (full);

	return stream;
}

static GnomeStream *
fs_open_stream (GnomeStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	GnomeStorageFS *storage_fs = GNOME_STORAGE_FS (storage);
	GnomeStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	stream = gnome_stream_open (path);
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
	new_storage = gnome_storage_fs_open (path, "w");
	g_free (full);

	return new_storage;
}

static void
fs_copy_to (GnomeStorage *storage, GNOME_Storage target, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_rename (GnomeStorage *storage, CORBA_char *path, CORBA_char *new_path, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_commit (GnomeStorage *storage, CORBA_Environment *ev)
{
}

static void
gnome_storage_fs_class_init (GnomeStorageFSClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	gnome_storage_fs_parent_class = gtk_type_class (gnome_storage_get_type ());

	class->create_stream  = fs_create_stream;
	class->open_stream    = fs_open_stream;
	class->create_storage = fs_create_storage;
	class->copy_to        = fs_copy_to;
	class->rename         = fs_rename;
	class->commit         = fs_commit;
	class->list_contents  = fs_list_contents;
	
	object_class->destroy = gnome_storage_fs_destroy;
}

static void
gnome_storage_init (GnomeObject *object)
{
}

GtkType
gnome_storage_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StorageFS:1.0",
			sizeof (GnomeStorage),
			sizeof (GnomeStorageClass),
			(GtkClassInitFunc) gnome_storage_class_init,
			(GtkObjectInitFunc) gnome_storage_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gnome_object_get_type (), &info);
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
		storage, corba_storage, path, open_mode);

	return storage;
}

GNOME_Storage *
create_gnome_storage (GnomeObject *storage_fs)
{
	POA_GNOME_Storage *servant;
	CORBA_Object o;

	servant = g_new0 (POA_GNOME_Storage, 1);
	servant->vepv = gnome_storage_vepv;
	POA_GNOME_Storage__init ((PortableServer_Servant) servant, &object->ev);
	if (object->ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
                return CORBA_OBJECT_NIL;
        }

	return gnome_object_activate_servant (object, servant);
}

GnomeStorage *
gnome_storage_fs_open (const char *path, const char *open_mode)
{
	GnomeStorageFS *storage_fs;
	GNOME_Storage corba_storage;
	struct stat s;
	int v;
	
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
	
	storage_fs = gtk_type_new (gnome_storage_fs_get_type ());
	storage_fs->path = g_strdup (path);
	
	corba_storage = create_gnome_storage_fs (
		GNOME_OBJECT (storage));
	if (corba_storage == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (storage));
		return NULL;
	}

	return gnome_storage_construct (storage, corba_storage);
}
