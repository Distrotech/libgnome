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
	$(WARN_CFLAGS)				\
	$(GNOMESUPPORT_CFLAGS)			\
	$(GLIB_CFLAGS)				\
	$(GNOME_VFS_CFLAGS)			\
	-DGNOMEPREFIX=\""$(prefix)"\"		\
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
	gnome-program.c		\
	gnome-config.c		\
	gnome-ditem.c		\
	gnome-exec.c		\
	gnome-fileconvert.c	\
	gnome-i18n.c		\
	gnome-regex.c		\
	gnome-remote.c		\
	gnome-score.c		\
	gnome-triggers.c	\
	gnome-url.c		\
	gnome-util.c		\
	gnome-paper.c		\
	gnomelib-init.c

libgnomeinclude_HEADERS = \
	gnome-program.h		\
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-ditem.h		\
	gnome-exec.h		\
	gnome-i18n.h 		\
	gnome-regex.h		\
	gnome-remote.h		\
	gnome-score.h		\
	gnome-triggers.h 	\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-paper.h		\
	libgnome.h		\
	gnomelib-init.h		\
	gnome-portability.h

noinst_HEADERS = \
	gnome-i18nP.h				\
	gnome-triggersP.h			\
	libgnomeP.h

EXTRA_DIST = parse-path.cP gnome-portability.h.in libgnome-2.0.pc.in
BUILT_SOURCES = gnome-portability.h

#just to be sure, add an explicit dependency
gnome-config.o: parse-path.cP

libgnome_2_la_LDFLAGS = $(LIBGNOME_LIBS) -version-info $(LIB_VERSION)
libgnome_2_la_LIBADD = $(GNOMESUPPORT_LIBS)

bin_PROGRAMS = \
	gnome-moz-remote

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

noinst_PROGRAMS = \
	test-libgnome

test_libgnome_SOURCES = test-libgnome.c
test_libgnome_LDADD = \
	libgnome-2.la


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnome-2.0.pc

