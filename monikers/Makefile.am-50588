
INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(top_srcdir)/libefs/src			\
        -I$(includedir)                                 \
	$(VFS_CFLAGS)					\
        $(GNOME_INCLUDEDIR)

common_ldflags = -avoid-version


#
# EFS storage module
#
libstorage_efs_la_SOURCES = 	\
	gnome-storage-efs.c	\
	gnome-storage-efs.h	\
	gnome-stream-efs.c	\
	gnome-stream-efs.h

libstorage_efs_la_LDFLAGS = 			\
	$(common_ldflags) 			\
	-L$(shell pwd)/$(top_builddir)/libefs/src/.libs\
	-lefs

#
# File-system based storage
#
libstorage_fs_la_SOURCES =	\
	gnome-storage-fs.c	\
	gnome-storage-fs.h

libstorage_fs_la_LDFLAGS = 	\
	$(common_ldflags)

#
# VFS storage module - disabled for now.
#
if HAVE_VFS
libstoragedir = $(libdir)
libstorage_LTLIBRARIES =	\
	libstorage_vfs.la

libstorage_vfs_la_SOURCES =	\
	bonobo-storage-vfs.c	\
	bonobo-storage-vfs.h	\
	bonobo-stream-vfs.c	\
	bonobo-stream-vfs.h

libstorage_vfs_la_LIBADD = 	\
	$(VFS_LIBS)
endif

lib_LTLIBRARIES = 		\
	libstorage_fs.la	\
	libstorage_efs.la
