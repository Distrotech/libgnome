/*
 *
 * Gnome popt routines.
 * (C) 1998 Red Hat Software
 *
 * Author: Elliot Lee
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
