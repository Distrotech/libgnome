#ifndef _GNOME_STORAGE_FS_H_
#define _GNOME_STORAGE_FS_H_

#include <bonobo/gnome-storage.h>

BEGIN_GNOME_DECLS

#define GNOME_STORAGE_FS_TYPE        (gnome_storage_fs_get_type ())
#define GNOME_STORAGE_FS(o)          (GTK_CHECK_CAST ((o), GNOME_STORAGE_FS_TYPE, GnomeStorageFS))
#define GNOME_STORAGE_FS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_STORAGE_FS_TYPE, GnomeStorageFSClass))
#define GNOME_IS_STORAGE_FS(o)       (GTK_CHECK_TYPE ((o), GNOME_STORAGE_FS_TYPE))
#define GNOME_IS_STORAGE_FS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_STORAGE_FS_TYPE))

typedef struct {
	GnomeStorage storage;
} GnomeStorageFS;

typedef struct {
	GnomeStorageClass parent_class;
} GnomeStorageClass;

END_GNOME_DECLS

#endif /* _GNOME_STORAGE_FS_H_ */
