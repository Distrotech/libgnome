/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STREAM_VFS_H_
#define _BONOBO_STREAM_VFS_H_

#include <bonobo/bonobo-stream.h>
#include <libgnomevfs/gnome-vfs.h>

BEGIN_GNOME_DECLS

typedef struct _BonoboStreamVfs BonoboStreamVfs;

#define BONOBO_STREAM_VFS_TYPE        (bonobo_stream_vfs_get_type ())
#define BONOBO_STREAM_VFS(o)          (GTK_CHECK_CAST ((o), BONOBO_STREAM_VFS_TYPE, BonoboStreamVfs))
#define BONOBO_STREAM_VFS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_STREAM_VFS_TYPE, BonoboStreamVfsClass))
#define BONOBO_IS_STREAM_VFS(o)       (GTK_CHECK_TYPE ((o), BONOBO_STREAM_VFS_TYPE))
#define BONOBO_IS_STREAM_VFS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_STREAM_VFS_TYPE))

typedef struct _BonoboStreamVfsPrivate BonoboStreamVfsPrivate;

struct _BonoboStreamVfs {
	BonoboStream    stream;
	GnomeVFSHandle *handle;

	BonoboStreamVfsPrivate *priv;
};

typedef struct {
	BonoboStreamClass parent_class;
} BonoboStreamVfsClass;

GtkType       bonobo_stream_vfs_get_type (void);
BonoboStream *bonobo_stream_vfs_open     (const char *path,
					  gint flags, gint mode,
					  CORBA_Environment *ev);
void bonobo_stream_vfs_storageinfo_from_file_info (Bonobo_StorageInfo *si,
						   GnomeVFSFileInfo   *fi);
	
END_GNOME_DECLS

#endif /* _BONOBO_STREAM_VFS_H_ */
