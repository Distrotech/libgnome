/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-storage-fs.c: Sample file-system based Storage implementation
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
#include <storage-modules/bonobo-storage-fs.h>
#include <storage-modules/bonobo-stream-fs.h>
#include <bonobo/bonobo-storage-plugin.h>

static BonoboStorageClass *bonobo_storage_fs_parent_class;

static void
bonobo_storage_fs_destroy (GtkObject *object)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (object);

	g_free (storage_fs->path);
}

static Bonobo_StorageInfo*
fs_get_info (BonoboStorage *storage,
	     const CORBA_char *path,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	g_warning ("Not implemented");

	return CORBA_OBJECT_NIL;
}

static void
fs_set_info (BonoboStorage *storage,
	     const CORBA_char *path,
	     const Bonobo_StorageInfo *info,
	     const Bonobo_StorageInfoFields mask,
	     CORBA_Environment *ev)
{
	g_warning ("Not implemented");
}

static BonoboStream *
fs_open_stream (BonoboStorage *storage, const CORBA_char *path, 
		Bonobo_Storage_OpenMode mode, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	BonoboStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	stream = bonobo_stream_fs_open (full, mode, 0644, ev);
	g_free (full);

	return stream;
}

static BonoboStorage *
fs_open_storage (BonoboStorage *storage, const CORBA_char *path, 
		 Bonobo_Storage_OpenMode mode, CORBA_Environment *ev)
{
	BonoboStorageFS *storage_fs = BONOBO_STORAGE_FS (storage);
	BonoboStorage *new_storage;
	char *full;

	full = g_concat_dir_and_file (storage_fs->path, path);
	new_storage = bonobo_storage_fs_open (full, mode, 0644, ev);
	g_free (full);

	return new_storage;
}

static void
fs_copy_to (BonoboStorage *storage, Bonobo_Storage target, 
	    CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_rename (BonoboStorage *storage, const CORBA_char *path, 
	   const CORBA_char *new_path, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
}

static void
fs_commit (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
fs_revert (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static Bonobo_Storage_DirectoryList *
fs_list_contents (BonoboStorage *storage, const CORBA_char *path, 
		  Bonobo_StorageInfoFields mask, CORBA_Environment *ev)
{
	g_error ("Not yet implemented");

	return NULL;
}

static void
bonobo_storage_fs_class_init (BonoboStorageFSClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	BonoboStorageClass *sclass = BONOBO_STORAGE_CLASS (class);
	
	bonobo_storage_fs_parent_class = 
		gtk_type_class (bonobo_storage_get_type ());

	sclass->get_info       = fs_get_info;
	sclass->set_info       = fs_set_info;
	sclass->open_stream    = fs_open_stream;
	sclass->open_storage   = fs_open_storage;
	sclass->copy_to        = fs_copy_to;
	sclass->rename         = fs_rename;
	sclass->commit         = fs_commit;
	sclass->revert         = fs_revert;
	sclass->list_contents  = fs_list_contents;
	
	object_class->destroy = bonobo_storage_fs_destroy;
}

static void
bonobo_storage_init (BonoboObject *object)
{
}

GtkType
bonobo_storage_fs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StorageFS:1.0",
			sizeof (BonoboStorageFS),
			sizeof (BonoboStorageFSClass),
			(GtkClassInitFunc) bonobo_storage_fs_class_init,
			(GtkObjectInitFunc) bonobo_storage_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_storage_get_type (), &info);
	}

	return type;
}

BonoboStorage *
bonobo_storage_fs_construct (BonoboStorageFS *storage,
			    Bonobo_Storage corba_storage,
			    const char *path, const char *open_mode)
{
	g_return_val_if_fail (storage != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_STORAGE (storage), NULL);
	g_return_val_if_fail (corba_storage != CORBA_OBJECT_NIL, NULL);

	bonobo_storage_construct (
		BONOBO_STORAGE (storage), corba_storage);

	return BONOBO_STORAGE (storage);
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static BonoboStorage *
do_bonobo_storage_fs_create (const char *path)
{
	BonoboStorageFS *storage_fs;
	Bonobo_Storage corba_storage;

	storage_fs = gtk_type_new (bonobo_storage_fs_get_type ());
	storage_fs->path = g_strdup (path);
	
	corba_storage = bonobo_storage_corba_object_create (
		BONOBO_OBJECT (storage_fs));
	if (corba_storage == CORBA_OBJECT_NIL){
		bonobo_object_unref (BONOBO_OBJECT (storage_fs));
		return NULL;
	}

	return bonobo_storage_construct
		(BONOBO_STORAGE (storage_fs), corba_storage);
}

/** 
 * bonobo_storage_fs_open:
 * @path: path to existing directory that represents the storage
 * @flags: open flags.
 * @mode: mode used if @flags containst Bonobo_Storage_CREATE for the storage.
 *
 * Returns a BonoboStorage object that represents the storage at @path
 */
BonoboStorage *
bonobo_storage_fs_open (const char *path, gint flags, gint mode,
			CORBA_Environment *ev)
{
	struct stat s;
	int v;
	
	g_return_val_if_fail (path != NULL, NULL);

	if (flags & Bonobo_Storage_CREATE) {
		if (mkdir (path, mode) == -1) {
			return NULL;
		}
	}

	v = stat (path, &s);

	if (flags & Bonobo_Storage_READ) {
		if (v == -1)
			return NULL;
		
		if (!S_ISDIR (s.st_mode))
			return NULL;

	} else if (flags & Bonobo_Storage_WRITE) {
		if (v == -1) {
			if (mkdir (path, 0777) == -1)
				return NULL;
		} else {
			if (!S_ISDIR (s.st_mode))
				return NULL;
		}
	}

	return do_bonobo_storage_fs_create (path);
}

gint 
init_storage_plugin (StoragePlugin *plugin)
{
	g_return_val_if_fail (plugin != NULL, -1);

	plugin->name = "fs";
	plugin->description = "Native Filesystem Driver";
	plugin->version = VERSION;
	
	plugin->storage_open = bonobo_storage_fs_open; 
	plugin->stream_open = bonobo_stream_fs_open; 

	return 0;
}

