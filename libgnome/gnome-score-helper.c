/*
 * By Elliot Lee.

 * Miguel suggested this scheme for verifying a program's
 * identity, as I was going to do some complicated md5-ing of binaries ;-)
 * It even works properly (although it would be nice if we could just
 * do inode_to_pathname() or something, it would be so much easier
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <locale.h>
#include <glib.h>

#ifndef GNOME_SCORE_C
#include "gnome-defs.h"
#include "config.h"
#endif

#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"

#ifndef GNOMEBINDIR
#define GNOMEBINDIR "/usr/local/bin"
#endif

#ifndef GNOMELOCALSTATEDIR
#define GNOMELOCALSTATEDIR "/var/lib"
#endif

#define SCORE_PATH GNOMELOCALSTATEDIR "/games"

#ifndef NSCORES
#define NSCORES 10
#endif

static gchar *
gnome_get_program_name(gint pid)
{
#ifdef __linux__
	FILE *infile;
	gchar buf [128], *str, *tmp;
	gint  i, v;
	struct stat sbuf;
	ino_t procino, realino;
	dev_t procdev, realdev;

	g_snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
	infile = fopen(buf, "r");
	
	if(infile){
		fgets(buf, sizeof(buf), infile);
		fclose(infile);
	} else
		return NULL;

	for(i = 0; buf[i] && !isspace(buf[i]); i++)
		/* */ ;
	buf[i] = '\0';
	tmp = strrchr(buf, '/');
	if(tmp == NULL)
		tmp = g_strdup(buf);
	else
		tmp = g_strdup(tmp + 1);
	
	str = g_concat_dir_and_file (GNOMEBINDIR, tmp);
	v = stat (str, &sbuf);
	g_free (str);
	if (v){
		g_free(tmp);
		return NULL;
	}
	realino = sbuf.st_ino;
	realdev = sbuf.st_dev;
	
	g_snprintf(buf, sizeof(buf), "/proc/%d/exe", pid);
	if(stat(buf, &sbuf)){
		g_free(tmp);
		return NULL;
	}
	procino = sbuf.st_ino;
	procdev = sbuf.st_dev;
	
	if(procino != realino || procdev != realdev){
		g_free(tmp);
		return NULL;
	} else
		return tmp;
#else
	g_warning("gnome_get_program_name: this function is not yet implemented for non-linux systems!");
	return NULL;
#endif
}

struct ascore_t {
	gchar *username;
	time_t scoretime;
	gfloat score;
};

GList *
read_scores(gchar *game_score_section)
{
	GList *scores = NULL;
	struct ascore_t *anitem;
	gchar *key;
	gchar str_i[5];
	gchar *str_score;
	gint nscores;
	gint i;

	key = g_copy_strings (game_score_section,
				"Scores=0",
				NULL);
	nscores = gnome_config_get_int (key);
	g_free (key);

	for ( i = 0; i < nscores; i++ ) 
	{
		g_snprintf (str_i, 5, "%d", i);
		anitem = g_malloc(sizeof(struct ascore_t));

		key = g_copy_strings (game_score_section,
					"User.", str_i, "=", NULL);
		anitem->username = gnome_config_get_string (key);
		g_free (key);
		
		key = g_copy_strings (game_score_section,
					"Score.", str_i, "=0.0", NULL);
		str_score = gnome_config_get_string (key);
		anitem->score = atof(str_score);
		g_free (str_score);
		g_free (key);
		
		key = g_copy_strings (game_score_section,
					"ScoreTime.", str_i, "=0", NULL);
		(long int) anitem->scoretime = gnome_config_get_int (key);
		g_free (key);
		
		scores = g_list_append(scores, (gpointer)anitem);
	}
	gnome_config_sync();
	return (scores);
}

#ifndef GNOME_SCORE_C
void
write_ascore(struct ascore_t *ascore, 
	     gchar *game_score_section)
{
	static int pos = 0;
	gchar str_pos[5];
	gchar buf[20];
	gchar *key;

	g_snprintf (str_pos, 5, "%d", pos);

	key = g_copy_strings (game_score_section,
				"User.", str_pos, NULL);
	gnome_config_set_string (key, ascore->username);
	g_free (key);
		
	key = g_copy_strings (game_score_section,
				"Score.", str_pos, NULL);
	g_snprintf (buf, 20, "%f", ascore->score);
	gnome_config_set_string (key, buf);
	g_free (key);
		
	key = g_copy_strings (game_score_section,
				"ScoreTime.", str_pos, NULL);
	gnome_config_set_int (key, (long int) ascore->scoretime);
	g_free (key);
		
	pos++;
}

gint
log_score(gchar *progname, 
          gchar *level, 
          gchar *username, 
          gfloat score, 
          int ordering)
{
	GList *scores, *anode;
	gchar *name, *game_score_section;
	gchar *key;
	struct ascore_t *anitem, *curscore;
	gint retval;
	gint pos;

	game_score_section = g_copy_strings ("=" SCORE_PATH "/", 
				       progname, ".scores=", "/",
				       level ? level : "Top Ten",
				       "/", NULL);

	scores = read_scores (game_score_section);

	anitem = g_malloc(sizeof(struct ascore_t));
	anitem->score = score;
	anitem->username = username;
	anitem->scoretime = time(NULL);

	for(pos = 0, anode = scores;
	    pos < NSCORES && anode;
	    pos++, anode = anode->next)
	{
		curscore = anode->data;
		if(ordering)
		{
			if(curscore->score < anitem->score)
			{
				break;
			}
		}
		else
		{
			if(curscore->score > anitem->score)
			{
				break;
			}
		}
	}

	if(pos < NSCORES)
	{
		scores = g_list_insert(scores, anitem, pos);
		scores = g_list_remove_link(scores, g_list_nth(scores, NSCORES));
		retval = pos+1;
	}
	else
		retval = 0;
	
	gnome_config_sync(); 
	key = g_copy_strings (game_score_section,
				"Scores", NULL);
	gnome_config_set_int (key, g_list_length (scores) );
	g_free (key);
	g_list_foreach(scores, (GFunc)write_ascore, game_score_section);
	gnome_config_sync();
	
	/* There's a memory leak here, of course - we don't free anything - but
	 * this program dies quickly so it doesn't matter
	 */
	return retval;
}

int
main(int argc, char *argv[])
{
	gchar *username;
	gfloat realfloat;
	struct passwd *pwent;
	gchar *progname;
	gchar *level;
	gboolean ordering;

#ifdef DEBUG
{
	int i;
	
	for(i = 0; i < argc; i++)
		g_print("%s: %s\n", argv[0], argv[i]);
}
#endif
	
	if(argc != 4){
		fprintf (stderr, "Internal GNOME program, usage: gnome-score-helper value username ordering\n");
		return 0;
	}

	setlocale(LC_ALL, "");

	progname = gnome_get_program_name(getppid());
	if(progname == NULL)
		return 0;

	realfloat = atof(argv[1]);
	ordering = atoi(argv[3]);
	pwent = getpwuid(getuid());
	if(!pwent)
		return 0;
	
	username = pwent->pw_gecos;
	level = argv[2];
	if(argv[2][0] == '\0')
		level = NULL;

	return log_score(progname, level, username, realfloat, ordering);
}
#endif /* !GNOME_SCORE_C */
