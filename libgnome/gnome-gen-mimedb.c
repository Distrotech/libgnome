/*
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Gnome Library; see the file COPYING.  If not, write
 * to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif
#include <sys/types.h>

#include <libgnome/libgnome.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "gnome-magic.h"

extern GnomeMagicEntry *gnome_magic_parse (const char *filename, int *nents);

int main(int argc, char *argv[])
{
	GnomeMagicEntry *ents = NULL;
	char *filename = NULL, *out_filename;
	int nents;

	FILE *f;

	gnome_program_init("gnome-gen-mimedb", VERSION, argc, argv, LIBGNOME_PARAM_CREATE_DIRECTORIES, FALSE, NULL);

	if(argc > 1) {
		if(argv[1][0] == '-') {
			fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
			return 1;
		} else if(g_file_exists(argv[1]))
			filename = argv[1];
	} else
		filename = gnome_config_file("mime-magic");

	if(!filename) {
		printf("Input file does not exist (or unspecified)...\n");
		printf("Usage: %s [filename]\n", argv[0]);
		return 1;
	}

	ents = gnome_magic_parse(filename, &nents);

	if(!nents){
		fprintf (stderr, "%s: Error parsing the %s file\n", argv [0], filename);
		return 0;
	}

	out_filename = g_strconcat(filename, ".dat", NULL);

	f = fopen (out_filename, "w");
	if (f == NULL){
		fprintf (stderr, "%s: Can not create the output file %s\n", argv [0], out_filename);
		return 1;
	}

	if(fwrite(ents, sizeof(GnomeMagicEntry), nents, f) != nents){
		fprintf (stderr, "%s: Error while writing the contents of %s\n", argv [0], out_filename);
		fclose(f);
		return 1;
	}

	fclose(f);

	return 0;
}
