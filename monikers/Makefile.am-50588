
INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-I$(top_srcdir)/libefs/src			\
        -I$(includedir)                                 \
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
	-L$(top_builddir)/libefs/src		\
	-lefs

#
# File-system based storage
#
libstorage_fs_la_SOURCES =	\
	gnome-storage-fs.c	\
	gnome-storage-fs.h

libstorage_fs_la_LDFLAGS = 	\
	$(common_ldflags)

lib_LTLIBRARIES = 		\
	libstorage_fs.la	\
	libstorage_efs.la
