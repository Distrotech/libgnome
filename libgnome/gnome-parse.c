/* Argument-parsing registry for libgnome.
   Written by Tom Tromey <tromey@cygnus.com>.  */
#include "gnome-defs.h"
#include "gnome-parse.h"

static error_t default_parse_func (int key, char *arg,
				   struct argp_state *state);


/* Number of items allocated for `arguments', below.  */
static int args_size;

/* Index of next empty argument.  */
static int args_count;

/* Vector of all registered child argument structures.  */
static struct argp_child *arguments;

/* Default argument parser.  */
static struct argp default_parser =
{
  NULL,
  default_parse_func,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};



/* The default parser reject all arguments and returns options (and
   everything else) as unknown.  */
static error_t
default_parse_func (int key, char *arg, struct argp_state *state)
{
  if (key == ARGP_KEY_ARG)
    {
      argp_usage (state);
      return EINVAL;
    }
  return ARGP_ERR_UNKNOWN;
}

void
gnome_parse_register_arguments (const struct argp *parser)
{
  /* We always need one extra argument structure at the end for
     termination.  */
  if (args_size == 0)
    {
      args_size = 5;
      arguments = (struct argp_child *) malloc (args_size
						* sizeof (struct argp_child));
    }
  else if (args_count <= args_size)
    {
      args_size += 5;
      arguments = (struct argp_child *) realloc (arguments,
						 args_size
						 * sizeof (struct argp_child));
    }

  arguments[args_count].argp = parser;
  arguments[args_count].flags = 0;
  arguments[args_count].header = "";
  /* FIXME: pick a better value here.  */
  arguments[args_count].group = -2;

  ++args_count;
  arguments[args_count].argp = NULL;
}

error_t
gnome_parse_arguments (struct argp *parser,
		       int argc, char **argv,
		       unsigned int flags,
		       int *arg_index)
{
  if (! parser)
    parser = &default_parser;
  parser->children = arguments;

  return argp_parse (parser, argc, argv, flags, arg_index, NULL);
}
