## Process this file with automake to produce Makefile.in
scoredir=$(localstatedir)/games

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @SUPPORTINCS@ \
        @GTK_CFLAGS@  \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/support

lib_LTLIBRARIES = libgnome.la

libgnome_la_SOURCES = \
	gnome-hook.c 		\
	gnome-util.c		\
	gnome-config.c		\
	gnomelib-init.c         \
	gnome-dentry.c          \
	gnome-dl.c		\
	gnome-exec.c		\
	gnome-fileconvert.c	\
	gnome-help.c		\
	gnome-history.c		\
	gnome-i18n.c		\
	gnome-mime.c		\
	gnome-parse.c		\
	gnome-score.c		\
	gnome-string.c		\
	gnome-triggers.c	\
	lib_date.c		\
	lib_date.h		\
	lib_defs.h

libgnomeinclude_HEADERS = \
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-hook.h   		\
	gnome-util.h   		\
	gnome-dl.h		\
	gnome-dentry.h 		\
	gnome-exec.h		\
	gnome-help.h		\
	gnome-history.h		\
	gnome-i18n.h 		\
	gnome-mime.h		\
	gnome-parse.h		\
	gnome-triggers.h 	\
	gnome-score.h		\
	gnome-string.h		\
	libgnome.h

noinst_HEADERS = \
	gnome-i18nP.h

EXTRA_DIST = lib_date.README

libgnome_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnome_la_LIBADD = -lglib -lm $(DL_LIB)

bin_PROGRAMS = dns-helper

dns_helper_SOURCES = dns-helper.c
dns_helper_LDADD = $(NSL_LIBS)

install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp games $(scoredir)
	-chmod g=rwXs $(scoredir)
