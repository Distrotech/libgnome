#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <libbonobo.h>
#include <orbit/poa/poa.h>

int
main (int argc, char *argv [])
{
	Bonobo_Unknown    ref;
	CORBA_Environment *ev, real_ev;

	free (malloc (8));

	if (bonobo_init (&argc, argv) == FALSE)
		g_error ("Can not bonobo_init");
	bonobo_activate ();

	ev = &real_ev;
	CORBA_exception_init (ev);

	ref = oaf_activate (
		"repo_ids.has ('IDL:Bonobo/Moniker:1.0') AND "
		"bonobo:moniker.has ('file:')", NULL, 0, NULL, ev);
	g_assert (ev->_major == CORBA_NO_EXCEPTION);
	g_assert (ref != CORBA_OBJECT_NIL);
	bonobo_object_release_unref (ref, ev);
	g_assert (ev->_major == CORBA_NO_EXCEPTION);

	ref = bonobo_get_object ("file:/tmp/foo", "Bonobo/Stream", ev);
	g_assert (ev->_major == CORBA_NO_EXCEPTION);
	g_assert (ref != CORBA_OBJECT_NIL);
	bonobo_object_release_unref (ref, ev);
	g_assert (ev->_major == CORBA_NO_EXCEPTION);

	CORBA_exception_free (ev);

	return 0;
}
