#ifndef __GNOME_POPT_H__
#define __GNOME_POPT_H__ 1


#include "gnome-defs.h"

BEGIN_GNOME_DECLS

/* This is here because it does not have its own #ifdef __cplusplus */
#include <popt-gnome.h>

void gnomelib_register_popt_table(const struct poptOption *options,
				  const char *description);

poptContext gnomelib_parse_args(int argc, char *argv[],
				int popt_flags);

/* Some systems, like Red Hat 4.0, define these but don't declare
   them.  Hopefully it is safe to always declare them here.  */
extern char *program_invocation_short_name;
extern char *program_invocation_name;
END_GNOME_DECLS

#endif /* __GNOME_HELP_H__ */
