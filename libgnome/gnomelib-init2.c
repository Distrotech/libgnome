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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <gmodule.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-portability.h"
#include "libgnome/gnomelib-init2.h"

/* data encapsulated by GnomeProgram:
   state
   attributes (string -> GnomeAttribute lookups)
   modules (array of pointers)
*/
/*@abstract@*/ struct _GnomeProgram {
  enum {
    APP_UNINIT=0,
    APP_CREATE_DONE=1,
    APP_PREINIT_DONE=2,
    APP_POSTINIT_DONE=3
  } state;
  /*@only@*/ GHashTable *attributes;

  /*@only@*/ GPtrArray *modules; /* valid-while: APP_UNINIT < state < APP_POSTINIT_DONE */

  /* valid-while: state > APP_CREATE_DONE */
  /*@only@*/ char *app_id;
  /*@only@*/ char *app_version;
  /*@only@*/ char **argv;
  int argc;

  /* Holds a 'poptContext' */
  GnomeAttributeValue arg_context_val; /* valid-while: state == APP_PREINIT_DONE */

  /* holds a 'struct poptOptions *' */
  GnomeAttributeValue app_options_val; /* valid-while: state == APP_PREINIT_DONE */

  /* holds an 'int' */
  GnomeAttributeValue app_popt_flags_val; /* valid-while: state == APP_PREINIT_DONE */

  /*@only@*/ GArray *top_options_table; /* valid-while: state == APP_PREINIT_DONE */
};

/**
 * gnome_program_get:
 *
 * Returns an object that stores information on the GNOME application's state.
 * If the object does not exist, it is created.
 */
/* Other functions assume that this will always return an appobject
   with state > APP_UNINIT */

GnomeProgram *
gnome_program_get(void)
{
  /*@only@*/ static GnomeProgram *gnome_app = NULL;

  if (!gnome_app)
    {
      gnome_app = g_new(GnomeProgram, 1);
      gnome_app->attributes = g_hash_table_new(g_str_hash, g_str_equal);

      gnome_app->arg_context_val.type = GNOME_ATTRIBUTE_NONE;
      gnome_app->app_options_val.type = GNOME_ATTRIBUTE_NONE;
      gnome_app->app_popt_flags_val.type = GNOME_ATTRIBUTE_NONE;

      gnome_app->argv = NULL;
      gnome_app->argc = 0;
      gnome_app->modules = NULL;
      gnome_app->top_options_table = NULL;

      gnome_app->app_version = gnome_app->app_id = NULL;

      gnome_app->state = APP_CREATE_DONE;
    }

  return gnome_app;
}

/**
 * gnome_program_get_name
 * @app: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as an identifier. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: application ID string.
 */
const char *
gnome_program_get_name(GnomeProgram *app)
{
  g_return_val_if_fail (app, NULL);
  g_return_val_if_fail (app->state >= APP_PREINIT_DONE, NULL);

  return app->app_id;
}

/**
 * gnome_program_get_version
 * @app: The application object
 *
 * Description:
 * This function returns a pointer to a static string that the
 * application has provided as a version number. This is not meant as a
 * human-readable identifier so much as a unique identifier for
 * programs and libraries.
 *
 * Returns: application version string.
 */
const char *
gnome_program_get_version(GnomeProgram *app)
{
  g_return_val_if_fail (app, NULL);
  g_return_val_if_fail (app->state >= APP_PREINIT_DONE, NULL);

  return app->app_version;
}

/***** attributes *****/

static void
gnome_program_framework_attrs_set(GnomeProgram *app,
			     const char *name,
			     const GnomeAttributeValue *value)
{
  if (!strcmp(name, GNOME_PARAM_POPT_TABLE))
    {
      g_assert(value->type == GNOME_ATTRIBUTE_POINTER);
      app->app_options_val = *value;
    }
  else if (!strcmp(name, GNOME_PARAM_POPT_FLAGS))
    {
      g_assert(value->type == GNOME_ATTRIBUTE_INTEGER);
      app->app_popt_flags_val = *value;
    }
  else if (!strcmp(name, GNOME_PARAM_MODULE))
    {
      g_assert(value->type == GNOME_ATTRIBUTE_POINTER);
      gnome_program_module_register(app, (GnomeModuleInfo *)value->u.pointer_value);
    }
  else if (!strcmp(name, GNOME_PARAM_POPT_CONTEXT))
    {
      if(app->arg_context_val.type != GNOME_ATTRIBUTE_NONE)
	{
	  poptFreeContext(app->arg_context_val.u.pointer_value);
	  app->arg_context_val.type = GNOME_ATTRIBUTE_NONE;
	  app->arg_context_val.u.pointer_value = NULL;
	}
    }
  else
    g_error("Unknown framework attribute '%s'", name);
}

