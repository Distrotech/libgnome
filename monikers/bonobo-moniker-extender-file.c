/*
 * gnome-moniker-extender-file.c: 
 *
 * Author:
 *	Dietmar Maurer (dietmar@maurer-it.com)
 *
 * Copyright 2000, Helix Code, Inc.
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
#include <bonobo/bonobo-stream.h>
#include <bonobo/bonobo-exception.h>
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
		       const Bonobo_ResolveOptions *options,
		       const CORBA_char      *display_name,
		       const CORBA_char      *requested_interface,
		       CORBA_Environment     *ev)
{
	const char      *mime_type;
	char            *oaf_requirements;
	Bonobo_Unknown   object;
	Bonobo_Persist   persist;
	OAF_ActivationID ret_id;
	const char      *fname;

	if (strchr (display_name, ':'))
		fname = strchr (display_name, ':');
	else
		fname = display_name;

	g_warning ("Filename : '%s'", fname);

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
		bonobo_object_unref (BONOBO_OBJECT (stream));

		return bonobo_moniker_util_qi_return (
			object, requested_interface, ev);
	}

 unref_object_exception:
	bonobo_object_release_unref (object, ev);

	return CORBA_OBJECT_NIL;
}


static void
extender_destroy_cb (BonoboMonikerExtender *extender, gpointer dummy)
{
	BonoboObject *factory = BONOBO_OBJECT (extender_factory);
	gpointer      impl_ptr;

	running_objects--;

	if (running_objects > 0)
		return;

	extender_factory = NULL;

	if (factory) {
		impl_ptr = gtk_object_get_data (
			GTK_OBJECT (factory), "OAF_IMPL_PTR");

		bonobo_object_unref (factory);

		oaf_plugin_unuse (impl_ptr);
	}
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

static CORBA_Object
make_file_extender_factory (PortableServer_POA poa,
			    const char        *iid,
			    gpointer           impl_ptr,
			    CORBA_Environment *ev)
{
        CORBA_Object object_ref;

        oaf_plugin_use (poa, impl_ptr);

	extender_factory = bonobo_generic_factory_new (
		"OAFIID:Bonobo_MonikerExtender_fileFactory",
		file_extender_factory, NULL);	

	gtk_object_set_data (GTK_OBJECT (extender_factory), 
			     "OAF_IMPL_PTR", 
			     impl_ptr);

        object_ref = bonobo_object_corba_objref (
		BONOBO_OBJECT (extender_factory));

        if (BONOBO_EX (ev) || object_ref == CORBA_OBJECT_NIL) {
                printf ("Server cannot get objref\n");
                return CORBA_OBJECT_NIL;
        }

        return object_ref;
}

static const OAFPluginObject file_extender_plugin_list[] = {
        {
                "OAFIID:Bonobo_MonikerExtender_fileFactory",
                make_file_extender_factory
        },
        {
                NULL
  	}
};

const OAFPlugin OAF_Plugin_info = {
        file_extender_plugin_list,
        "bonobo file moniker extender"
};

