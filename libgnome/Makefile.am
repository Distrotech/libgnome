## Process this file with automake to produce Makefile.in

#
# Targets
#
lib_LTLIBRARIES = libgnome.la

scoredir=$(localstatedir)/games

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(SUPPORTINCS) \
        $(GDK_IMLIB_CFLAGS)  \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DGNOMESYSCONFDIR=\""$(sysconfdir)"\" \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl


libgnome_la_SOURCES = \
	gnome-config.c		\
	gnome-dentry.c          \
	gnome-dl.c		\
	gnome-exec.c		\
	gnome-fileconvert.c	\
	gnome-help.c		\
	gnome-history.c		\
	gnome-hook.c 		\
	gnome-i18n.c		\
	gnome-metadata.c	\
	gnome-mime.c		\
	gnome-parse.c		\
	gnome-regex.c		\
	gnome-remote.c		\
	gnome-score.c		\
	gnome-sound.c		\
	gnome-string.c		\
	gnome-triggers.c	\
	gnome-url.c		\
	gnome-util.c		\
	gnome-paper.c		\
	gnomelib-init.c         \
	lib_date.c

libgnomeinclude_HEADERS = \
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-hook.h   		\
	gnome-dl.h		\
	gnome-dentry.h 		\
	gnome-exec.h		\
	gnome-help.h		\
	gnome-history.h		\
	gnome-i18n.h 		\
	gnome-metadata.h	\
	gnome-mime.h		\
	gnome-regex.h		\
	gnome-remote.h		\
	gnome-parse.h		\
	gnome-score.h		\
	gnome-sound.h		\
	gnome-string.h		\
	gnome-triggers.h 	\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-paper.h		\
	lib_date.h		\
	lib_defs.h		\
	libgnome.h

noinst_HEADERS = \
	gnome-i18nP.h		\
	libgnomeP.h

EXTRA_DIST = lib_date.README

sysconf_DATA = paper.config

libgnome_la_LDFLAGS = -version-info 30:0:30 -rpath $(libdir)
libgnome_la_LIBADD = $(GLIB_LIBS) -lm $(DL_LIB)

bin_PROGRAMS = dns-helper gnome-dump-metadata gnome-moz-remote

dns_helper_SOURCES = dns-helper.c
dns_helper_LDADD = $(NSL_LIBS)

gnome_dump_metadata_SOURCES = gnome-dump.c
gnome_dump_metadata_LDADD = libgnome.la $(LIBSUPPORT) \
	$(LIBGNOME_LIBS) $(INTLLIBS)
gnome_dump_metadata_LDFLAGS = `glib-config --libs`

gnome_moz_remote_SOURCES = gnome-moz-remote.c vroot.h
gnome_moz_remote_LDADD = libgnome.la $(LIBSUPPORT) $(LIBGNOME_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXmu -lXt -lX11 $(X_EXTRA_LIBS) $(INTLLIBS)


install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp games $(scoredir) && chmod g=rwXs $(scoredir)
