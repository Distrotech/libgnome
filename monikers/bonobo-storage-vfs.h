/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STORAGE_VFS_H_
#define _BONOBO_STORAGE_VFS_H_

#include <bonobo/bonobo-object.h>

G_BEGIN_DECLS

#define BONOBO_STORAGE_VFS_TYPE        (bonobo_storage_vfs_get_type ())
#define BONOBO_STORAGE_VFS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), BONOBO_STORAGE_VFS_TYPE, BonoboStorageVfs))
#define BONOBO_STORAGE_VFS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), BONOBO_STORAGE_VFS_TYPE, BonoboStorageVfsClass))
#define BONOBO_IS_STORAGE_VFS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), BONOBO_STORAGE_VFS_TYPE))
#define BONOBO_IS_STORAGE_VFS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), BONOBO_STORAGE_VFS_TYPE))

typedef struct {
	BonoboObject parent;
	char        *path;
} BonoboStorageVfs;

typedef struct {
	BonoboObjectClass       parent_class;

	POA_Bonobo_Storage__epv epv;
} BonoboStorageVfsClass;

GType             bonobo_storage_vfs_get_type (void);
BonoboStorageVfs *bonobo_storage_vfs_open     (const char *path,
					       gint flags, gint mode,
					       CORBA_Environment *ev);

G_END_DECLS

#endif /* _BONOBO_STORAGE_VFS_H_ */
