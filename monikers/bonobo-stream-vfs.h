/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STREAM_VFS_H_
#define _BONOBO_STREAM_VFS_H_

#include <bonobo/bonobo-object.h>
#include <libgnomevfs/gnome-vfs.h>

G_BEGIN_DECLS

typedef struct _BonoboStreamVfs BonoboStreamVfs;


#define BONOBO_STREAM_VFS_TYPE        (bonobo_stream_vfs_get_type ())
#define BONOBO_STREAM_VFS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), BONOBO_STREAM_VFS_TYPE, BonoboStreamVfs))
#define BONOBO_STREAM_VFS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), BONOBO_STREAM_VFS_TYPE, BonoboStreamVfsClass))
#define BONOBO_IS_STREAM_VFS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), BONOBO_STREAM_VFS_TYPE))
#define BONOBO_IS_STREAM_VFS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), BONOBO_STREAM_VFS_TYPE))

typedef struct _BonoboStreamVfsPrivate BonoboStreamVfsPrivate;

struct _BonoboStreamVfs {
	BonoboObject            parent;
	GnomeVFSHandle         *handle;

	BonoboStreamVfsPrivate *priv;
};

typedef struct {
	BonoboObjectClass       parent_class;

	POA_Bonobo_Stream__epv  epv;
} BonoboStreamVfsClass;

GType            bonobo_stream_vfs_get_type (void);
BonoboStreamVfs *bonobo_stream_vfs_open     (const char *path,
					     gint flags, gint mode,
					     CORBA_Environment *ev);
void bonobo_stream_vfs_storageinfo_from_file_info (Bonobo_StorageInfo *si,
						   GnomeVFSFileInfo   *fi);
	
G_END_DECLS

#endif /* _BONOBO_STREAM_VFS_H_ */
