/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _BONOBO_STREAM_FS_H_
#define _BONOBO_STREAM_FS_H_

#include <bonobo/bonobo-stream.h>

BEGIN_GNOME_DECLS

struct _BonoboStreamFS;
typedef struct _BonoboStreamFS BonoboStreamFS;

#ifndef _BONOBO_STORAGE_FS_H_
struct _BonoboStorageFS;
typedef struct _BonoboStorageFS BonoboStorageFS;
#endif

#define BONOBO_STREAM_FS_TYPE        (bonobo_stream_fs_get_type ())
#define BONOBO_STREAM_FS(o)          (GTK_CHECK_CAST ((o), BONOBO_STREAM_FS_TYPE, BonoboStreamFS))
#define BONOBO_STREAM_FS_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), BONOBO_STREAM_FS_TYPE, BonoboStreamFSClass))
#define BONOBO_IS_STREAM_FS(o)       (GTK_CHECK_TYPE ((o), BONOBO_STREAM_FS_TYPE))
#define BONOBO_IS_STREAM_FS_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), BONOBO_STREAM_FS_TYPE))

typedef struct _BonoboStreamFSPrivate BonoboStreamFSPrivate;

struct _BonoboStreamFS {
	BonoboStream stream;
	int fd;
	char *path;

	BonoboStreamFSPrivate *priv;
};

typedef struct {
	BonoboStreamClass parent_class;
} BonoboStreamFSClass;

GtkType         bonobo_stream_fs_get_type     (void);
BonoboStream    *bonobo_stream_fs_open         (const CORBA_char *path, Bonobo_Storage_OpenMode mode);
BonoboStream    *bonobo_stream_fs_create       (const CORBA_char *path);
BonoboStream    *bonobo_stream_fs_construct    (BonoboStreamFS *stream,
					      Bonobo_Stream corba_stream);
	
END_GNOME_DECLS

#endif /* _BONOBO_STREAM_FS_H_ */
