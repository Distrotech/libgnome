## Process this file with automake to produce Makefile.in

#
# Targets
#

GNOME_MAKEENUMS = $(top_srcdir)/tools/gnome-makeenums.pl
GNOME_MAKETYPES = $(top_builddir)/tools/gnome-maketypes.awk

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

lib_LTLIBRARIES = libgnome-2.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/gnome/$(GNOME_INTERFACE_VERSION)/libgnome

INCLUDES = \
	-I..					\
	-I$(srcdir)/..				\
	$(WARN_CFLAGS)				\
	$(GLIB_CFLAGS)				\
	$(LIBBONOBO_CFLAGS)			\
	$(OAF_CFLAGS)				\
	$(GCONF_CFLAGS)				\
	$(LIBXML_CFLAGS)			\
	$(GNOME_VFS_CFLAGS)			\
	-DLIBGNOME_PREFIX=\""$(prefix)"\"		\
	-DLIBGNOME_LIBDIR=\""$(libdir)"\"		\
	-DLIBGNOME_DATADIR=\""$(datadir)"\"		\
	-DLIBGNOME_BINDIR=\""$(bindir)"\"		\
	-DLIBGNOME_LOCALSTATEDIR=\""$(localstatedir)"\" \
	-DLIBGNOME_LOCALEDIR=\""$(gnomelocaledir)"\"	\
	-DLIBGNOME_SYSCONFDIR=\""$(sysconfdir)"\"	\
	-DVERSION=\""$(VERSION)"\"			\
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
	gnome-preferences.c	\
	libgnome-init.c		\
	libgnometypes.c

libgnome_headers = \
	gnome-program.h		\
	gnome-i18n.h		\
        gnome-config.h 		\
	gnome-fileconvert.h	\
	gnome-ditem.h		\
	gnome-exec.h		\
	gnome-regex.h		\
	gnome-remote.h		\
	gnome-score.h		\
	gnome-triggers.h 	\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-paper.h		\
	gnome-preferences.h	\
	libgnome-init.h		\
	libgnome.h

libgnomeinclude_HEADERS = \
	libgnometypebuiltins.h	\
	$(libgnome_headers)

noinst_HEADERS = \
	gnome-triggersP.h			\
	libgnomeP.h

# generate libgnome.defs file from libgnome-boxed.defs and *.h
libgnome.defs: $(GNOME_MAKEENUMS) libgnome-boxed.defs $(libgnome_headers)
	cd $(srcdir) \
	&& $(GNOME_MAKEENUMS) defs $(libgnome_headers) > gd.tmp \
	&& cat libgnome-boxed.defs >> gd.tmp \
	&& mv gd.tmp libgnome.defs

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
libgnometypebuiltins.h: libgnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnome.defs macros > gtb.tmp \
	&& mv gtb.tmp libgnometypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
libgnometypebuiltins_vars.c: libgnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnome.defs variables > gtbv.tmp \
	&& mv gtbv.tmp libgnometypebuiltins_vars.c

# generate type entries for type-id registration
libgnometypebuiltins_ids.c: libgnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) libgnome.defs entries > gtbi.tmp \
	&& mv gtbi.tmp libgnometypebuiltins_ids.c

# generate enum value arrays
libgnometypebuiltins_evals.c: $(GNOME_MAKEENUMS) libgnome.defs
	cd $(srcdir) \
	&& $(GNOME_MAKEENUMS) arrays $(libgnome_headers) > gtbe.tmp \
	&& mv gtbe.tmp libgnometypebuiltins_evals.c

EXTRA_DIST = \
	parse-path.cP			\
	libgnome-2.0.pc.in

BUILT_SOURCES = \
	libgnometypebuiltins.h		\
	libgnometypebuiltins_evals.c	\
	libgnometypebuiltins_ids.c	\
	libgnometypebuiltins_vars.c

#just to be sure, add an explicit dependency
gnome-config.o: parse-path.cP

libgnome_2_la_LDFLAGS = -version-info $(LIB_VERSION)
libgnome_2_la_LIBADD =		\
	$(OAF_LIBS)		\
	$(GCONF_LIBS)		\
	$(LIBXML_LIBS)		\
	$(GNOME_VFS_LIBS)	\
	$(LIBBONOBO_LIBS)	\
	$(GLIB_LIBS)

noinst_PROGRAMS = \
	test-libgnome

test_libgnome_SOURCES = test-libgnome.c
test_libgnome_LDADD = \
	libgnome-2.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnome-2.0.pc

