/* Argument-parsing registry for libgnome.
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

#ifndef __GNOME_PARSE_H__
#define __GNOME_PARSE_H__

BEGIN_GNOME_DECLS

#include <argp.h>

/* Call this to register some command-line arguments with the central
   parser.  Typically this will be called by a library's
   initialization routine to have some library-specific command-line
   arguments parsed with the application's arguments.  */
extern void gnome_parse_register_arguments (const struct argp *parser);

/* Call this to actually parse the command-line arguments.  If ARGP is
   non-NULL, then it is used as the primary argp structure for
   argument parsing.  Typically this argument will be supplied by the
   actual application.  Return values are like argp_parse, which see.  */
extern error_t gnome_parse_arguments (struct argp *parser,
				      int argc, char **argv,
				      unsigned int flags,
				      int *arg_index);

END_GNOME_DECLS

#endif /* __GNOME_PARSE_H__ */
