#include "config.h"
#include <string.h>

#include <bonobo/bonobo-shlib-factory.h>
#include "gnome-moniker-std.h"

static BonoboObject *
bonobo_std_moniker_factory (BonoboGenericFactory *this,
			    const char           *object_id,
			    void                 *data)
{
	g_return_val_if_fail (object_id != NULL, NULL);

	if (!strcmp (object_id, "OAFIID:GNOME_Moniker_File"))

		return BONOBO_OBJECT (bonobo_moniker_simple_new (
			"file:", bonobo_moniker_file_resolve));

	else if (!strcmp (object_id, "OAFIID:GNOME_MonikerExtender_file"))
		
		return BONOBO_OBJECT (bonobo_moniker_extender_new (
			bonobo_file_extender_resolve, NULL));

	else
		g_warning ("Failing to manufacture a '%s'", object_id);

	return NULL;
}


BONOBO_OAF_SHLIB_FACTORY_MULTI ("OAFIID:Bonobo_Moniker_std_Factory",
				"bonobo standard moniker",
				bonobo_std_moniker_factory,
				NULL);


