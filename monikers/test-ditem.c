/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
#include <libbonobo.h>
#include <libgnome/Gnome.h>
#include <locale.h>
#include <stdlib.h>

#include "bonobo-config-ditem.h"
#include "Foo.h"

#define USE_BUILTIN_MONIKER 1

static void G_GNUC_UNUSED
boot_ditem (Bonobo_ConfigDatabase db)
{
	BonoboArg *arg;
	CORBA_Environment ev;

	arg = bonobo_arg_new (TC_Foo_Desktop_Entry);
	bonobo_pbclient_set_value (db, "/Desktop Entry", arg, NULL);
	bonobo_arg_release (arg);

	arg = bonobo_arg_new (TC_Foo_Test_Simple);
	bonobo_pbclient_set_value (db, "/Simple", arg, NULL);
	bonobo_arg_release (arg);

	CORBA_exception_init (&ev);
	Bonobo_ConfigDatabase_sync (db, &ev);
	CORBA_exception_free (&ev);
}

#ifdef USE_BUILTIN_MONIKER
static void
test_builtin (void)
{
	Bonobo_ConfigDatabase db, parent_db;
	CORBA_Environment ev;
	CORBA_TypeCode type;
	BonoboArg *value;
	gchar *string;

	CORBA_exception_init (&ev);
	parent_db = bonobo_get_object ("xmldb:" MONIKER_SRCDIR "/foo.xml", "Bonobo/ConfigDatabase", &ev);
	g_assert (!BONOBO_EX (&ev) && parent_db != CORBA_OBJECT_NIL);
	CORBA_exception_free (&ev);

	db = bonobo_config_ditem_new (MONIKER_SRCDIR "/foo.desktop", parent_db, &ev);
	g_assert (!BONOBO_EX (&ev) && db != CORBA_OBJECT_NIL);
}
#endif

static void
print_simple (Foo_Test_Simple *simple)
{
	g_message (G_STRLOC ": %p => %u - %lu", simple,
		   (unsigned) simple->a, (unsigned long) simple->b);
}

static void
print_ditem (Foo_Desktop_Entry *entry)
{
	g_message (G_STRLOC ": %p", entry);

	g_message (G_STRLOC ": |%s| - |%s| - |%s|", entry->Exec, entry->Icon, entry->URL);
}

