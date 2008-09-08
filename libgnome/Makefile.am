## Process this file with automake to produce Makefile.in

#
# Targets
#

lib_LTLIBRARIES = libgnome-2.la

bin_PROGRAMS = gnome-open

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome-2.0/libgnome

INCLUDES = \
	-I..							\
	-I$(srcdir)/..						\
	$(WARN_CFLAGS)						\
	$(LIBGNOME_CFLAGS)					\
	-DG_DISABLE_DEPRECATED					\
	-DLIBGNOME_PREFIX=\""$(prefix)"\"			\
	-DLIBGNOME_LIBDIR=\""$(libdir)"\"			\
	-DLIBGNOME_DATADIR=\""$(datadir)"\"			\
	-DLIBGNOME_LOCALSTATEDIR=\""$(localstatedir)"\"		\
	-DLIBGNOME_SYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBGNOME_LOCALEDIR="\"$(datadir)/locale\""		\
	-DGNOMEVFSVERSION=\"$(GNOME_VFS_VERSION)\"		\
	-DGNOME_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE"	\
	-DG_LOG_DOMAIN=\"Gnome\"

if OS_WIN32
platform_sources = dllmain.c
endif

libgnome_2_la_SOURCES = \
	libgnometypebuiltins.h	\
	libgnometypebuiltins.c	\
	gnome-config.c		\
	gnome-sound.c		\
	gnome-triggers.c	\
	gnome-triggersP.h	\
	gnome-program.c		\
	gnome-help.c		\
	gnome-exec.c		\
	gnome-gconf.c		\
	gnome-gconfP.h		\
	gnome-i18n.c		\
	gnome-url.c		\
	gnome-util.c		\
	gnome-score.c		\
	gnome-init.c		\
	$(platform_sources)

noinst_HEADERS = \
	libgnome-private.h

libgnome_headers = \
	gnome-config.h		\
	gnome-sound.h		\
	gnome-triggers.h	\
	gnome-program.h		\
	gnome-help.h		\
	gnome-i18n.h		\
	gnome-exec.h		\
	gnome-gconf.h		\
	gnome-url.h		\
	gnome-util.h   		\
	gnome-macros.h		\
	gnome-score.h		\
	gnome-init.h		\
	libgnome.h

libgnomeinclude_HEADERS = \
	libgnometypebuiltins.h	\
	$(libgnome_headers)

gnome_open_SOURCES = \
	gnome-open.c

gnome_open_LDADD = \
	libgnome-2.la	\
	$(LIBGNOME_LIBS) \
	$(GIO_LIBS)

libgnometypebuiltins.h: stamp-libgnometypebuiltins.h
	@true
stamp-libgnometypebuiltins.h: Makefile @REBUILD@ $(libgnome_headers)
	(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __LIBGNOMETYPEBUILTINS_H__\n" \
			--fhead "#define __LIBGNOMETYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <libgnome/libgnome.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBGNOMETYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define GNOME_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(libgnome_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth libgnometypebuiltins.h || cp xgen-gth libgnometypebuiltins.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

libgnometypebuiltins.c: Makefile @REBUILD@ $(libgnome_headers)
	(cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include <config.h>\n" \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"libgnometypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(libgnome_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc

BUILT_SOURCES = libgnometypebuiltins.h libgnometypebuiltins.c

MAINTAINERCLEANFILES = stamp-libgnometypebuiltins.h \
  $(BUILT_SOURCES)
CLEANFILES = xgen-gth xgen-gtc

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

EXTRA_DIST = \
	libgnome-2.0.pc.in		\
	libgnome-2.0-uninstalled.pc.in	\
	parse-path.cP

libgnome_2_la_LDFLAGS = \
	-version-info $(LIBGNOME_CURRENT):$(LIBGNOME_REVISION):$(LIBGNOME_AGE) \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

libgnome_2_la_LIBADD =		\
	$(LIBGNOME_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnome-2.0.pc

