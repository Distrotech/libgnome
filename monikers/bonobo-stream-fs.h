#ifndef _GNOME_STREAM_FS_H_
#define _GNOME_STREAM_FS_H_

#include <bonobo/gnome-stream.h>

BEGIN_GNOME_DECLS

struct _GnomeStreamFS;
typedef struct _GnomeStreamFS GnomeStreamFS;

#ifndef _GNOME_STORAGE_FS_H_
struct _GnomeStorageFS;
typedef struct _GnomeStorageFS GnomeStorageFS;
#endif

#define GNOME_STREAM_FS_TYPE        (gnome_stream_fs_get_type ())
#define GNOME_STREAM_FS(o)          (GTK_CHECK_CAST ((o), GNOME_STREAM_FS_TYPE, GnomeStreamFS))
#define GNOME_STREAM_FS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_STREAM_FS_TYPE, GnomeStreamFSClass))
#define GNOME_IS_STREAM_FS(o)       (GTK_CHECK_TYPE ((o), GNOME_STREAM_FS_TYPE))
#define GNOME_IS_STREAM_FS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_STREAM_FS_TYPE))

struct _GnomeStreamFS {
	GnomeStream stream;
	int fd;
	char *path;
};

typedef struct {
	GnomeStreamClass parent_class;
} GnomeStreamFSClass;

GtkType         gnome_stream_fs_get_type     (void);
GnomeStream    *gnome_stream_fs_open         (GnomeStorageFS *parent,
					      const CORBA_char *path, GNOME_Storage_OpenMode mode);
GnomeStream    *gnome_stream_fs_create       (GnomeStorageFS *fs,
					      const CORBA_char *path);
GnomeStream    *gnome_stream_fs_construct    (GnomeStreamFS *stream,
					      GNOME_Stream corba_stream);
	
END_GNOME_DECLS

#endif /* _GNOME_STREAM_FS_H_ */