/*@observer@*/ static const GnomeAttributeValue *
gnome_program_framework_attrs_get(GnomeProgram *app,
			     const char *name)
{
  if (!strcmp(name, GNOME_PARAM_POPT_CONTEXT))
    return &(app->arg_context_val);
  else if (!strcmp(name, GNOME_PARAM_POPT_FLAGS))
    return &(app->app_popt_flags_val);
  else if (!strcmp(name, GNOME_PARAM_POPT_TABLE))
    return &(app->app_options_val);
  else if (!strcmp(name, GNOME_PARAM_MODULE)) 
    g_error("You cannot retrieve a module as an attribute.");

  return NULL;
}

/**
 * gnome_program_attribute_set:
 * @app: the application object
 * @name: The attribute name, via the library-provided #define
 * @value: A structure representing the value to be set.
 *
 * This function sets an attribute on the application object, for use in
 * controlling the initialization of the application.
 */
void
gnome_program_attribute_set(GnomeProgram *app,
			    const char *name,
			    const GnomeAttributeValue *value)
{
  GnomeAttribute *new_value, *stored_value;
  gboolean do_append;
  GnomeAttributeType should_be;
  char *faked_name;

  g_return_if_fail(app);
  g_return_if_fail(name);
  g_return_if_fail(value);
  g_return_if_fail(name[1] == ':');
  /* Assumptions: We assume that app->state > APP_UNINIT - as long as
     it is impossible for other modules to get hold of a GnomeProgram
     without using gnome_program_get(), we're safe. */

  if(name[2] == '!')
    {
      gnome_program_framework_attrs_set(app, name, value);
      return;
    }

  do_append = FALSE;
  faked_name = (char *)name;
  switch(name[0])
    {
    case 'A':
      {
	do_append = TRUE;
	faked_name = g_alloca(strlen(name) + 1);
	strcpy(faked_name, name);
	faked_name[0] = 'V';
      }
    case 'S':
      should_be = GNOME_ATTRIBUTE_STRING;
      break;
    case 'V':
      should_be = GNOME_ATTRIBUTE_STRING_VECTOR;
      break;
    case 'I':
      should_be = GNOME_ATTRIBUTE_INTEGER;
      break;
    case 'D':
      should_be = GNOME_ATTRIBUTE_DOUBLE;
      break;
    case 'B':
      should_be = GNOME_ATTRIBUTE_BOOLEAN;
      break;
    case 'F': /* Note: To be ANSI-correct, we need a separate case for
		 function pointers, since they're not guaranteed to be
		 the same size as data pointers. For now, hack it */
    case 'P':
      should_be = GNOME_ATTRIBUTE_POINTER;
      break;
    default:
      should_be = 0;
      g_error("Application attribute name \"%s\" is invalid", name);
      break;
    }

  g_return_if_fail(should_be == value->type);

      /* free old value */
  stored_value = g_hash_table_lookup(app->attributes, faked_name);
  if(stored_value)
    {
      if(do_append)
	{
	  g_return_if_fail(stored_value->value.type == GNOME_ATTRIBUTE_STRING_VECTOR);
	}
      else
	{
	  switch(stored_value->value.type) {
	  case GNOME_ATTRIBUTE_STRING:
	    g_free(stored_value->value.u.string_value);
	    break;
	  case GNOME_ATTRIBUTE_STRING_VECTOR:
	    g_strfreev(stored_value->value.u.string_vector_value);
	    break;
	  default:
	    break;
	  }
	}

      new_value = stored_value;
    }
  else
    {
      new_value = g_new0(GnomeAttribute, 1);
      new_value->name = g_strdup(faked_name);

      if(do_append)
	new_value->value.type = GNOME_ATTRIBUTE_STRING_VECTOR;

      g_hash_table_insert(app->attributes, (char *)new_value->name,
			  new_value);
    }

  /* store new value */

  if(do_append)
    {
      int i;

      for(i = 0; value->u.string_vector_value && value->u.string_vector_value[i]; i++) /* */;

      new_value->value.u.string_vector_value = g_realloc(new_value->value.u.string_vector_value, sizeof(char *) * (i+2));
      new_value->value.u.string_vector_value[i] = g_strdup(value->u.string_vector_value[i]);
      new_value->value.u.string_vector_value[i+1] = NULL;
    }
  else
    {
      new_value->value = *value;
      switch(value->type) {
      case GNOME_ATTRIBUTE_STRING:
	new_value->value.u.string_value = g_strdup(value->u.string_value);
	break;
      case GNOME_ATTRIBUTE_STRING_VECTOR:
	{
	  int i;
	  for(i = 0; value->u.string_vector_value[i]; i++) /* */;
	  new_value->value.u.string_vector_value = g_new(char *, i);
	  for(i = 0; value->u.string_vector_value[i]; i++)
	    new_value->value.u.string_vector_value[i] = g_strdup(value->u.string_vector_value[i]);
	  new_value->value.u.string_vector_value[i] = NULL;
	}
	break;
      default:
	break;
      }
    }
}

