/*
 * gnome-moniker-extender-file.c: 
 *
 * Author:
 *	Dietmar Maurer (dietmar@maurer-it.com)
 */
#include <config.h>
#include <stdlib.h>
  
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-init.h>
#include <gtk/gtk.h>

#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-moniker.h>
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-moniker-extender.h>
#include <libgnome/gnome-mime.h>
#include <liboaf/liboaf.h>

static int running_objects = 0;
static BonoboGenericFactory *extender_factory = NULL;

static Bonobo_Unknown
file_extender_resolve (BonoboMonikerExtender *extender,
		       const Bonobo_Moniker   m,
		       const CORBA_char      *display_name,
		       const CORBA_char      *requested_interface,
		       CORBA_Environment     *ev)
{
	/* FIXME: we need to get some nice infastructure here first. */
#if 0
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
#endif

	return CORBA_OBJECT_NIL;
}


static void
extender_destroy_cb (BonoboMonikerExtender *extender, gpointer dummy)
{
	running_objects--;

	if (running_objects > 0)
		return;

	bonobo_object_unref (BONOBO_OBJECT (extender_factory));
	gtk_main_quit ();
}

static BonoboObject *
file_extender_factory (BonoboGenericFactory *this,
		       void                 *data)
{
	BonoboMonikerExtender *extender = NULL;
	BonoboObject  *object  = NULL;
	
	extender = bonobo_moniker_extender_new (file_extender_resolve, NULL);

	object = BONOBO_OBJECT (extender);
	
	if (object) {
		running_objects++;
		
		gtk_signal_connect (GTK_OBJECT (extender), "destroy",
				    GTK_SIGNAL_FUNC (extender_destroy_cb), NULL);
	}

	return object;
}

int
main (int argc, char *argv [])
{
	CORBA_Environment ev;
	CORBA_ORB orb = CORBA_OBJECT_NIL;
	char *dummy;

	dummy = malloc (8); if (dummy) free (dummy);

	CORBA_exception_init (&ev);

        gnome_init_with_popt_table ("file-moniker-extender", "0.0", argc, argv,
				    oaf_popt_options, 0, NULL); 
	orb = oaf_init (argc, argv);

	if (bonobo_init (orb, CORBA_OBJECT_NIL, CORBA_OBJECT_NIL) == FALSE)
		g_error (_("I could not initialize Bonobo"));

	extender_factory = bonobo_generic_factory_new (
		"OAFIID:Bonobo_MonikerExtender_fileFactory",
		file_extender_factory, NULL);	

	bonobo_main ();

	printf ("EXIT file extender\n");

	CORBA_exception_free (&ev);

	return 0;
}
