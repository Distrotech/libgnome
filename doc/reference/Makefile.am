## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libgnome

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="GNOME_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libgnome

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libgnome/*.h
CFILE_GLOB=$(top_srcdir)/libgnome/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=			\
	gnome-gconfP.h		\
	gnome-i18nP.h		\
	gnome-preferences.h	\
	gnome-triggersP.h

INCLUDES = -I$(top_srcdir) -I$(top_builddir) $(LIBGNOME_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/libgnome/libgnome-2.la

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = version.xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Images to copy into HTML directory.
HTML_IMAGES =

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += version.xml.in