/**
 * gnome_program_attributes_set:
 * @app: Application object
 * @...: a NULL-terminated list of attribute name/value pairs
 */
void
gnome_program_attributes_set(GnomeProgram *app, ...)
{
  va_list args;

  va_start(args, app);
  gnome_program_attributes_setv(app, args);
  va_end(args);
}

/**
 * gnome_program_attributes_setv:
 * @app: Application object
 * @args: a va_list representing a NULL-terminated list of attribute name/value pairs
 *
 * Sets the specified attributes to the specified values.
 */
void
gnome_program_attributes_setv(GnomeProgram *app, va_list args)
{
  GnomeAttribute attr;
  
  while((attr.name = va_arg(args, char*))) {
    g_assert(attr.name[1] == ':');

    switch (attr.name[0])
      {
      case 'P':
      case 'F':
	attr.value.type = GNOME_ATTRIBUTE_POINTER;
	attr.value.u.pointer_value = va_arg(args, gpointer);
	break;
      case 'A':
      case 'S':
	attr.value.type = GNOME_ATTRIBUTE_STRING;
	attr.value.u.string_value = va_arg(args, char*);
	break;
      case 'V':
	attr.value.type = GNOME_ATTRIBUTE_STRING_VECTOR;
	attr.value.u.string_vector_value = va_arg(args, char**);
	break;
      case 'D':
	attr.value.type = GNOME_ATTRIBUTE_DOUBLE;
	attr.value.u.double_value = va_arg(args, gdouble);
	break;
      case 'I':
	attr.value.type = GNOME_ATTRIBUTE_INTEGER;
	attr.value.u.integer_value = va_arg(args, gint);
	break;
      case 'B':
	attr.value.type = GNOME_ATTRIBUTE_BOOLEAN;
	attr.value.u.boolean_value = va_arg(args, gboolean);
	break;
      default:
	attr.value.type = GNOME_ATTRIBUTE_NONE;
	g_error("Application attribute name \"%s\" is invalid", attr.name);
	break;
      }	

    gnome_program_attribute_set(app, attr.name, &attr.value);
  }
}


/**
 * gnome_program_attribute_get:
 * @app: application object
 * @name: The attribute name, via the library-provided #define
 *
 * Description:
 * Retrieves a pointer to the value of the specified attribute.
 * The memory pointed to continues to be owned by the GNOME libraries.
 *
 * Returns: A pointer to a GnomeAttributeValue.
 */
/*@observer@*/ const GnomeAttributeValue *
gnome_program_attribute_get(GnomeProgram *app,
		       const char *name)
{
  GnomeAttribute *attr;

  g_return_val_if_fail(app, NULL);
  g_return_val_if_fail(name, NULL);
  g_return_val_if_fail(name[1] == ':', NULL);
  g_return_val_if_fail(name[0] != 'A', NULL);

  /* Assumptions: We assume that app->state > APP_UNINIT - as long as
     it is impossible for other modules to get hold of a GnomeProgram
     without using gnome_program_get(), we're safe. */

  if(name[2] == '!')
    return gnome_program_framework_attrs_get(app, name);
 
  attr = g_hash_table_lookup(app->attributes, name);
  if(attr)
    return &(attr->value);
  else
    return NULL;
}
  
/**
 * gnome_program_attributes_get:
 * @app: application object
 * @...: attribute name/return-value-pointer pairs
 *
 * This function is a convenience wrapper for gnome_program_attribute_get that allows passing
 * attribute names, and pointers to variables where the attribute value is to be stored.
 */
void
gnome_program_attributes_get(GnomeProgram *app, ...)
{
  va_list args;

  va_start(args, app);
  gnome_program_attributes_getv(app, args);
  va_end(args);
}

/**
 * gnome_program_attributes_getv:
 * @app: application object
 * @args: attribute name/return-value-pointer pairs
 *
 * This function is a convenience wrapper for gnome_program_attribute_get that allows passing
 * attribute names, and pointers to variables where the attribute value is to be stored.
 */
