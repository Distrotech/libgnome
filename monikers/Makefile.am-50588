PLUGIN_DIR=$(libdir)/bonobo/plugin

INCLUDES =                                              \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-I$(top_srcdir)/libefs/src			\
        -I$(includedir)                                 \
	$(VFS_CFLAGS)					\
        $(GNOME_INCLUDEDIR)

common_ldflags = -avoid-version

bonobo_plugindir =  $(PLUGIN_DIR)
bonobo_plugin_LTLIBRARIES = libstorage_efs.la libstorage_fs.la 

#
# EFS storage module
#
libstorage_efs_la_SOURCES = 	\
	bonobo-storage-efs.c	\
	bonobo-storage-efs.h	\
	bonobo-stream-efs.c	\
	bonobo-stream-efs.h

libstorage_efs_la_LDFLAGS = 			\
	$(common_ldflags) 			\
	-L$(top_builddir)/libefs/src		\
	-lefs

#
# File-system based storage
#
libstorage_fs_la_SOURCES =	\
	bonobo-storage-fs.c	\
	bonobo-storage-fs.h	\
	bonobo-stream-fs.c	\
	bonobo-stream-fs.h

libstorage_fs_la_LDFLAGS = 	\
	$(common_ldflags)

#
# VFS storage module - disabled for now.
#
if HAVE_VFS
libstoragedir = $(PLUGIN_DIR)
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
