/* Argument-parsing registry for libgnome.
   Written by Tom Tromey <tromey@cygnus.com>.  */

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