void
gnome_program_attributes_getv(GnomeProgram *app, va_list args)
{
  const GnomeAttributeValue *val;
  char *attr_name;
  gpointer ret_pointer;

  while((attr_name = va_arg(args, char*))) {
    val = gnome_program_attribute_get(app, attr_name);

    ret_pointer = va_arg(args, gpointer);

    if(!val)
      continue;

    switch (attr_name[0])
      {
      case 'P':
      case 'F':
	{
	  gpointer *pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_POINTER);

	  *pval = val->u.pointer_value;
	}
	break;
      case 'S':
	{
	  gchar **pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_STRING);

	  *pval = val->u.string_value;
	}
	break;
      case 'V':
	{
	  gchar ***pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_STRING_VECTOR);

	  *pval = val->u.string_vector_value;
	}
	break;
      case 'D':
	{
	  gdouble *pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_DOUBLE);
	  *pval = val->u.double_value;
	}
	break;
      case 'I':
	{
	  gint *pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_INTEGER);

	  *pval = val->u.integer_value;
	}
	break;
      case 'B':
	{
	  gboolean *pval = ret_pointer;

	  g_assert(val->type == GNOME_ATTRIBUTE_BOOLEAN);
	  *pval = val->u.boolean_value;
	}
	break;
      default:
	g_error("Application attribute name \"%s\" is invalid", attr_name);
	break;
      }	
  }
}


/******** modules *******/

