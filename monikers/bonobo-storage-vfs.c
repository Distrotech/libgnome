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
#include <storage-modules/bonobo-stream-vfs.h>
#include <bonobo/bonobo-storage-plugin.h>

static BonoboStorageClass *bonobo_storage_vfs_parent_class;

static Bonobo_StorageInfo*
vfs_get_info (BonoboStorage *storage,
	      const CORBA_char *path,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment *ev)
{
	g_warning ("FIXME: get_info not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);
	return CORBA_OBJECT_NIL;
}

static void
vfs_set_info (BonoboStorage *storage,
	      const CORBA_char *path,
	      const Bonobo_StorageInfo *info,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment *ev)
{
	g_warning ("FIXME: set_info not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);
}

static BonoboStream *
vfs_open_stream (BonoboStorage *storage, const CORBA_char *path,
		Bonobo_Storage_OpenMode mode, CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	BonoboStream *stream;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);
	stream = bonobo_stream_vfs_open (full, 0664, mode, ev);
	g_free (full);

	return stream;
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static BonoboStorage *
do_bonobo_storage_vfs_create (const char *path)
{
	BonoboStorageVfs *storage_vfs;
	Bonobo_Storage corba_storage;

	storage_vfs = gtk_type_new (bonobo_storage_vfs_get_type ());
	storage_vfs->path = g_strdup (path);
	
	corba_storage = bonobo_storage_corba_object_create (
		BONOBO_OBJECT (storage_vfs));
	if (corba_storage == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (storage_vfs));
		return NULL;
	}

	return bonobo_storage_construct (
		BONOBO_STORAGE (storage_vfs), corba_storage);
}

static void
vfs_rename (BonoboStorage *storage, const CORBA_char *path,
	   const CORBA_char *new_path, CORBA_Environment *ev)
{
	g_warning ("Not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_IOError, NULL);
}

static void
vfs_commit (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_revert (BonoboStorage *storage, CORBA_Environment *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static Bonobo_Storage_DirectoryList *
vfs_list_contents (BonoboStorage *storage, const CORBA_char *path, 
		   Bonobo_StorageInfoFields mask, CORBA_Environment *ev)
{
	BonoboStorageVfs              *storage_vfs;
	Bonobo_Storage_DirectoryList  *list = NULL;
	GnomeVFSResult                 result;
	GnomeVFSDirectoryList         *dir_list;
	GnomeVFSFileInfo              *info;
	char                          *uri;
	int                            len, i;

	storage_vfs = BONOBO_STORAGE_VFS (storage);

	uri = g_concat_dir_and_file (storage_vfs->path, path);

	result = gnome_vfs_directory_list_load (
		&dir_list, uri, 
		(mask & Bonobo_FIELD_CONTENT_TYPE) ?
		GNOME_VFS_FILE_INFO_GET_MIME_TYPE :
		GNOME_VFS_FILE_INFO_DEFAULT, NULL);

	if (result != GNOME_VFS_OK) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NotFound, NULL);
		g_free (uri);
		return NULL;
	}

	len  = gnome_vfs_directory_list_get_num_entries (dir_list);
	list = Bonobo_Storage_DirectoryList__alloc      ();
	list->_buffer = CORBA_sequence_Bonobo_StorageInfo_allocbuf (len);
	list->_length = len;
	CORBA_sequence_set_release (list, TRUE); 

	i = 0;
	for (info = gnome_vfs_directory_list_first (dir_list);
	     info; info = gnome_vfs_directory_list_next (dir_list))

		bonobo_stream_vfs_storageinfo_from_file_info (
			&list->_buffer [i++], info);

	gnome_vfs_directory_list_destroy (dir_list);

	g_free (uri);

	return list;
}

/** 
 * bonobo_storage_vfs_open:
 * @path: path to existing directory that represents the storage
 * @flags: open flags.
 * @mode: mode used if @flags containst BONOBO_SS_CREATE for the storage.
 *
 * Returns a BonoboStorage object that represents the storage at @path
 */
static BonoboStorage *
bonobo_storage_vfs_open (const char *path, gint flags, gint mode,
			 CORBA_Environment *ev)
{
	GnomeVFSResult    result;
	GnomeVFSFileInfo *info;
	gboolean          create = FALSE;
	
	g_return_val_if_fail (path != NULL, NULL);

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (
		path, info, GNOME_VFS_FILE_INFO_DEFAULT);

	if (result == GNOME_VFS_ERROR_NOT_FOUND &&
	    (flags & Bonobo_Storage_CREATE))
		create = TRUE;
	    
	else if (flags & Bonobo_Storage_READ) {
		if (result != GNOME_VFS_OK) {
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_NoPermission, NULL);
			return NULL;
		}

		if ((info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) &&
		    (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)) {
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_IOError, NULL);
			return NULL;
		}

	} else if (flags & (Bonobo_Storage_WRITE)) {
		if (result == GNOME_VFS_ERROR_NOT_FOUND)
			create = TRUE;
		else
			if ((info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) &&
			    (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)) {
				CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
						     ex_Bonobo_Stream_IOError, NULL);
				return NULL;
			}
	}
	gnome_vfs_file_info_unref (info);

	if (create) {
		result = gnome_vfs_make_directory (
			path, GNOME_VFS_PERM_USER_ALL |
			GNOME_VFS_PERM_GROUP_ALL);

		if (result != GNOME_VFS_OK) {
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Stream_NoPermission, NULL);
			return NULL;
		}
	}

	return do_bonobo_storage_vfs_create (path);
}

