/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef __GNOME_POPT_H__
#define __GNOME_POPT_H__ 1


#include "gnome-defs.h"

BEGIN_GNOME_DECLS

/* This is here because it does not have its own #ifdef __cplusplus */
#include <popt.h>

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
