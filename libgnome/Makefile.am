## Process this file with automake to produce Makefile.in
scoredir=$(localstatedir)/var/lib/games

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(libdir)/locale"\" \
	-I$(top_srcdir)/intl \
	-I$(srcdir)/../programs

lib_LTLIBRARIES = libgnome.la

libgnome_la_SOURCES = \
	gnome-hook.c 		\
	gnome-util.c		\
	gnome-config.c		\
	gnomelib-init.c         \
	gnome-dentry.c          \
	gnome-dns.c		\
	gnome-fileconvert.c	\
	gnome-history.c		\
	gnome-mime.c		\
	gnome-score.c		\
	gnome-string.c		\
	gnome-triggers.c

libgnomeinclude_HEADERS = \
        gnome-config.h 		\
	gnome-defs.h   		\
	gnome-fileconvert.h	\
	gnome-hook.h   		\
	gnome-util.h   		\
	gnome-dns.h    		\
	gnome-dentry.h 		\
	gnome-history.h		\
	gnome-i18n.h 		\
	gnome-mime.h		\
	gnome-triggers.h 	\
	gnome-score.h		\
	gnome-string.h		\
	libgnome.h


libgnome_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnome_la_LIBADD = -lgdk -lglib -lm

bin_PROGRAMS = gnome-score-helper

gnome_score_helper_SOURCES = gnome-score-helper.c
gnome_score_helper_LDADD = \
	$(top_builddir)/libgnome/libgnome.la \
	-lgdk -lglib $(INTLLIBS) $(X_LIBS) $(x_libs)

install-data-local:
	-chgrp games $(bindir)/gnome-score-helper
	-chmod g+s $(bindir)/gnome-score-helper
	-chgrp games $(scoredir)
	-chmod g=rwXs $(scoredir)