int
main (int argc, char **argv)
{
	Bonobo_ConfigDatabase db = NULL;
	Bonobo_ConfigDatabase default_db = NULL;
	Bonobo_KeyList *dirlist, *keylist;
        CORBA_Environment  ev;
	CORBA_TypeCode type;
	CORBA_any *value;
	gchar *string;
	guint i, j;

        CORBA_exception_init (&ev);

	setlocale (LC_ALL, "");

	if (bonobo_init (&argc, argv) == FALSE)
		g_error ("Cannot init bonobo");

	bonobo_activate ();

        CORBA_exception_init (&ev);
	default_db = bonobo_get_object ("xmldb:" MONIKER_SRCDIR "/foo.xml", "Bonobo/ConfigDatabase", &ev);
	g_assert (!BONOBO_EX (&ev));
        CORBA_exception_free (&ev);

#ifdef USE_BUILTIN_MONIKER
	test_builtin ();

        CORBA_exception_init (&ev);
	db = bonobo_config_ditem_new (MONIKER_SRCDIR "/foo.desktop", default_db, &ev);
	g_assert (!BONOBO_EX (&ev));
#else
        CORBA_exception_init (&ev);
	db = bonobo_get_object ("ditem:" MONIKER_SRCDIR "/foo.desktop", "Bonobo/ConfigDatabase", &ev);
	g_assert (!BONOBO_EX (&ev));
        CORBA_exception_free (&ev);
#endif

	g_assert (db != NULL);
	g_assert (default_db != NULL);

        CORBA_exception_init (&ev);
	Bonobo_ConfigDatabase_addDatabase (db, default_db, "/gnome-ditem/",
					   Bonobo_ConfigDatabase_DEFAULT, &ev);
	g_assert (!BONOBO_EX (&ev));

	boot_ditem (default_db);

	dirlist = Bonobo_ConfigDatabase_getDirs (db, "", &ev);
	g_assert (!BONOBO_EX (&ev));

	if (dirlist) {
		for (i = 0; i < dirlist->_length; i++) {
			g_print ("DIR: |%s|\n", dirlist->_buffer [i]);

			keylist = Bonobo_ConfigDatabase_getKeys (db, dirlist->_buffer [i], &ev);
			g_assert (!BONOBO_EX (&ev));

			if (keylist)
				for (j = 0; j < keylist->_length; j++)
					g_print ("KEY (%s): |%s|\n", dirlist->_buffer [i],
						 keylist->_buffer [j]);
	    }
	}

	keylist = Bonobo_ConfigDatabase_getKeys (db, "/Config/Foo", &ev);
	g_assert (!BONOBO_EX (&ev));

	if (keylist)
		for (j = 0; j < keylist->_length; j++)
			g_print ("TEST KEY: |%s|\n", keylist->_buffer [j]);

        CORBA_exception_init (&ev);
	type = bonobo_pbclient_get_type (db, "/Foo/Test", &ev);
	if (type)
		printf ("type is %d - %s (%s)\n", type->kind, type->name, type->repo_id);

        CORBA_exception_init (&ev);
	value = bonobo_pbclient_get_value (db, "/Foo/Test", TC_CORBA_long, &ev);
	if (value) {
		printf ("got value as long %d\n", BONOBO_ARG_GET_LONG (value));	
		BONOBO_ARG_SET_LONG (value, 512);
		bonobo_pbclient_set_value (db, "/Foo/Test", value, &ev);
	}
	bonobo_arg_release (value);
        CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	type = bonobo_pbclient_get_type (db, "/Foo/Test", &ev);
	if (type)
		g_message (G_STRLOC ": type is %d - %s (%s)", type->kind, type->name, type->repo_id);
	CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	value = bonobo_pbclient_get_value (db, "/Foo/Test", TC_CORBA_long, &ev);
	if (value) {
		g_message (G_STRLOC ": got value as long %d", BONOBO_ARG_GET_LONG (value));
		BONOBO_ARG_SET_LONG (value, 512);
		bonobo_pbclient_set_value (db, "/Foo/Test", value, &ev);
	}
	bonobo_arg_release (value);
        CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	type = bonobo_pbclient_get_type (db, "/Desktop Entry/Terminal", &ev);
	if (type)
		g_message (G_STRLOC ": type is %d - %s (%s)", type->kind, type->name, type->repo_id);
	CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	type = bonobo_pbclient_get_type (db, "/Desktop Entry/Name", &ev);
	if (type)
		g_message (G_STRLOC ": type is %d - %s (%s)", type->kind, type->name, type->repo_id);
	CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	type = bonobo_pbclient_get_type (db, "/Desktop Entry/URL", &ev);
	if (type)
		g_message (G_STRLOC ": type is %d - %s (%s)", type->kind, type->name, type->repo_id);
	CORBA_exception_free (&ev);

	string = bonobo_pbclient_get_string (db, "/Desktop Entry/URL", NULL);
	g_message (G_STRLOC ": |%s|", string);
	bonobo_pbclient_set_string (db, "/Desktop Entry/URL", "http://www.gnome.org/", NULL);

        CORBA_exception_init (&ev);
	value = bonobo_pbclient_get_value (db, "/Simple", TC_Foo_Test_Simple, &ev);
	g_message (G_STRLOC ": %p", value);
	if (value) {
		Foo_Test_Simple simple;

		printf ("got value as Foo::Test::Simple\n");

		simple = BONOBO_ARG_GET_GENERAL (value, TC_Foo_Test_Simple, Foo_Test_Simple, NULL);

		print_simple (&simple);
	}

        CORBA_exception_free (&ev);

	CORBA_exception_init (&ev);
	Bonobo_ConfigDatabase_sync (db, &ev);
	g_assert (!BONOBO_EX (&ev));
        CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	value = bonobo_pbclient_get_value (db, "/Simple", TC_Foo_Test_Simple, &ev);
	g_message (G_STRLOC ": %p", value);
	if (value) {
		Foo_Test_Simple simple;

		printf ("got value as Foo::Test::Simple\n");

		simple = BONOBO_ARG_GET_GENERAL (value, TC_Foo_Test_Simple, Foo_Test_Simple, NULL);

		print_simple (&simple);
	}

        CORBA_exception_free (&ev);

        CORBA_exception_init (&ev);
	value = bonobo_pbclient_get_value (db, "/Desktop Entry", TC_Foo_Desktop_Entry, &ev);
	g_message (G_STRLOC ": %p", value);
	if (value) {
		Foo_Desktop_Entry entry;

		printf ("got value as Foo::Desktop::Entry\n");

		entry = BONOBO_ARG_GET_GENERAL (value, TC_Foo_Desktop_Entry, Foo_Desktop_Entry, NULL);

		print_ditem (&entry);
	}

        CORBA_exception_free (&ev);

	exit (0);
}
