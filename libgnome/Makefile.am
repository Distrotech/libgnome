## Process this file with automake to produce Makefile.in

#
# Targets
#

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

lib_LTLIBRARIES = libgnome-2.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/gnome/$(GNOME_INTERFACE_VERSION)/libgnome

INCLUDES = \
	-I..					\
	-I$(srcdir)/..				\
	$(GNOMESUPPORT_CFLAGS)			\
	$(GLIB_CFLAGS)				\
	$(GNOME_VFS_CFLAGS)			\
	-DGNOMELIBDIR=\""$(libdir)"\"		\
	-DGNOMEDATADIR=\""$(datadir)"\"		\
	-DGNOMEBINDIR=\""$(bindir)"\"		\
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" \
	-DGNOMESYSCONFDIR=\""$(sysconfdir)"\"	\
	-DVERSION=\""$(VERSION)"\"		\
	-DGNOMEVFSVERSION=\""$(GNOME_VFS_VERSION)"\" \
	-DG_LOG_DOMAIN=\"Gnome\"

libgnome_2_la_SOURCES = \
	gnome-config.c		\
	gnome-ditem.c		\
	gnome-exec.c		\
	gnome-fileconvert.c	\
	gnome-i18n.c		\
	gnome-magic.c		\
	gnome-metadata.c	\
	gnome-mime.c		\
	gnome-mime-info.c	\
	gnome-regex.c		\
	gnome-remote.c		\
	gnome-score.c		\
	gnome-triggers.c	\
	gnome-url.c		\
	gnome-util.c		\
	gnome-paper.c		\
	gnomelib-init.c		\
	gnomelib-init2.c

libgnomeinclude_HEADERS = \
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-ditem.h		\
	gnome-exec.h		\
	gnome-i18n.h 		\
	gnome-magic.h		\
	gnome-metadata.h	\
	gnome-mime.h		\
	gnome-mime-info.h	\
	gnome-regex.h		\
	gnome-remote.h		\
	gnome-score.h		\
	gnome-triggers.h 	\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-paper.h		\
	libgnome.h		\
	gnomelib-init2.h	\
	gnomelib-init.h	\
	gnome-portability.h

noinst_HEADERS = \
	gnome-i18nP.h				\
	gnome-triggersP.h			\
	libgnomeP.h

EXTRA_DIST = parse-path.cP gnome-portability.h.in
BUILT_SOURCES = gnome-portability.h

#just to be sure, add an explicit dependency
gnome-config.o: parse-path.cP

libgnome_2_la_LDFLAGS = $(LIBGNOME_LIBS) -version-info $(LIB_VERSION)
libgnome_2_la_LIBADD = $(GNOMESUPPORT_LIBS)

bin_PROGRAMS = \
	gnome-dump-metadata	\
	gnome-moz-remote	\
	gnome-gen-mimedb

gnome_dump_metadata_SOURCES = \
	gnome-dump.c
gnome_dump_metadata_LDADD = \
	libgnome-$(GNOME_INTERFACE_VERSION).la	\
	$(LIBGNOME_LIBS)			\
	$(INTLLIBS)
gnome_dump_metadata_LDFLAGS = $(GLIB_LIBS)

gnome_moz_remote_SOURCES = gnome-moz-remote2.c vroot.h
gnome_moz_remote_LDADD = \
	libgnome-$(GNOME_INTERFACE_VERSION).la	\
	$(LIBGNOME_LIBS)			\
	$(X_LIBS)				\
	$(X_PRE_LIBS)				\
	-lXext					\
	-lX11					\
	$(X_EXTRA_LIBS)				\
	$(INTLLIBS)

gnome_gen_mimedb_SOURCES = gnome-gen-mimedb.c
gnome_gen_mimedb_LDADD = \
	libgnome-$(GNOME_INTERFACE_VERSION).la	\
	$(LIBGNOME_LIBS)			\
	$(INTLLIBS)