/* Stolen verbatim from rpm/lib/misc.c 
   RPM is Copyright (c) 1998 by Red Hat Software, Inc.,
   and may be distributed under the terms of the GPL and LGPL.
*/
/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
static int rpmvercmp(const char * a, const char * b) {
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;
    
    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    str1 = g_alloca(strlen(a) + 1);
    str2 = g_alloca(strlen(b) + 1);

    strcpy(str1, a);
    strcpy(str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one && *two) {
	while (*one && !isalnum(*one)) one++;
	while (*two && !isalnum(*two)) two++;

	str1 = one;
	str2 = two;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (isdigit(*str1)) {
	    while (*str1 && isdigit(*str1)) str1++;
	    while (*str2 && isdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && isalpha(*str1)) str1++;
	    while (*str2 && isalpha(*str2)) str2++;
	    isnum = 0;
	}
		
	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';

	/* take care of the case where the two version segments are */
	/* different types: one numeric and one alpha */
	if (one == str1) return -1;	/* arbitrary */
	if (two == str2) return -1;

	if (isnum) {
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */
	  
	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    if (strlen(one) > strlen(two)) return 1;
	    if (strlen(two) > strlen(one)) return -1;
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) return rc;
	
	/* restore character that was replaced by null above */
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}

static gboolean
gnome_program_version_check(const char *required_version,
		       const char *provided_version)
{
  if(required_version && provided_version)
    return (rpmvercmp(required_version, provided_version) >= 0);
  else
    return TRUE;
}

/**
 * gnome_program_module_register:
 * @app: Application object
 * @module_info: A pointer to a GnomeModuleInfo structure describing the module
 *               to be initialized
 *
 * Description:
 * This function is used to register a module to be initialized by the
 * GNOME library framework. The memory pointed to by 'module_info' must be
 * valid during the whole application initialization process, and the module
 * described by 'module_info' must only use the 'module_info' pointer to
 * register itself.
 *
 */
void
gnome_program_module_register(GnomeProgram *app,
			      const GnomeModuleInfo *module_info)
{
  int i;

  g_return_if_fail(app);
  g_return_if_fail(module_info);
  g_return_if_fail(app->state == APP_CREATE_DONE);

  if(!app->modules)
    app->modules = g_ptr_array_new();

  /* Check that it's not already registered. */

  if(gnome_program_module_registered(app, module_info))
    return;

  g_ptr_array_add(app->modules, (GnomeModuleInfo *)module_info);

  /* We register requirements *after* the module itself to avoid loops.
     Initialization order gets sorted out later on. */
  if(module_info->requirements)
    {
      for(i = 0; module_info->requirements[i].required_version; i++)
	{
	  GnomeModuleInfo *dep_mod;

	  dep_mod = module_info->requirements[i].module_info;
	  if(gnome_program_version_check(module_info->requirements[i].required_version,
				    dep_mod->version))
	    {
	      gnome_program_module_register(app, dep_mod);
	    }
	  else
	    {
	      /* The required version is not installed */
	      /* I18N needed */
	      g_message("Module '%s' requires version '%s' of module '%s' "
			"to be installed, and you only have version '%s' of '%s'. "
			"Aborting initialization.",
			module_info->name,
			module_info->requirements[i].required_version,
			dep_mod->name,
			dep_mod->version,
			dep_mod->name);
	    }
	}
    }

}

/**
 * gnome_program_module_registered:
 * @app: Application object
 * @module_info: A pointer to a GnomeModuleInfo structure describing the module
 *               to be queried
 *
 * Description: This method checks to see whether a specific module has been initialized in the specified program.
 *
 * Returns: A value indicating whether the specified module has been registered/initialized in the current program
 */
gboolean
gnome_program_module_registered(/*@in@*/ GnomeProgram *app,
				const GnomeModuleInfo *module_info)
{
  int i;
  GnomeModuleInfo *curmod;

  g_return_val_if_fail(app, FALSE);
  g_return_val_if_fail(module_info, FALSE);
  g_return_val_if_fail(app->state >= APP_CREATE_DONE, FALSE);

  for(i = 0; i < app->modules->len; i++)
    {
      curmod = g_ptr_array_index(app->modules, i);
      if(curmod == module_info
	 || !strcmp(curmod->name, module_info->name))
	return TRUE;
    }

  return FALSE;
}

/**
 * gnome_program_preinit:
 * @app: Application object
 * @app_id: application ID string
 * @app_version: application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 * @...: a NULL-terminated list of attribute name/value pairs
 *
 * Description:
 * This function performs the portion of application initialization that
 * needs to be done prior to command line argument parsing. The poptContext
 * returned can be used for getopt()-style option processing.
 *
 * Returns: A poptContext representing the argument parsing state.
 */
poptContext
gnome_program_preinit(GnomeProgram *app,
		 const char *app_id, const char *app_version,
		 int argc, char **argv, ...)
{
  poptContext retval;
  va_list args;

  va_start(args, argv);
  retval = gnome_program_preinitv(app, app_id, app_version, argc, argv, args);
  va_end(args);

  return retval;
}

/**
 * gnome_program_preinitv:
 * @app: Application object
 * @app_id: application ID string
 * @app_version: application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 * @args: a va_list representing a NULL-terminated list of attribute name/value pairs
 *
 * Description:
 * This function performs the portion of application initialization that
 * needs to be done prior to command-line argument parsing. The poptContext
 * returned can be used for getopt()-style option processing.
 *
 * Returns: A poptContext representing the argument parsing state.
 */
poptContext
gnome_program_preinitv(GnomeProgram *app,
		  const char *app_id, const char *app_version,
		  int argc, char **argv, va_list args)
{
  GnomeAttribute *attrs;
  int n_attrs;

  {
    va_list count_args;
    char *attr_name;

    G_VA_COPY(count_args, args);
    for (n_attrs = 0; (attr_name = va_arg(count_args, char*)); )
      {
	n_attrs++;

	if (attr_name[1] != ':')
	  g_error("Invalid application attribute name '%s'", attr_name);

      /* Skip over the value */
	switch (attr_name[0])
	  {
	  case 'P':
	  case 'F':
	  case 'S':
	  case 'A':
	  case 'V':
	    {
	      gpointer skipme;
	      skipme = va_arg(count_args, gpointer);
	    }
	    break;
	  case 'D':
	    {
	      gdouble skipme;
	      skipme = va_arg(count_args, gdouble);
	    }
	    break;
	  case 'I':
	    {
	      gint skipme;
	      skipme = va_arg(count_args, gint);
	    }
	    break;
	  case 'B':
	    {
	      gboolean skipme;
	      skipme = va_arg(count_args, gboolean);
	    }
	    break;
	  default:
	    g_error("Invalid application attribute type '%c'",
		    attr_name[0]);
	  }
      }

    va_end(count_args);
  }

  {
    va_list build_args;
    char *attr_name;
    int i;

    attrs = g_alloca((n_attrs + 1) * sizeof(attrs[0]));

    G_VA_COPY(build_args, args);

    for (i = 0; i < n_attrs; i++) 
      {
	attrs[i].name = attr_name = va_arg(build_args, char*);

	switch (attr_name[0])
	  {
	  case 'P':
	  case 'F':
	    attrs[i].value.type = GNOME_ATTRIBUTE_POINTER;
	    attrs[i].value.u.pointer_value = va_arg(build_args, gpointer);
	    break;
	  case 'A':
	  case 'S':
	    attrs[i].value.type = GNOME_ATTRIBUTE_STRING;
	    attrs[i].value.u.string_value = va_arg(build_args, char*);
	    break;
	  case 'V':
	    attrs[i].value.type = GNOME_ATTRIBUTE_STRING_VECTOR;
	    attrs[i].value.u.string_vector_value = va_arg(build_args, char**);
	    break;
	  case 'D':
	    attrs[i].value.type = GNOME_ATTRIBUTE_DOUBLE;
	    attrs[i].value.u.double_value = va_arg(build_args, gdouble);
	    break;
	  case 'I':
	    attrs[i].value.type = GNOME_ATTRIBUTE_INTEGER;
	    attrs[i].value.u.integer_value = va_arg(build_args, gint);
	    break;
	  case 'B':
	    attrs[i].value.type = GNOME_ATTRIBUTE_BOOLEAN;
	    attrs[i].value.u.boolean_value = va_arg(build_args, gboolean);
	    break;
	  }	
      }

    attrs[i].name = NULL;

    va_end(build_args);
  }

  return gnome_program_preinita(app, app_id, app_version, argc, argv, attrs);
}

static int
find_module_in_array(GnomeModuleInfo *ptr, GnomeModuleInfo **array)
{
  int i;

  for(i = 0; array[i] && array[i] != ptr; i++)
    {
      if(!strcmp(array[i]->name, ptr->name))
	break;
    }

  if(array[i])
    return i;
  else
    return -1;
}

static void /* recursive */
gnome_program_module_addtolist(GnomeProgram *app,
			       GnomeModuleInfo **new_list,
			       int *times_visited,
			       int *num_items_used,
			       int new_item_idx)
{
  GnomeModuleInfo *new_item;
  int i;

  g_assert(new_item >= 0);

  new_item = g_ptr_array_index(app->modules, new_item_idx);
  if(!new_item)
    return;

  if(find_module_in_array(new_item, new_list) >= 0)
    return; /* already cared for */

  /* Does this item have any dependencies? */
  if(times_visited[new_item_idx] > 0)
    {
      /* We already tried to satisfy all the dependencies for this module,
       *  and we've come back to it again. There's obviously a loop going on.
       */
      g_error("Module '%s' version '%s' has a requirements loop.",
	      new_item->name, new_item->version);
    }
  times_visited[new_item_idx]++;

  if(new_item->requirements)
    {
      for(i = 0; new_item->requirements[i].required_version; i++)
	{
	  int n;

	  n = find_module_in_array(new_item->requirements[i].module_info,
				   (GnomeModuleInfo **)app->modules->pdata);
	  gnome_program_module_addtolist
	    (app, new_list, times_visited, num_items_used, n);
	}
    }

  /* now add this module on */
  new_list[*num_items_used] = new_item;
  (*num_items_used)++;
  new_list[*num_items_used] = NULL;
}


static void
gnome_program_modules_order(GnomeProgram *app)
{
  int i;
  GnomeModuleInfo **new_list;
  int *times_visited; /* Detects dependency loops */
  int num_items_used;

  new_list = alloca(app->modules->len * sizeof(gpointer));
  new_list[0] = NULL;
  num_items_used = 0;
  
  times_visited = alloca(app->modules->len * sizeof(int));
  memset(times_visited, '\0', app->modules->len * sizeof(int));

  /* Create the new list with proper ordering */
  for(i = 0; i < (app->modules->len - 1); i++)
    {
      gnome_program_module_addtolist(app, new_list, times_visited,
				&num_items_used, i);
    }

  /* Now stick the new, ordered list in place */
  memcpy(app->modules->pdata, new_list, app->modules->len * sizeof(gpointer));
}

/**
 * gnome_program_module_load:
 * @app: Application object
 * @mod_name: module name
 *
 * Loads a shared library that contains a 'GnomeModuleInfo dynamic_module_info' structure.
 */
void
gnome_program_module_load(GnomeProgram *app, const char *mod_name)
{
  GModule *mh;
  GnomeModuleInfo *gmi;
  char tbuf[1024];

  g_snprintf(tbuf, sizeof(tbuf), "lib%s.so.0", mod_name);

  mh = g_module_open(mod_name, G_MODULE_BIND_LAZY);
  if(!mh)
    {
      g_snprintf(tbuf, sizeof(tbuf), "lib%s.so", mod_name);

      mh = g_module_open(mod_name, G_MODULE_BIND_LAZY);
    }

  if(!mh)
    return;

  if(g_module_symbol(mh, "dynamic_module_info", (gpointer *)&gmi))
    {
      gnome_program_module_register(app, gmi);
      g_module_make_resident(mh);
    }
  else
    g_module_close(mh);
}

/**
 * gnome_program_preinita:
 * @app: Application object
 * @app_id: application ID string
 * @app_version: application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 * @attrs: An array
 *
 * Description:
 * This function performs the portion of application initialization that
 * needs to be done prior to command-line argument parsing. The poptContext
 * returned can be used for getopt()-style option processing.
 * The 'preinita' version is mainly for use by language bindings which
 * cannot deal with C-style variable arguments.
 *
 * Returns: A poptContext representing the argument parsing state.
 */
poptContext
gnome_program_preinita(GnomeProgram *app,
		       const char *app_id, const char *app_version,
		       int argc, char **argv, GnomeAttribute *attrs)
{
  GnomeModuleInfo *a_module;
  poptContext argctx;
  int popt_flags = 0;
  int i;

  g_return_val_if_fail(app, NULL);
  if(app->state != APP_CREATE_DONE)
    return NULL;

  /* On non-glibc systems, this is not set up for us.  */
  if (!program_invocation_name) {
    program_invocation_name = argv[0];
    program_invocation_short_name = g_basename(program_invocation_name);
  }

  /* 0. Misc setup */
  if(app->app_id)
    g_free(app->app_id);
  app->app_id = g_strdup(app_id);
  g_set_prgname(app_id);
  if(app->app_version)
    g_free(app->app_version);
  app->app_version = g_strdup(app_version);
  app->argc = argc;
  app->argv = argv;

  if(!app->modules)
    app->modules = g_ptr_array_new();

  /* Major steps in this function:
     1. Process all framework attributes in 'attrs'
     2. Order the module list for dependencies
     3. Call the preinit functions for the modules
     4. Process other attributes 
     5. Create a top-level 'struct poptOption *' for use in arg-parsing.
     6. Create a poptContext
     7. Cleanup/return
   */

  /* 1. Process all framework attributes in 'attrs' */
  for (i = 0; attrs[i].name; i++)
    {
      if (attrs[i].name[2] == '!') /* is a framework attr */
	gnome_program_framework_attrs_set(app, attrs[i].name, &attrs[i].value);
    }

  /* We have to handle --load-modules=foo,bar,baz specially */
  for (i = 0; i < argc; i++)
    {
      if(!strncmp(argv[i], "--load-modules=", strlen("--load-modules=")))
	{
	  char **modnames;
	  int j;

	  modnames = g_strsplit(argv[i] + strlen("--load-modules="), ",", -1);
	  for(j = 0; modnames && modnames[j]; j++)
	    {
	      gnome_program_module_load(app, modnames[j]);
	    }
	  g_strfreev(modnames);
	}
    }

  {
    char *ctmp;
    if((ctmp = getenv("GNOME_MODULES")))
      {
	char **modnames;
	int j;

	modnames = g_strsplit(ctmp, ",", -1);
	for(j = 0; modnames && modnames[j]; j++)
	  {
	    gnome_program_module_load(app, modnames[j]);
	  }

	g_strfreev(modnames);
      }
  }

  /* 1.(b) call the init_pass functions, including those of modules
     that get registered from init_pass functions. */
  for(i = 0; i < app->modules->len; i++)
    {
      a_module = g_ptr_array_index(app->modules, i);

      if (a_module && a_module->init_pass)
	a_module->init_pass(app, a_module);
    }

  g_ptr_array_add(app->modules, NULL); /* Make sure the array
					  is NULL-terminated */

  /* 2. Order the module list for dependencies */
  gnome_program_modules_order(app);

  /* 3. call the pre-init functions */
  for (i = 0; (a_module = g_ptr_array_index(app->modules, i)); i++)
    {
      if (a_module->pre_args_parse)
	a_module->pre_args_parse(app, a_module);
    }

  /* 4. Process other attributes. We do this *after* calling pre-init
     because we want to allow the libraries to provide reasonable
     defaults, and the application to override those as needed. */
  for (i = 0; attrs[i].name; i++)
    {
      if (attrs[i].name[2] != '!') /* Not a framework attr */
	gnome_program_attribute_set(app, attrs[i].name, &attrs[i].value);
    }

  /* 5. Create a top-level 'struct poptOption *' for use in arg-parsing. */
  {
    struct poptOption includer = {NULL, '\0', POPT_ARG_INCLUDE_TABLE,
				  NULL, 0, NULL, NULL};

    app->top_options_table = g_array_new (TRUE, TRUE,
					  sizeof (struct poptOption));

    /* Put the special popt table in first */
    includer.arg = poptHelpOptions;
    includer.descrip = "Help options";
    g_array_append_val (app->top_options_table, includer);

    for (i = 0; (a_module = g_ptr_array_index(app->modules, i)); i++)
      {
	if (a_module->options)
	  {
	    includer.arg = a_module->options;
	    includer.descrip = (char *)a_module->description;

	    g_array_append_val (app->top_options_table, includer);
	  }
      }

    if ((app->app_options_val.type == GNOME_ATTRIBUTE_POINTER)
	&& app->app_options_val.u.pointer_value)
      {
	includer.arg = app->app_options_val.u.pointer_value;
	includer.descrip = app->app_id;

	g_array_append_val (app->top_options_table, includer);
      }

    includer.longName = "load-modules";
    includer.argInfo = POPT_ARG_STRING;
    includer.descrip = N_("Dynamic modules to load");
    includer.argDescrip = N_("MODULE1,MODULE2,...");
    g_array_append_val (app->top_options_table, includer);
  }

  /*  6. Create a poptContext */
  if (app->app_popt_flags_val.type == GNOME_ATTRIBUTE_INTEGER)
    popt_flags = app->app_popt_flags_val.u.integer_value;

  argctx =
    app->arg_context_val.u.pointer_value =
    poptGetContext(app->app_id, argc, argv,
		   (struct poptOption *)app->top_options_table->data,
		   popt_flags);
  
  app->arg_context_val.type = GNOME_ATTRIBUTE_POINTER;

  /* 7. Cleanup/return */
  app->state = APP_PREINIT_DONE;

  return argctx;
}

/**
 * gnome_program_parse_args:
 * @app: Application object
 *
 * Description: Parses the command line arguments for the application
 */
void
gnome_program_parse_args(GnomeProgram *app)
{
  int nextopt;
  poptContext ctx;

  g_return_if_fail(app);
  if (app->state != APP_PREINIT_DONE)
    return;

  ctx = app->arg_context_val.u.pointer_value;
  while((nextopt = poptGetNextOpt(ctx)) > 0
	|| nextopt == POPT_ERROR_BADOPT)
    /* do nothing */ ;

  if(nextopt != -1) {
    g_print("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n",
	   poptBadOption(ctx, 0),
	   poptStrerror(nextopt),
	   app->argv[0]);
    exit(1);
  }
}

/**
 * gnome_program_postinit:
 * @app: Application object
 *
 * Description: Called after gnome_program_parse_args(), this function
 * takes care of post-parse initialization and cleanup
 */
void
gnome_program_postinit(GnomeProgram *app)
{
  int i;
  GnomeModuleInfo *a_module;

  g_return_if_fail(app);

  if (app->state != APP_PREINIT_DONE)
    return;

  /* Call post-parse functions */
  for (i = 0; (a_module = g_ptr_array_index(app->modules, i)); i++)
    {
      if (a_module->post_args_parse)
	a_module->post_args_parse(app, a_module);
    }

  /* Free up stuff we don't need to keep around for the lifetime of the app */
#if 0
  g_ptr_array_free(app->modules, TRUE);
  app->modules = NULL;
#endif

  g_array_free(app->top_options_table, TRUE);
  app->top_options_table = NULL;

  g_blow_chunks(); /* Try to compact memory usage a bit */

  app->state = APP_POSTINIT_DONE;
}

/**
 * gnome_program_init:
 * @app_id: Application ID string
 * @app_version: Application version string
 * @argc: The number of commmand line arguments contained in 'argv'
 * @argv: A string array of command line arguments
 * @...: a NULL-terminated list of attribute name/value pairs
 *
 * Description:
 * Performs application initialization.
 */
void
gnome_program_init(const char *app_id, const char *app_version,
	      int argc, char **argv, ...)
{
  va_list args;

  va_start(args, argv);
  gnome_program_initv(app_id, app_version, argc, argv, args);
  va_end(args);
}

/* Yes, I know this one sort of counts as "duplication of code" over
 * gnome_program_inita(), but it's less code being duplicated here than *
 * there would be if I did that whole long loop through all 'args' to
 * build an 'attrs' array
 */
void
gnome_program_initv(const char *app_id, const char *app_version,
	       int argc, char **argv, va_list args)
{
  GnomeProgram *app = gnome_program_get();

  gnome_program_preinitv(app, app_id, app_version, argc, argv, args);
  gnome_program_parse_args(app);
  gnome_program_postinit(app);
}

void
gnome_program_inita(const char *app_id, const char *app_version,
	       int argc, char **argv, GnomeAttribute *attrs)
{
  GnomeProgram *app = gnome_program_get();

  gnome_program_preinita(app, app_id, app_version, argc, argv, attrs);
  gnome_program_parse_args(app);
  gnome_program_postinit(app);
}
