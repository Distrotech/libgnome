/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-stream-fs.c: Sample file-system based Stream implementation
 *
 * This is just a sample file-system based Stream implementation.
 * it is only used for debugging purposes
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 */
#ifndef _BONOBO_STREAM_FS_H_
#define _BONOBO_STREAM_FS_H_

#include <bonobo/bonobo-object.h>

G_BEGIN_DECLS

#define BONOBO_STREAM_FS_TYPE        (bonobo_stream_fs_get_type ())
#define BONOBO_STREAM_FS(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), BONOBO_STREAM_FS_TYPE, BonoboStreamFS))
#define BONOBO_STREAM_FS_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), BONOBO_STREAM_FS_TYPE, BonoboStreamFSClass))
#define BONOBO_IS_STREAM_FS(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), BONOBO_STREAM_FS_TYPE))
#define BONOBO_IS_STREAM_FS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), BONOBO_STREAM_FS_TYPE))

typedef struct _BonoboStreamFS BonoboStreamFS;
typedef struct _BonoboStreamFSPrivate BonoboStreamFSPrivate;

struct _BonoboStreamFS {
	BonoboObject stream;
	int fd;
	char *path;

	BonoboStreamFSPrivate *priv;
};

typedef struct {
	BonoboObjectClass parent_class;

	POA_Bonobo_Stream__epv epv;
} BonoboStreamFSClass;

GType           bonobo_stream_fs_get_type (void);
BonoboStreamFS *bonobo_stream_fs_open     (const char *path, 
					   gint flags, gint mode, 
					   CORBA_Environment *ev);	

G_END_DECLS

#endif /* _BONOBO_STREAM_FS_H_ */
