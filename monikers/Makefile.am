INCLUDES = \
	-I$(top_srcdir) 				\
	$(BONOBOUI_CFLAGS)				\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
	-DG_LOG_DOMAIN=\"Bonoboui\"			\
	-DVERSION=\"$(VERSION)\"

Bonobo_Moniker_extra.server.in: Bonobo_Moniker_extra.server.in.in
	sed -e "s|\@MONIKER_LIBDIR\@|$(monikerdir)|" $< > $@

CLEANFILES+=Bonobo_Moniker_extra.server Bonobo_Moniker_extra.server.in

serverdir = $(libdir)/bonobo/servers
server_DATA = Bonobo_Moniker_extra.server

@INTLTOOL_SERVER_RULE@

monikerdir = $(libdir)/bonobo/monikers
moniker_LTLIBRARIES = libmoniker_extra_2.la

libmoniker_extra_2_la_SOURCES =			\
	bonobo-config-bag.c			\
	bonobo-config-bag.h			\
	bonobo-moniker-config.c			\
	bonobo-moniker-conf-indirect.c		\
	bonobo-moniker-extra.h            	\
	bonobo-moniker-extra.c			

libmoniker_extra_2_la_LDFLAGS = -module -avoid-version $(BONOBOUI_LIBS)

EXTRA_DIST = Bonobo_Moniker_extra.server.in.in
