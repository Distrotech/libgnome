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
#include <bonobo/bonobo-storage.h>
#include "bonobo-storage-vfs.h"
#include "bonobo-stream-vfs.h"

static BonoboObjectClass *bonobo_storage_vfs_parent_class;

static Bonobo_StorageInfo*
vfs_get_info (PortableServer_Servant         storage,
	      const CORBA_char              *path,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment             *ev)
{
	g_warning ("FIXME: get_info not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);

	return CORBA_OBJECT_NIL;
}

static void
vfs_set_info (PortableServer_Servant         storage,
	      const CORBA_char              *path,
	      const Bonobo_StorageInfo      *info,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment             *ev)
{
	g_warning ("FIXME: set_info not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION, 
			     ex_Bonobo_Storage_NotSupported, 
			     NULL);
}

static Bonobo_Stream
vfs_open_stream (PortableServer_Servant  storage,
		 const CORBA_char       *path,
		 Bonobo_Storage_OpenMode mode,
		 CORBA_Environment      *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (
		bonobo_object (storage));
	BonoboStreamVfs *stream;
	char *full;

	full = g_concat_dir_and_file (storage_vfs->path, path);
	stream = bonobo_stream_vfs_open (full, 0664, mode, ev);
	g_free (full);

	return BONOBO_OBJREF (stream);
}

/*
 * Creates the Gtk object and the corba server bound to it
 */
static BonoboStorageVfs *
do_bonobo_storage_vfs_create (const char *path)
{
	BonoboStorageVfs *storage_vfs;

	storage_vfs = g_object_new (bonobo_storage_vfs_get_type (), NULL);
	storage_vfs->path = g_strdup (path);

	return storage_vfs;
}

static void
vfs_rename (PortableServer_Servant storage,
	    const CORBA_char      *path,
	   const CORBA_char       *new_path,
	    CORBA_Environment     *ev)
{
	g_warning ("Not yet implemented");
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_IOError, NULL);
}

static void
vfs_commit (PortableServer_Servant storage,
	    CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static void
vfs_revert (PortableServer_Servant storage,
	    CORBA_Environment     *ev)
{
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NotSupported, NULL);
}

static Bonobo_Storage_DirectoryList *
vfs_list_contents (PortableServer_Servant   storage,
		   const CORBA_char        *path, 
		   Bonobo_StorageInfoFields mask,
		   CORBA_Environment       *ev)
{
	BonoboStorageVfs              *storage_vfs;
	Bonobo_Storage_DirectoryList  *list = NULL;
	GnomeVFSResult                 result;
	GList                         *dir_list, *info;
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

	len  = g_list_length (dir_list);
	list = Bonobo_Storage_DirectoryList__alloc      ();
	list->_buffer = CORBA_sequence_Bonobo_StorageInfo_allocbuf (len);
	list->_length = len;
	CORBA_sequence_set_release (list, TRUE); 

	i = 0;
	for (info = dir_list; info; info = info->next) {
		bonobo_stream_vfs_storageinfo_from_file_info (
			&list->_buffer [i++], info->data);
		gnome_vfs_file_info_unref (info->data);
	}

	g_list_free (dir_list);
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
BonoboStorageVfs *
bonobo_storage_vfs_open (const char *path,
			 gint flags, gint mode,
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

static Bonobo_Storage
vfs_open_storage (PortableServer_Servant  storage,
		  const CORBA_char       *path,
		  Bonobo_Storage_OpenMode mode,
		  CORBA_Environment      *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (
		bonobo_object (storage));
	BonoboStorageVfs *new_storage;
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

	return BONOBO_OBJREF (new_storage);
}

static void
vfs_erase (PortableServer_Servant storage,
	   const CORBA_char      *path,
	   CORBA_Environment     *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (
		bonobo_object (storage));
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
vfs_copy_to (PortableServer_Servant storage,
	     Bonobo_Storage         dest,
	     CORBA_Environment     *ev)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (
		bonobo_object (storage));

	bonobo_storage_copy_to (
		BONOBO_OBJREF (storage_vfs), dest, ev);
}

static void
bonobo_storage_vfs_finalize (GObject *object)
{
	BonoboStorageVfs *storage_vfs = BONOBO_STORAGE_VFS (object);

	g_free (storage_vfs->path);
	storage_vfs->path = NULL;
}

static void
bonobo_storage_vfs_class_init (BonoboStorageVfsClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_Bonobo_Storage__epv *epv = &klass->epv;
	
	bonobo_storage_vfs_parent_class = 
		g_type_class_peek_parent (klass);

	epv->getInfo       = vfs_get_info;
	epv->setInfo       = vfs_set_info;
	epv->openStream    = vfs_open_stream;
	epv->openStorage   = vfs_open_storage;
	epv->copyTo        = vfs_copy_to;
	epv->rename        = vfs_rename;
	epv->commit        = vfs_commit;
	epv->revert        = vfs_revert;
	epv->listContents  = vfs_list_contents;
	epv->erase         = vfs_erase;

	object_class->finalize = bonobo_storage_vfs_finalize;
}

static void 
bonobo_storage_vfs_init (GObject *object)
{
	/* nothing to do */
}

BONOBO_TYPE_FUNC (BonoboStorageVfs,
		  bonobo_object_get_type (),
		  bonobo_storage_vfs);
