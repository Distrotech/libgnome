noinst_PROGRAMS =		\
	test-moniker		\
	test-properties-server	\
	test-properties-client	\
	test-storage		\
	test-event-source

INCLUDES =	\
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(srcdir) -I$(top_srcdir) 			\
	-I. -I$(top_srcdir)/bonobo			\
        -I$(includedir)                                 \
	$(WARN_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(ORBIT_CFLAGS)					\
	$(OAF_CFLAGS)					\
	$(LIBGNOMEBASE_CFLAGS)

LDADD =	$(top_builddir)/bonobo/libbonobo-2.la

test_properties_server_SOURCES =			\
	test-properties-server.c

test_properties_client_SOURCES =			\
	test-properties-client.c

test_storage_SOURCES = test-storage.c

test_moniker_SOURCES = test-moniker.c

test_event_source_SOURCES = test-event-source.c

TESTS_ENVIRONMENT =					\
	LD_LIBRARY_PATH=$(top_builddir)/bonobo/.libs/	\
	BONOBO_PLUGIN_PATH=$(top_builddir)/storage-modules/.libs

TESTS = test-storage test-any test-moniker test-event-source
