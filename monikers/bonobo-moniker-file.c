/*
 * gnome-moniker-file.c: Sample file-system based Moniker implementation
 *
 * This is the file-system based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 */
#include <config.h>
#include <bonobo/bonobo-moniker.h>
#include <bonobo/bonobo-moniker-util.h>
#include <libgnome/gnome-mime.h>
#include <liboaf/liboaf.h>

#include "bonobo-moniker-file.h"

#define PREFIX_LEN (sizeof ("file:") - 1)

static BonoboMonikerClass *bonobo_moniker_file_parent_class;

static Bonobo_Moniker 
file_parse_display_name (BonoboMoniker     *moniker,
			 Bonobo_Moniker     parent,
			 const CORBA_char  *name,
			 CORBA_Environment *ev)
{
	int i;
	BonoboMonikerFile *m_file = BONOBO_MONIKER_FILE (moniker);

	g_return_val_if_fail (m_file != NULL, CORBA_OBJECT_NIL);
	g_return_val_if_fail (strlen (name) >= PREFIX_LEN, CORBA_OBJECT_NIL);

	bonobo_moniker_set_parent (moniker, parent, ev);

	i = bonobo_moniker_util_seek_std_separator (name, 0);

	bonobo_moniker_set_name (moniker, name, i);

	if (name [i])
		return bonobo_moniker_util_new_from_name_full (
			bonobo_object_corba_objref (BONOBO_OBJECT (m_file)),
			&name [i], ev);
	else
		return bonobo_object_dup_ref (
			bonobo_object_corba_objref (BONOBO_OBJECT (m_file)), ev);
}

static Bonobo_Unknown
file_resolve (BonoboMoniker               *moniker,
	      const Bonobo_ResolveOptions *options,
	      const CORBA_char            *requested_interface,
	      CORBA_Environment           *ev)
{
	const char *fname = bonobo_moniker_get_name (moniker, PREFIX_LEN);

	g_warning ("Fname '%s'", fname);

	if (!strcmp (requested_interface, "IDL:Bonobo/Stream:1.0")) {
		BonoboStream *stream;
		
		stream = bonobo_stream_open ("fs", fname,
					     Bonobo_Storage_READ, 0664);

		if (!stream) {
			g_warning ("Failed to open stream '%s'", fname);
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Moniker_InterfaceNotFound, NULL);
			return CORBA_OBJECT_NIL;
		}

		return CORBA_Object_duplicate (
			bonobo_object_corba_objref (BONOBO_OBJECT (stream)), ev);
	} else {
		const char    *mime_type;
		char          *oaf_requirements;
		Bonobo_Unknown object;
		Bonobo_Persist persist;
		OAF_ActivationID ret_id;

		mime_type = gnome_mime_type (fname);

		oaf_requirements = g_strdup_printf (
			"bonobo:supported_mime_types.has ('%s') AND repo_ids.has ('%s') AND "
			"repo_ids.has_one (['IDL:Bonobo/PersistFile:1.0', 'IDL:Bonobo/PersistStream:1.0'])",
			mime_type, requested_interface);
		
		object = oaf_activate (oaf_requirements, NULL, 0, &ret_id, ev);
		g_warning ("Attempt activate object satisfying '%s': %p",
			   oaf_requirements, object);
		g_free (oaf_requirements);

		if (ev->_major != CORBA_NO_EXCEPTION)
			return CORBA_OBJECT_NIL;
		
		if (object == CORBA_OBJECT_NIL) {
			g_warning ("Can't find object satisfying requirements");
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Moniker_InterfaceNotFound, NULL);
			return CORBA_OBJECT_NIL;
		}

		persist = Bonobo_Unknown_queryInterface (
			object, "IDL:Bonobo/PersistFile:1.0", ev);

		if (ev->_major != CORBA_NO_EXCEPTION)
			return CORBA_OBJECT_NIL;

		if (persist != CORBA_OBJECT_NIL) {
			Bonobo_PersistFile_load (persist, fname, ev);

			bonobo_object_release_unref (persist, ev);

			return bonobo_moniker_util_qi_return (
				object, requested_interface, ev);
		}
		
		persist = Bonobo_Unknown_queryInterface (
			object, "IDL:Bonobo/PersistStream:1.0", ev);

		if (ev->_major != CORBA_NO_EXCEPTION)
			goto unref_object_exception;

		if (persist != CORBA_OBJECT_NIL) {
			BonoboStream *stream;

			stream = bonobo_stream_open ("fs", fname, 
						     Bonobo_Storage_READ, 
						     0664);

			if (!stream) {
				g_warning ("Failed to open stream '%s'", fname);
				CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
						     ex_Bonobo_Moniker_InterfaceNotFound, NULL);
				return CORBA_OBJECT_NIL;
			}

			Bonobo_PersistStream_load (
				persist, 
				bonobo_object_corba_objref (BONOBO_OBJECT (stream)),
				(const Bonobo_Persist_ContentType)mime_type, ev);

			bonobo_object_release_unref (persist, ev);

			return bonobo_moniker_util_qi_return (
				object, requested_interface, ev);
		}

		/* FIXME: so perhaps here we need to start doing in-file storages etc. */
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_Bonobo_Moniker_InterfaceNotFound, NULL);

	unref_object_exception:
		bonobo_object_release_unref (object, ev);
		return CORBA_OBJECT_NIL;
	}
}

static void
bonobo_moniker_file_class_init (BonoboMonikerFileClass *klass)
{
	BonoboMonikerClass *mclass = (BonoboMonikerClass *) klass;
	
	bonobo_moniker_file_parent_class = gtk_type_class (
		bonobo_moniker_get_type ());

	mclass->parse_display_name = file_parse_display_name;
	mclass->resolve            = file_resolve;
}

/**
 * bonobo_moniker_file_get_type:
 *
 * Returns the GtkType for the BonoboMonikerFile class.
 */
GtkType
bonobo_moniker_file_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboMonikerFile",
			sizeof (BonoboMonikerFile),
			sizeof (BonoboMonikerFileClass),
			(GtkClassInitFunc) bonobo_moniker_file_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_moniker_get_type (), &info);
	}

	return type;
}
