#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <libgnome/gnome-init.h>
#include <libgnome/gnome-sound.h>
#include <libgnome/gnome-triggers.h>

int global_cntr = 0;

static void
sample_trigger_function(char *msg, char *level, char *supinfo[])
{
    int i;
    for(i = 0; supinfo[i]; i++)
	g_print("%s ", supinfo[i]);
    g_print("[%s] %s\n", level, msg);
    fflush(stdout);

    global_cntr++;
}

int
main (int argc, char **argv)
{
    struct _GnomeTrigger trig;
    GnomeSoundDriver *driver;
    GnomeProgram *program;
    GnomeSoundSample *gs;

    program = gnome_program_init ("test-sound", VERSION,
				  &libgnome_module_info,
				  argc, argv, NULL);

    driver = gnome_sound_init ("csl", NULL, NULL);

    gs = gnome_sound_driver_sample_new_from_file (driver, "gnome/warning",
						  "/usr/share/sounds/phone.wav",
						  NULL);
    gnome_sound_driver_cache_add_sample (driver, gs, NULL);
    gnome_sound_driver_sample_release (driver, gs, NULL);

    gs = gnome_sound_driver_sample_new_from_file (driver, "test/one",
						  "/usr/share/sounds/panel/slide.wav",
						  NULL);
    gnome_sound_driver_cache_add_sample (driver, gs, NULL);
    gnome_sound_driver_sample_release (driver, gs, NULL);

    trig.type = GTRIG_FUNCTION;
    trig.u.function = sample_trigger_function;
    trig.level = NULL;

    gnome_triggers_init ();
    gnome_triggers_add_trigger (&trig, "test", "one", NULL);
    gnome_triggers_do ("Test of direct hit", "warning", "test", "one", NULL);

    g_object_unref (G_OBJECT (driver));

    sleep(5);

    return 0;
}
