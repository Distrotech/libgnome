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

static void
sample_read_from_file (GnomeSoundSample *gs, const gchar *name)
{
    int fd = open (name, O_RDONLY);

    if (fd >= 0) {
	char buffer[4096];
	int len;		

	do {
	    do
		len = read (fd, buffer, sizeof (buffer));
	    while (len < 0 && errno == EINTR);
	      
	    if (len > 0)
		gnome_sound_sample_write (gs, len, buffer, NULL);
	}

	while (len > 0);
	close (fd);
    }

    gnome_sound_sample_write_done (gs, NULL);
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

    gs = gnome_sound_sample_new ("gnome/warning", NULL);
    sample_read_from_file (gs, "/usr/share/sounds/phone.wav");
    gnome_sound_cache_add_sample (gs, NULL);
    gnome_sound_sample_release (gs, NULL);

    gs = gnome_sound_sample_new ("test/one", NULL);
    sample_read_from_file (gs, "/usr/share/sounds/panel/slide.wav");
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
