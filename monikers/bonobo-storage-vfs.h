/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STORAGE_VFS_H_
#define _BONOBO_STORAGE_VFS_H_

#include <bonobo/bonobo-storage.h>

BEGIN_GNOME_DECLS

#define BONOBO_STORAGE_VFS_TYPE        (bonobo_storage_vfs_get_type ())
#define BONOBO_STORAGE_VFS(o)          (GTK_CHECK_CAST ((o), BONOBO_STORAGE_VFS_TYPE, BonoboStorageVfs))
#define BONOBO_STORAGE_VFS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_STORAGE_VFS_TYPE, BonoboStorageVfsClass))
#define BONOBO_IS_STORAGE_VFS(o)       (GTK_CHECK_TYPE ((o), BONOBO_STORAGE_VFS_TYPE))
#define BONOBO_IS_STORAGE_VFS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_STORAGE_VFS_TYPE))

typedef struct {
	BonoboStorage storage;
	char *path;
} BonoboStorageVfs;

typedef struct {
	BonoboStorageClass parent_class;
} BonoboStorageVfsClass;

GtkType         bonobo_storage_vfs_get_type     (void);

END_GNOME_DECLS

#endif /* _BONOBO_STORAGE_VFS_H_ */
