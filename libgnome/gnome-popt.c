/*
 * Copyright (C) 1998, 1999, 2000 Red Hat, Inc.
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

#include <config.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <sys/stat.h>
#include <pwd.h>
#include "gnome-defs.h"
#include "gnomelib-init2.h"
#include "gnome-popt.h"

static GArray *opt_tables = NULL;

/**
 * gnomelib_register_popt_table:
 * @options: a poptOption table to register
 * @description: description for this table of options.
 *
 * Registers the @options table of command line options for the GNOME command line
 * argument parser.
 */
void
gnomelib_register_popt_table (const struct poptOption *options,
			      const char *description)
{
	int i;
	struct poptOption includer = {NULL, '\0', POPT_ARG_INCLUDE_TABLE,
				      NULL, 0, NULL, NULL};
	includer.arg = (struct poptOption *)options;
	includer.descrip = (char *)description;
	
	if(opt_tables == NULL)
		opt_tables = g_array_new (TRUE, TRUE, sizeof (struct poptOption));
	
	/* prevent multiple registration of arg tables */
	for(i = 0; i < opt_tables->len; i++) {
		if (g_array_index (opt_tables, struct poptOption, i).arg == options)
			return;
	}
	
	g_array_append_val (opt_tables, includer);
}

/**
 * gnomelib_parse_args:
 * @argc: the argument count to parse
 * @argv: a vector with the argument values
 * @popt_flags: popt flags that control the parse process
 *
 * Parses the arguments in argv according to all of the registered
 * tables.
 *
 * Returns a poptContext to the remaining arguments.
 */
poptContext
gnomelib_parse_args (int argc, char *argv[], int popt_flags)
{
	poptContext retval;
	int nextopt;
	struct poptOption end_opt;

	memset(&end_opt, 0, sizeof(end_opt));
	g_array_append_val(opt_tables, end_opt);

	gnome_program_init(g_basename(argv[0]), "broken!", argc, argv, GNOME_PARAM_POPT_FLAGS, popt_flags,
			   GNOME_PARAM_POPT_TABLE, opt_tables->data, NULL);
	gnome_program_attributes_get(gnome_program_get(), GNOME_PARAM_POPT_CONTEXT, &retval, NULL);

	return retval;
}
