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
    GnomeProgram *program;
    GnomeSoundSample *gs;

    program = gnome_program_init ("test-sound", VERSION,
				  &libgnome_module_info,
				  argc, argv, NULL);

    gnome_sound_init ("csl", NULL, NULL, NULL);

    gs = gnome_sound_sample_new_from_file ("gnome/warning",
					   "/usr/share/sounds/phone.wav",
					   NULL);
    gnome_sound_cache_add_sample (gs, NULL);
    gnome_sound_sample_release (gs, NULL);

    gs = gnome_sound_sample_new_from_file ("test/one",
					   "/usr/share/sounds/panel/slide.wav",
					   NULL);
    gnome_sound_cache_add_sample (gs, NULL);
    gnome_sound_sample_release (gs, NULL);

    trig.type = GTRIG_FUNCTION;
    trig.u.function = sample_trigger_function;
    trig.level = NULL;

    gnome_triggers_init ();
    gnome_triggers_add_trigger (&trig, "test", "one", NULL);
    gnome_triggers_do ("Test of direct hit", "warning", "test", "one", NULL);

    sleep(5);

    gnome_sound_shutdown (NULL);

    return 0;
}
