/*
 *  GNOME: a cool desktop
 *
 *  Copyright (C) 1999 Elliot Lee
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *          Elliot Lee <sopwith@redhat.com>
 *
 */
/* This module takes care of handling application and library
   initialization and command line parsing */

#ifndef GNOMELIB_INIT2_H
#define GNOMELIB_INIT2_H

#include <glib.h>
#include <popt.h>
#include <stdarg.h>

typedef struct _GnomeProgram GnomeProgram;

/* Returns the "application" object, creating it if it doesn't exist. */
/*@observer@*/ GnomeProgram *gnome_program_get(void);
/*@observer@*/ const char *gnome_program_get_name(/*@in@*/ GnomeProgram *app);
/*@observer@*/ const char *gnome_program_get_version(/*@in@*/ GnomeProgram *app);

/***** application attributes ******/
typedef enum {
  GNOME_ATTRIBUTE_NONE=0,
  GNOME_ATTRIBUTE_STRING,
  GNOME_ATTRIBUTE_INTEGER,
  GNOME_ATTRIBUTE_BOOLEAN,
  GNOME_ATTRIBUTE_STRING_VECTOR,
  GNOME_ATTRIBUTE_DOUBLE,
  GNOME_ATTRIBUTE_POINTER
} GnomeAttributeType;

typedef /*@abstract@*/ struct {
  union {
    char *string_value;
    int integer_value;
    double double_value;
    gboolean boolean_value;
    char **string_vector_value;
    gpointer pointer_value;
  } u;
  GnomeAttributeType type : 8;
} GnomeAttributeValue;

typedef /*@abstract@*/ struct {
  char *name;

  GnomeAttributeValue value;
} GnomeAttribute;

/* These settings are on a per-process basis, e.g. for
   affecting the initialization of modules. They are not persistent
   between application invocations.

   Literal strings should not be passed for 'name' - please use the
   defines provided by the module.
*/

void gnome_program_attribute_set(GnomeProgram *app,
			    const char *name,
			    const GnomeAttributeValue *value);
/*@observer@*/ const GnomeAttributeValue *gnome_program_attribute_get(/*@in@*/ GnomeProgram *app,
								      /*@in@*/ const char *name);
/* Convenience functions for C */
void gnome_program_attributes_get(GnomeProgram *app, ...);
void gnome_program_attributes_getv(GnomeProgram *app, va_list args);
void gnome_program_attributes_set(GnomeProgram *app, ...);
void gnome_program_attributes_setv(GnomeProgram *app, va_list args);

/***** application modules (aka libraries :) ******/
typedef struct _GnomeModuleInfo GnomeModuleInfo;

typedef struct {
  const char *required_version;
  GnomeModuleInfo *module_info;
} GnomeModuleRequirement;

typedef void (*GnomeModuleHook)(/*@in@*/ GnomeProgram *app, /*@in@*/ const GnomeModuleInfo *mod_info);

/*@abstract@*/ struct _GnomeModuleInfo {
  const char *name, *version, *description;
  GnomeModuleRequirement *requirements; /* last element has NULL version */

  GnomeModuleHook pre_args_parse, post_args_parse;

  struct poptOption *options;

  GnomeModuleHook init_pass; /* This gets run before the preinit
				function to allow the module to
				register other modules as needed. The
				module cannot assume its required
				modules are initialized (they aren't). */
  gpointer expansion1, expansion2;
};

/* This function should be called before gnomelib_preinit() - it's an alternative
   to the GNOME_PARAM_MODULE thing passed by the app. */
void gnome_program_module_register(/*@in@*/ GnomeProgram *app,
				   /*@in@*/ const GnomeModuleInfo *module_info);
gboolean gnome_program_module_registered(/*@in@*/ GnomeProgram *app,
					 const GnomeModuleInfo *module_info);
void gnome_program_module_load(GnomeProgram *app, const char *mod_name); /* do the dlopen funky, yea yea yea */

/*
  In order to process arguments, gnomelib_preinit needs to know their
  types. This is done by prefacing each attribute name with a character that
  defines its type.

  The characters for each type are:
     'S' for string
     'V' for string vectors
     'I' for integers
     'D' for doubles
     'B' for "gboolean"
     'F' for all function pointers
     'P' for pointers (aka "miscellaneous hack-ins", inevitably)
     'A' for string vector appendation - this adds one string onto a string vector.

  For libraries wishing to define their own
  LIBNAME_PARAM_... constants, the naming convention is
  "T:modulename/attributename" ("T" being the attribute's type character.)
  A modulename of '!' is an attribute that's part of the basic framework.

  Attribute type 'A' IS VALID FOR 'set' OPERATIONS ONLY! Libraries should provide
  separate #define constants for append and set on string vectors.
*/
extern const char gnome_param_popt_table[], gnome_param_popt_flags[], gnome_param_popt_context[], gnome_param_module[];
#define GNOME_PARAM_POPT_TABLE gnome_param_popt_table
#define GNOME_PARAM_POPT_FLAGS gnome_param_popt_flags
#define GNOME_PARAM_POPT_CONTEXT gnome_param_popt_context
#define GNOME_PARAM_MODULE gnome_param_module
#define GNOME_PARAM_HUMAN_READABLE_NAME gnome_param_human_readable_name

/*
 * If the application writer wishes to use getopt()-style arg
 * processing, they can do it using a while looped sandwiched between
 * calls to these two functions.
 */
/*@observer@*/
poptContext gnome_program_preinit(/*@in@*/ GnomeProgram *app,
				  /*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
				  int argc, /*@in@*/ char **argv, ...);
/*@observer@*/
poptContext gnome_program_preinitv(/*@in@*/ GnomeProgram *app,
				   /*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
				   int argc, /*@in@*/ char **argv, va_list args);
/*@observer@*/
poptContext gnome_program_preinita(/*@in@*/ GnomeProgram *app,
				   /*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
				   int argc, /*@in@*/ char **argv, GnomeAttribute *attrs);
void gnome_program_parse_args(/*@in@*/ GnomeProgram *app);
void gnome_program_postinit(/*@in@*/ GnomeProgram *app);

/* These are convenience functions that calls gnomelib_preinit(...), have
   popt parse all args, and then call gnomelib_postinit() */
void gnome_program_init(/*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
			int argc, /*@in@*/ char **argv, ...);
void gnome_program_initv(/*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
			 int argc, /*@in@*/ char **argv, va_list args);
void gnome_program_inita(/*@in@*/ const char *app_id, /*@in@*/ const char *app_version,
			 int argc, /*@in@*/ char **argv, GnomeAttribute *attrs);

/* Some systems, like Red Hat 4.0, define these but don't declare
   them.  Hopefully it is safe to always declare them here.  */
extern char *program_invocation_short_name;
extern char *program_invocation_name;

#endif
