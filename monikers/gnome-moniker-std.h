#ifndef _GNOME_MONIKER_STD_H_
#define _GNOME_MONIKER_STD_H_

#include <bonobo/bonobo-moniker-simple.h>
#include <bonobo/bonobo-moniker-extender.h>

Bonobo_Unknown bonobo_moniker_file_resolve    (BonoboMoniker               *moniker,
					       const Bonobo_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

Bonobo_Unknown bonobo_moniker_vfs_resolve     (BonoboMoniker               *moniker,
					       const Bonobo_ResolveOptions *options,
					       const CORBA_char            *requested_interface,
					       CORBA_Environment           *ev);

Bonobo_Unknown bonobo_file_extender_resolve   (BonoboMonikerExtender *extender,
					       const Bonobo_Moniker   m,
					       const Bonobo_ResolveOptions *options,
					       const CORBA_char      *display_name,
					       const CORBA_char      *requested_interface,
					       CORBA_Environment     *ev);

#endif /* _GNOME_MONIKER_STD_H_ */
