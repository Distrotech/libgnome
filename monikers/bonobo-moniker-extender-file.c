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
#include <bonobo/bonobo-shlib-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-stream.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-moniker.h>
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-moniker-extender.h>
#include <libgnome/gnome-mime.h>
#include <liboaf/liboaf.h>

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
		fname = strchr (display_name, ':') + 1;
	else
		fname = display_name;

	g_warning ("Filename : '%s'", fname);

	mime_type = gnome_mime_type (fname);

	oaf_requirements = g_strdup_printf (
		"bonobo:supported_mime_types.has ('%s') AND repo_ids.has ('%s') AND "
		"repo_ids.has ('IDL:Bonobo/PersistFile:1.0')",
		mime_type, requested_interface);
		
	object = oaf_activate (oaf_requirements, NULL, 0, &ret_id, ev);
	g_warning ("Attempt activate object satisfying '%s': %p",
		   oaf_requirements, object);
	g_free (oaf_requirements);

	if (BONOBO_EX (ev) || object == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	persist = Bonobo_Unknown_queryInterface (
		object, "IDL:Bonobo/PersistFile:1.0", ev);

	if (BONOBO_EX (ev) || persist == CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (object, ev);
		return CORBA_OBJECT_NIL;
	}

	if (persist != CORBA_OBJECT_NIL) {
		Bonobo_PersistFile_load (persist, fname, ev);

		bonobo_object_release_unref (persist, ev);

		return bonobo_moniker_util_qi_return (
			object, requested_interface, ev);
	}
}

static BonoboObject *
file_extender_factory (BonoboGenericFactory *this,
		       void                 *data)
{
	return BONOBO_OBJECT (
		bonobo_moniker_extender_new (
			file_extender_resolve, NULL));
}

static CORBA_Object
make_file_extender_factory (PortableServer_POA poa,
			    const char        *iid,
			    gpointer           impl_ptr,
			    CORBA_Environment *ev)
{
	BonoboShlibFactory *extender_factory;

	extender_factory = bonobo_shlib_factory_new (
		"OAFIID:Bonobo_MonikerExtender_fileFactory",
		poa, impl_ptr, file_extender_factory, NULL);	

        return bonobo_object_corba_objref (
		BONOBO_OBJECT (extender_factory));
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

