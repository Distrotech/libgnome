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
	char *path;
} GnomeStorageFS;

typedef struct {
	GnomeStorageClass parent_class;
} GnomeStorageFSClass;

GtkType         gnome_storage_fs_get_type     (void);
GnomeStorage   *gnome_storage_fs_construct    (GnomeStorageFS *storage,
					       GNOME_Storage corba_storage,
					       const char *path, const char *open_mode);
GnomeStorage   *gnome_storage_fs_open         (const char *path,
					       GNOME_Storage_OpenMode mode);
GnomeStorage   *gnome_storage_fs_create       (GnomeStorageFS *storage_fs,
					       const CORBA_char *path);
END_GNOME_DECLS

#endif /* _GNOME_STORAGE_FS_H_ */
