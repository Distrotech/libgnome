/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STORAGE_FS_H_
#define _BONOBO_STORAGE_FS_H_

#include <bonobo/bonobo-object.h>

G_BEGIN_DECLS

#define BONOBO_STORAGE_FS_TYPE        (bonobo_storage_fs_get_type ())
#define BONOBO_STORAGE_FS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), BONOBO_STORAGE_FS_TYPE, BonoboStorageFS))
#define BONOBO_STORAGE_FS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), BONOBO_STORAGE_FS_TYPE, BonoboStorageFSClass))
#define BONOBO_IS_STORAGE_FS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), BONOBO_STORAGE_FS_TYPE))
#define BONOBO_IS_STORAGE_FS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), BONOBO_STORAGE_FS_TYPE))

typedef struct {
	BonoboObject parent;
	char *path;
} BonoboStorageFS;

typedef struct {
	BonoboObjectClass parent_class;

	POA_Bonobo_Storage__epv epv;
} BonoboStorageFSClass;

GType            bonobo_storage_fs_get_type (void);
BonoboStorageFS *bonobo_storage_fs_open     (const char        *path,
					     gint               flags, 
					     gint               mode,
					     CORBA_Environment *ev);

G_END_DECLS

#endif /* _BONOBO_STORAGE_FS_H_ */
