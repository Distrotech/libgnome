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
	printf ("file_extender_resolve: test %08x %s %s\n", m,
		display_name, requested_interface);

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