static BonoboStorage *
vfs_open_storage (BonoboStorage *storage,
		  const CORBA_char *path,
		  Bonobo_Storage_OpenMode mode,
		  CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	BonoboStorage    *new_storage;
	GnomeVFSResult    result;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);

	result = gnome_vfs_make_directory (full, GNOME_VFS_PERM_USER_ALL);
	if (result == GNOME_VFS_OK ||
	    result == GNOME_VFS_ERROR_FILE_EXISTS)
		new_storage = do_bonobo_storage_vfs_create (full);
	else
		new_storage = NULL;

	g_free (full);

	return new_storage;
}

static void
vfs_erase (BonoboStorage *storage,
	   const CORBA_char *path,
	   CORBA_Environment *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (storage);
	GnomeVFSResult    result;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);

	result = gnome_vfs_unlink (full);

	g_free (full);

	if (result != GNOME_VFS_OK)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
				     ex_Bonobo_Storage_NoPermission, 
				     NULL);
}

static void
bonobo_storage_vfs_destroy (GtkObject *object)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (object);

	g_free (storage_vfs->path);
}

static void
bonobo_storage_vfs_class_init (BonoboStorageVfsClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;
	BonoboStorageClass *sclass = BONOBO_STORAGE_CLASS (class);
	
	bonobo_storage_vfs_parent_class = gtk_type_class (bonobo_storage_get_type ());

	sclass->get_info       = vfs_get_info;
	sclass->set_info       = vfs_set_info;
	sclass->open_stream    = vfs_open_stream;
	sclass->open_storage   = vfs_open_storage;
	sclass->copy_to        = NULL; /* use the generic method */
	sclass->rename         = vfs_rename;
	sclass->commit         = vfs_commit;
	sclass->revert         = vfs_revert;
	sclass->list_contents  = vfs_list_contents;
	sclass->erase          = vfs_erase;

	object_class->destroy = bonobo_storage_vfs_destroy;
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
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_storage_get_type (), &info);
	}

	return type;
}

gint 
init_storage_plugin (StoragePlugin *plugin)
{
	g_return_val_if_fail (plugin != NULL, -1);

	plugin->name = "vfs";
	plugin->description = "Gnome Virtual Filesystem Driver";
	plugin->version = BONOBO_STORAGE_VERSION;
	
	plugin->storage_open = bonobo_storage_vfs_open; 
	plugin->stream_open  = bonobo_stream_vfs_open; 

	return 0;
}
