soundlistdir = $(sysconfdir)/sound/events

soundlist_in_files = \
	gnome-2.soundlist.in		\
	gtk-events-2.soundlist.in

soundlist_DATA = $(soundlist_in_files:.soundlist.in=.soundlist)

man_MANS = gnome-options.7

backgrounddir = $(datadir)/pixmaps/backgrounds/gnome

background_DATA = \
	background-default.jpg

metadata_in_files = gnome-default.xml.in
metadatadir = $(datadir)/gnome-background-properties
metadata_DATA = $(metadata_in_files:.xml.in=.xml)

gnome-default.xml.in: gnome-default.xml.in.in Makefile
	sed -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/gnome-default.xml.in.in > gnome-default.xml.in.tmp \
	&& mv gnome-default.xml.in.tmp gnome-default.xml.in

@INTLTOOL_SOUNDLIST_RULE@
@INTLTOOL_XML_RULE@

noinst_DATA = gnome-default.xml.in
CLEANFILES = $(soundlist_DATA) gnome-default.xml gnome-default.xml.in

EXTRA_DIST = $(soundlist_in_files) $(soundlist_DATA) $(man_MANS) $(background_DATA) gnome-default.xml.in.in
