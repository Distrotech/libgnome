/* gnome-dump.c - Dump the metadata database.

   Copyright (C) 1998 Tom Tromey

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <config.h>

#ifdef HAVE_DB_185_H
# include <db_185.h>
#else
# include <db.h>
#endif
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <libgnome/libgnome.h>

int
main (int argc, char *argv[])
{
	char *filename;
	DB *db;
	DBT key, value;
	int flag, n;
	char *p, *end;

	if (argc == 2)
		filename = argv[1];
	else
		filename = gnome_util_home_file ("metadata.db");

	db = dbopen (filename, O_RDONLY, 0700, DB_HASH, NULL);
	if (! db) {
		fprintf (stderr, "Couldn't open `%s' for reading: %s\n",
			 filename, strerror (errno));
		exit (1);
	}

	flag = R_FIRST;
	while (1) {
		int ret = db->seq (db, &key, &value, flag);
		flag = R_NEXT;

		if (ret == -1) {
			fprintf (stderr, "Error reading database: %s\n",
				 strerror (errno));
			exit (1);
		} else if (ret) {
			break;
		}

		p = key.data;
		n = strlen (p);
		printf ("%s ", p);
		if (n > 4 && ! strcmp (p + n - 4, "list")) {
			/* Got a special element.  */
			p += n + 1;
			printf (" %s\n", p);

			p = value.data;
			end = p + value.size;
			while (p < end) {
				printf ("    %s\n", p);
				p += strlen (p) + 1;
			}
			printf ("\n");
		} else {
			/* Ordinary element.  */
			printf (" %s", p);
			p += strlen (p) + 1;
			printf (" %s  (value not printed)\n", p);
		}
	}

	db->close (db);
	exit (0);
}
