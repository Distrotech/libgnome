## Process this file with automake to produce Makefile.in

#
# Targets
#
lib_LTLIBRARIES = libgnome.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. $(SUPPORTINCS) \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	-I$(includedir) $(GLIB_CFLAGS) $(GDK_IMLIB_CFLAGS)  \
	$(AUDIOFILE_CFLAGS) $(ESD_CFLAGS) \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" \
	-DGNOMESYSCONFDIR=\""$(sysconfdir)"\" \
	-DVERSION=\""$(VERSION)"\" \
	-DG_LOG_DOMAIN=\"Gnome\"

libgnome_la_SOURCES = \
	gnome-config.c		\
	gnome-dentry.c          \
	gnome-exec.c		\
	gnome-fileconvert.c	\
	gnome-help.c		\
	gnome-history.c		\
	gnome-i18n.c		\
	gnome-magic.c		\
	gnome-metadata.c	\
	gnome-mime.c		\
	gnome-mime-info.c	\
	gnome-popt.c		\
	gnome-regex.c		\
	gnome-remote.c		\
	gnome-score.c		\
	gnome-sound.c		\
	gnome-triggers.c	\
	gnome-url.c		\
	gnome-util.c		\
	gnome-paper.c		\
	gnomelib-init.c

libgnomeinclude_HEADERS = \
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-dentry.h 		\
	gnome-exec.h		\
	gnome-help.h		\
	gnome-history.h		\
	gnome-i18n.h 		\
	gnome-magic.h		\
	gnome-metadata.h	\
	gnome-mime.h		\
	gnome-mime-info.h	\
	gnome-popt.h		\
	gnome-regex.h		\
	gnome-remote.h		\
	gnome-score.h		\
	gnome-sound.h		\
	gnome-triggers.h 	\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-paper.h		\
	libgnome.h

noinst_HEADERS =				\
	gnome-i18nP.h				\
	gnome-triggersP.h			\
	libgnomeP.h


EXTRA_DIST = parse-path.cP 

#just to be sure, add an explicit dependency
gnome-config.o: parse-path.cP

libgnome_la_LDFLAGS = -version-info 36:2:4 #-rpath $(libdir)
libgnome_la_LIBADD = $(GLIB_LIBS) -lm $(DL_LIB)

bin_PROGRAMS = dns-helper gnome-dump-metadata gnome-moz-remote \
	gconfigger gnome-gen-mimedb

dns_helper_SOURCES = dns-helper.c
dns_helper_LDADD = $(NSL_LIBS)

gnome_dump_metadata_SOURCES = gnome-dump.c
gnome_dump_metadata_LDADD = libgnome.la $(LIBSUPPORT) \
	$(LIBGNOME_LIBS) $(INTLLIBS)
gnome_dump_metadata_LDFLAGS = $(GLIB_LIBS)

gnome_moz_remote_SOURCES = gnome-moz-remote.c vroot.h
gnome_moz_remote_LDADD = libgnome.la $(LIBSUPPORT) $(LIBGNOME_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) $(INTLLIBS)

gconfigger_SOURCES = gconfigger.c
gconfigger_LDADD = libgnome.la $(LIBSUPPORT) \
	$(LIBGNOME_LIBS) $(INTLLIBS)

# go ahead and laugh, it's better than exporting internal routines
gnome_gen_mimedb_SOURCES = gnome-gen-mimedb.c
gnome_gen_mimedb_LDADD = libgnome.la $(LIBSUPPORT) \
	$(LIBGNOME_LIBS) $(INTLLIBS)
