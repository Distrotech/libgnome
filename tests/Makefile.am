noinst_PROGRAMS = \
	test-storage

INCLUDES = \
	-I$(srcdir) -I$(top_srcdir) \
	-I. $(LIBGNOME_CFLAGS)

LDADD =	$(LIBGNOME_LIBS)

test_storage_SOURCES = test-storage.c

TESTS_ENVIRONMENT =					\
	LD_LIBRARY_PATH=$(top_builddir)/libgnome/.libs/	\
	BONOBO_PLUGIN_PATH=$(top_builddir)/storage-modules/.libs
