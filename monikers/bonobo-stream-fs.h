#ifndef _GNOME_STREAM_FS_H_
#define _GNOME_STREAM_FS_H_

#include <bonobo/gnome-stream.h>

BEGIN_GNOME_DECLS

#define GNOME_STREAM_FS_TYPE        (gnome_stream_fs_get_type ())
#define GNOME_STREAM_FS(o)          (GTK_CHECK_CAST ((o), GNOME_STREAM_FS_TYPE, GnomeStreamFS))
#define GNOME_STREAM_FS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), GNOME_STREAM_FS_TYPE, GnomeStreamFSClass))
#define GNOME_IS_STREAM_FS(o)       (GTK_CHECK_TYPE ((o), GNOME_STREAM_FS_TYPE))
#define GNOME_IS_STREAM_FS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), GNOME_STREAM_FS_TYPE))

typedef struct {
	GnomeStream stream;
	int fd;
} GnomeStreamFS;

typedef struct {
	GnomeStreamClass parent_class;
} GnomeStreamClass;

END_GNOME_DECLS

#endif /* _GNOME_STREAM_FS_H_ */
