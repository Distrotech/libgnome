## Process this file with automake to produce Makefile.in

#
# Targets
#
lib_LTLIBRARIES = libgnome.la

scoredir=$(localstatedir)/games

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(SUPPORTINCS) \
        $(GTK_CFLAGS)  \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
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
	gnome-mime.c		\
	gnome-parse.c		\
	gnome-score.c		\
	gnome-string.c		\
	gnome-triggers.c	\
	gnome-util.c		\
	gnomelib-init.c         \
	lib_date.c		\
	lib_date.h		\
	lib_defs.h

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
	gnome-mime.h		\
	gnome-parse.h		\
	gnome-score.h		\
	gnome-string.h		\
	gnome-triggers.h 	\
	gnome-util.h   		\
	libgnome.h

noinst_HEADERS = \
	gnome-i18nP.h		\
	libgnomeP.h

EXTRA_DIST = lib_date.README

libgnome_la_LDFLAGS = -version-info 26:0:26 -rpath $(libdir)
libgnome_la_LIBADD = $(GLIB_LIBS) -lm $(DL_LIB)

bin_PROGRAMS = dns-helper

dns_helper_SOURCES = dns-helper.c
dns_helper_LDADD = $(NSL_LIBS)

install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp games $(scoredir)
	-chmod g=rwXs $(scoredir)
