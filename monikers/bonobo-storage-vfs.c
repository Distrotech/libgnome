/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gnome-storage-vfs.c: Gnome VFS based storage implementation
 *
 * Author:
 *   Michael Meeks <michael@helixcode.com>
 */
#include <config.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <storage-modules/bonobo-storage-vfs.h>
#include "bonobo-stream-vfs.h"

static BonoboStorageClass *bonobo_storage_vfs_parent_class;

static void
bonobo_storage_vfs_destroy (GtkObject *object)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (object);

	g_free (storage_vfs->path);
}

static BonoboStream *
fs_create_stream (BonoboStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	BonoboStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);
	stream = bonobo_stream_vfs_create (full);
	g_free (full);

	return stream;
}

static BonoboStream *
fs_open_stream (BonoboStorage *storage, const CORBA_char *path,
		Bonobo_Storage_OpenMode mode, CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	BonoboStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);
	stream = bonobo_stream_vfs_open (full, mode);
	g_free (full);

	return stream;
}

static BonoboStorage *
fs_create_storage (BonoboStorage *storage, const CORBA_char *path, CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	BonoboStorage *new_storage;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);
	new_storage = bonobo_storage_vfs_create (BONOBO_STORAGE_VFS (storage), path);
	g_free (full);

	return new_storage;
}

static void
fs_copy_to (BonoboStorage *storage, Bonobo_Storage target, CORBA_Environment *ev)
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
}

static Bonobo_Storage_directory_list *
fs_list_contents (BonoboStorage *storage, const CORBA_char *path,
		  CORBA_Environment *ev)
{
	g_error ("Not yet implemented");

	return NULL;
}

static void
bonobo_storage_vfs_class_init (BonoboStorageVfsClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	BonoboStorageClass *sclass = BONOBO_STORAGE_CLASS (class);
	
	bonobo_storage_vfs_parent_class = gtk_type_class (bonobo_storage_get_type ());

	sclass->create_stream  = fs_create_stream;
	sclass->open_stream    = fs_open_stream;
	sclass->create_storage = fs_create_storage;
	sclass->copy_to        = fs_copy_to;
	sclass->rename         = fs_rename;
	sclass->commit         = fs_commit;
	sclass->list_contents  = fs_list_contents;
	
	object_class->destroy = bonobo_storage_vfs_destroy;
}

static void
bonobo_storage_init (BonoboObject *object)
{
}

GtkType
bonobo_storage_vfs_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"IDL:GNOME/StorageVfs:1.0",
			sizeof (BonoboStorageVfs),
			sizeof (BonoboStorageVfsClass),
			(GtkClassInitFunc) bonobo_storage_vfs_class_init,
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
bonobo_storage_vfs_construct (BonoboStorageVfs *storage,
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

static Bonobo_Storage
create_bonobo_storage_vfs (BonoboObject *object)
{
	POA_Bonobo_Storage *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_Storage *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_storage_vepv;

	CORBA_exception_init (&ev);
	POA_Bonobo_Storage__init ((PortableServer_Servant) servant, &ev);
	if (ev._major != CORBA_NO_EXCEPTION){
                g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
        }
	CORBA_exception_free (&ev);

	return (Bonobo_Storage) bonobo_object_activate_servant (object, servant);
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static BonoboStorage *
do_bonobo_storage_vfs_create (char *path)
{
	BonoboStorageVfs *storage_vfs;
	Bonobo_Storage corba_storage;

	storage_vfs = gtk_type_new (bonobo_storage_vfs_get_type ());
	storage_vfs->path = g_strdup (path);
	
	corba_storage = create_bonobo_storage_vfs (
		BONOBO_OBJECT (storage_vfs));
	if (corba_storage == CORBA_OBJECT_NIL){
		gtk_object_destroy (GTK_OBJECT (storage_vfs));
		return NULL;
	}

	return bonobo_storage_construct (
		BONOBO_STORAGE (storage_vfs), corba_storage);
}

/** 
 * bonobo_storage_vfs_open:
 * @path: path to existing directory that represents the storage
 * @flags: open flags.
 * @mode: mode used if @flags containst BONOBO_SS_CREATE for the storage.
 *
 * Returns a BonoboStorage object that represents the storage at @path
 */
BonoboStorage *
bonobo_storage_vfs_open (const char *path, gint flags, gint mode)
{
	struct stat s;
	int v;
	
	g_return_val_if_fail (path != NULL, NULL);

	if (flags & BONOBO_SS_CREATE){
		if (mkdir (path, mode) == -1){
			return NULL;
		}
	}

	v = stat (path, &s);

	if (flags & BONOBO_SS_READ) {
		if (v == -1)
			return NULL;
		
		if (!S_ISDIR (s.st_mode))
			return NULL;

	} else if (flags & (BONOBO_SS_RDWR|BONOBO_SS_WRITE)){
		if (v == -1) {
			if (mkdir (path, 0777) == -1)
				return NULL;
		} else {
			if (!S_ISDIR (s.st_mode))
				return NULL;
		}
	}

	return do_bonobo_storage_vfs_create (g_strdup (path));
}

/*
 * Shared library entry point
 */
BonoboStorage *
bonobo_storage_driver_open (const char *path, gint flags, gint mode)
{
	return bonobo_storage_vfs_open (path, flags, mode);
}

