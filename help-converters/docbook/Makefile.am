SUBDIRS= docbook gnome-customization

INCLUDES = \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DPREFIXDIR=\""$(prefix)"\" 			\
	$(LIBGNOME_CFLAGS)				\
	$(XSLT_CFLAGS)				

bin_PROGRAMS = gnome2-db2html

gnome2_db2html_SOURCES = \
	gnome-db2html3.c

gnome2_db2html_LDADD = \
	$(LIBGNOME_LIBS)				\
	$(XSLT_LIBS)