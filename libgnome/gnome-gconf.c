/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME Library - gnome-gconf.c
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/


#include <config.h>
#include <stdlib.h>

#define GCONF_ENABLE_INTERNALS 1
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
extern struct poptOption gconf_options[];

#include "gnome-i18nP.h"

#include <libgnome/libgnome.h>
#include "gnome-gconf.h"

gchar*
gnome_gconf_get_gnome_libs_settings_relative (const gchar *subkey)
{
        gchar *dir;
        gchar *key;

        dir = g_strconcat("/apps/gnome-settings/",
                          gnome_program_get_name(gnome_program_get()),
                          NULL);

        if (subkey && *subkey) {
                key = gconf_concat_dir_and_key(dir, subkey);
                g_free(dir);
        } else {
                /* subkey == "" */
                key = dir;
        }

        return key;
}

gchar*
gnome_gconf_get_app_settings_relative (const gchar *subkey)
{
        gchar *dir;
        gchar *key;

        dir = g_strconcat("/apps/",
                          gnome_program_get_name(gnome_program_get()),
                          NULL);

        if (subkey && *subkey) {
                key = gconf_concat_dir_and_key(dir, subkey);
                g_free(dir);
        } else {
                /* subkey == "" */
                key = dir;
        }

        return key;
}

/*
 * Our global GConfClient, and module stuff
 */

static GConfClient* global_client = NULL;

static void
gnome_gconf_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gconf_preinit(app, (GnomeModuleInfo*)mod_info);
}

static void
gnome_gconf_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
        gchar *settings_dir;
        
        gconf_postinit(app, (GnomeModuleInfo*)mod_info);

        global_client = gconf_client_get_default();

        gconf_client_add_dir(global_client,
                             "/desktop/gnome",
                             GCONF_CLIENT_PRELOAD_NONE, NULL);

        settings_dir = gnome_gconf_get_gnome_libs_settings_relative("");

        gconf_client_add_dir(global_client,
                             settings_dir,
                             /* Possibly we should turn preload on for this */
                             GCONF_CLIENT_PRELOAD_NONE,
                             NULL);
        g_free(settings_dir);
}

static GnomeModuleRequirement gnome_gconf_requirements[] = {
        /* VERSION is also our version note - it's all libgnomeui */
	{ VERSION, &gnome_bonobo_activation_module_info },
        { NULL, NULL }
};

GnomeModuleInfo gnome_gconf_module_info = {
        "gnome-gconf", VERSION, N_("GNOME GConf Support"),
        gnome_gconf_requirements,
	NULL /* instance init */,
        gnome_gconf_pre_args_parse,
	gnome_gconf_post_args_parse,
        gconf_options,
	NULL /* init_pass */,
	NULL /* class_init */,
	NULL, NULL /* expansions */
};
