INCLUDES = \
	-I..					\
	-I$(srcdir)/..				\
	$(WARN_CFLAGS)				\
	$(MONIKER_CFLAGS)			\
	-Wno-unused				\
	-DLIBGNOME_PREFIX=\""$(prefix)"\"		\
	-DLIBGNOME_LIBDIR=\""$(libdir)"\"		\
	-DLIBGNOME_DATADIR=\""$(datadir)"\"		\
	-DLIBGNOME_BINDIR=\""$(bindir)"\"		\
	-DLIBGNOME_LOCALSTATEDIR=\""$(localstatedir)"\" \
	-DLIBGNOME_LOCALEDIR=\""$(gnomelocaledir)"\"	\
	-DLIBGNOME_SYSCONFDIR=\""$(sysconfdir)"\"	\
	-DVERSION=\""$(VERSION)"\"			\
	-DG_LOG_DOMAIN=\"GnomeMonikers\"

EXTRA_DIST = \
	Bonobo_Moniker_ditem.oaf.in \
	GNOME_Moniker_std.oaf.in.in

moniker_LTLIBRARIES = libmoniker_ditem.la libmoniker_gnome_std.la
monikerdir = $(libdir)/bonobo/monikers


# File monikers

GNOME_Moniker_std.oaf.in : $(srcdir)/GNOME_Moniker_std.oaf.in.in
	sed -e "s|\@MONIKER_LIBDIR\@|$(monikerdir)|" \
	 $(srcdir)/GNOME_Moniker_std.oaf.in.in > GNOME_Moniker_std.oaf.in

CLEANFILES+=GNOME_Moniker_std.oaf GNOME_Moniker_std.oaf.in

oafdir = $(datadir)/oaf
oaf_DATA = \
	Bonobo_Moniker_ditem.oaf \
	GNOME_Moniker_std.oaf

@XML_I18N_MERGE_OAF_RULE@

clean-local:
	-rm -f $(oaf_DATA)

libmoniker_gnome_std_la_SOURCES = \
	gnome-moniker-std.c		\
	gnome-moniker-std.h		\
	bonobo-stream-fs.h		\
	bonobo-stream-fs.c		\
	bonobo-storage-fs.h		\
	bonobo-storage-fs.c		\
	bonobo-moniker-extender-file.c	\
	bonobo-moniker-file.c		\
	bonobo-stream-vfs.h		\
	bonobo-stream-vfs.c		\
	bonobo-storage-vfs.h		\
	bonobo-storage-vfs.c		\
	bonobo-moniker-vfs.c

libmoniker_gnome_std_la_LIBADD = \
	../libgnome/libgnome-2.la \
	$(MONIKER_LIBS)


# Desktop Item specific bits...

# Data
gnomesysconfdir = $(sysconfdir)/gnome-2.0
gnomesysconf_DATA = gnome-desktop.xmldb

# DItem moniker

Bonobo_Moniker_ditem.oaf : $(srcdir)/Bonobo_Moniker_ditem.oaf.in $(top_builddir)/config.status
	sed -e "s|\@MONIKER_LIBDIR\@|$(monikerdir)|" \
	$(srcdir)/Bonobo_Moniker_ditem.oaf.in > Bonobo_Moniker_ditem.oaf 

libmoniker_ditem_la_SOURCES =	 		\
	bonobo-moniker-ditem.c 			\
	bonobo-config-ditem.c			\
	bonobo-config-ditem.h			\
	bonobo-config-ditem-utils.c		\
	bonobo-config-ditem-utils.h

libmoniker_ditem_la_LIBADD = \
	../libgnome/libgnome-2.la		\
	$(MONIKER_LIBS)

# DItem tests ...

noinst_PROGRAMS = test-ditem desktop-item-boot

test_ditem_SOURCES = \
	test-ditem.c

test_ditem_LDADD = \
	../libgnome/libgnome-2.la		\
	$(MONIKER_LIBS)

desktop_item_boot_SOURCES = \
	desktop-item-boot.c

desktop_item_boot_LDADD = \
	../libgnome/libgnome-2.la
