/*
 * bonobo-moniker-file.c: Sample file-system based Moniker implementation
 *
 * This is the file-system based Moniker implementation.
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000, Helix Code, Inc.
 */
#include <config.h>

#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-storage.h>
#include <bonobo/bonobo-moniker-util.h>

#include "bonobo-moniker-std.h"

Bonobo_Unknown
bonobo_moniker_file_resolve (BonoboMoniker               *moniker,
			     const Bonobo_ResolveOptions *options,
			     const CORBA_char            *requested_interface,
			     CORBA_Environment           *ev)
{
	const char    *fname = bonobo_moniker_get_name (moniker);
	Bonobo_Unknown retval;

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

		return CORBA_Object_duplicate (BONOBO_OBJREF (stream), ev);

	} else if (!strcmp (requested_interface, "IDL:Bonobo/Storage:1.0")) {
		BonoboStorage *storage;
		
		storage = bonobo_storage_open ("fs", fname,
					       Bonobo_Storage_READ, 0664);

		if (!storage) {
			g_warning ("Failed to open storage '%s'", fname);
			CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
					     ex_Bonobo_Moniker_InterfaceNotFound, NULL);
			return CORBA_OBJECT_NIL;
		}

		return CORBA_Object_duplicate (BONOBO_OBJREF (storage), ev);
	}

	retval = bonobo_moniker_use_extender (
		"OAFIID:Bonobo_MonikerExtender_file",
		moniker, options, requested_interface, ev);

	if (BONOBO_EX (ev))
		return CORBA_OBJECT_NIL;
	
	if (retval == CORBA_OBJECT_NIL)
		retval = bonobo_moniker_use_extender (
			"OAFIID:Bonobo_MonikerExtender_stream",
			moniker, options, requested_interface, ev);

	return retval;
}
