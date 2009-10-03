INCLUDES = \
	-I$(top_srcdir) 				\
	$(LIBGNOME_CFLAGS)				\
	-DG_LOG_DOMAIN=\"libgnome\"			\
	-DVERSION=\"$(VERSION)\"

GNOME_Moniker_std.server.in: GNOME_Moniker_std.server.in.in
	$(AM_V_GEN) $(SED) -e "s|\@MONIKER_LIBDIR\@|$(monikerdir_in_server_file)|" \
		-e "s|\@SOPREFIX\@|$(SOPREFIX)|" $< > $@

CLEANFILES = GNOME_Moniker_std.server GNOME_Moniker_std.server.in

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_Moniker_std.server

@INTLTOOL_SERVER_RULE@

monikerdir = $(libdir)/bonobo/monikers

if OS_WIN32
monikerdir_in_server_file = ../monikers
else
monikerdir_in_server_file = $(monikerdir)
endif

moniker_LTLIBRARIES = libmoniker_extra_2.la

libmoniker_extra_2_la_SOURCES =			\
	bonobo-config-bag.c			\
	bonobo-config-bag.h			\
	bonobo-moniker-config.c			\
	bonobo-moniker-conf-indirect.c		\
	bonobo-moniker-extra.h            	\
	bonobo-moniker-extra.c

libmoniker_extra_2_la_LDFLAGS = -module -avoid-version -no-undefined

libmoniker_extra_2_la_LIBADD = $(LIBGNOME_LIBS)

EXTRA_DIST = GNOME_Moniker_std.server.in.in
