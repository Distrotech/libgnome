#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <libgnome/libgnome-init.h>
#include <libgnome/gnome-sound.h>

int
main (int argc, char **argv)
{
    GnomeProgram *program;

    program = gnome_program_init ("test-sound", VERSION,
				  &libgnome_module_info,
				  argc, argv, NULL);

    gnome_sound_init ("csl", NULL, NULL, NULL);

    gnome_sound_play ("gameover.wav", NULL);

    sleep(5);

    gnome_sound_shutdown (NULL);

    return 0;
}
