@INTLTOOL_SOUNDLIST_RULE@

soundlist_in_files = \
		gnome-2.soundlist.in		\
		gtk-events-2.soundlist.in


soundlist_files=$(soundlist_in_files:.soundlist.in=.soundlist)

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = \
		gnome-2.soundlist 		\
		gtk-events-2.soundlist

EXTRA_DIST = \
		$(soundlist_DATA)		\
		$(soundlist_in_files)


