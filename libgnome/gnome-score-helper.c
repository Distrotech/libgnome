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

#ifndef GNOME_SCORE_C
#include "gnome-defs.h"
#include "config.h"
#endif

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
	FILE *infile;
	gchar buf [128], *str, *tmp;
	gint  i, v;
	struct stat sbuf;
	ino_t procino, realino;
	dev_t procdev, realdev;

	snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
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
	
	snprintf(buf, sizeof(buf), "/proc/%d/exe", pid);
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
}

#ifndef GNOME_SCORE_C
struct ascore_t {
  gchar *username;
  time_t scoretime;
  gfloat score;
};

void
print_ascore(struct ascore_t *ascore, FILE *outfile)
{
	fprintf(outfile, "%f %ld %s\n", ascore->score, (long int)ascore->scoretime,
		ascore->username);
}

gint
log_score(gchar *progname, gchar *username, gfloat score)
{
	FILE *infile;
	FILE *outfile;
	gchar buf [512];
	GList *scores = NULL;
	gchar *name, *game_score_file;
	gfloat ascore;
	time_t atime;
	struct ascore_t *anitem;
	int i;
	gint retval = 1;
	GList *item;
	gint pos;
	gboolean we_are_top_ten = FALSE;

	game_score_file = g_copy_strings (SCORE_PATH "/", progname, ".scores", NULL);
	infile = fopen (game_score_file, "r");
	if(infile)
	{
		while(fgets(buf, sizeof(buf), infile))
		{
			i = strlen(buf) - 1; /* Chomp */
			while(isspace(buf[i])) buf[i--] = '\0';
			
			sscanf(buf, "%f %ld %as", &ascore, (long int *)&atime, &name);
			
			anitem = g_malloc(sizeof(struct ascore_t));
			anitem->score = ascore;
			anitem->username = name;
			anitem->scoretime = atime;
			scores = g_list_append(scores, (gpointer)anitem);
		}
		fclose(infile);
	}
	anitem = g_malloc(sizeof(struct ascore_t));
	anitem->score = score;
	anitem->username = username;
	anitem->scoretime = time(NULL);
	
	/* Certifiable spaghetti code. There has to be a nicer way to do this,
	   I'm sure */
	
	for(item = scores, pos = 0; item; item = item->next, pos++)
	{
		if(((struct ascore_t *)item->data)->score < anitem->score)
		{
			scores = g_list_insert(scores, anitem, pos);
			we_are_top_ten = TRUE;
			break;
		}
	}
 out_of_loop:
	if(we_are_top_ten)
	{
		scores = g_list_remove_link(scores, g_list_nth(scores, NSCORES));
		retval = 0;
	}
	
/* XXX TODO: set permissions etc. on this file... Need suid root though :( */
	umask(202);
	outfile = fopen(game_score_file, "w");
	{ 
		struct group *gent = getgrnam("games");
		if(gent)
			chown(buf, -1, gent->gr_gid);
	}
	g_free (game_score_file);
	
	if(outfile){
		g_list_foreach(scores, (GFunc)print_ascore, outfile);
		fclose(outfile);
	} else
		perror("gnome-score-helper");
	
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
#ifdef DEBUG
	int i;
	
	for(i = 0; i < argc; i++)
		g_print("%s: %s\n", argv[0], argv[i]);
#endif
	
	if(argc != 2)
		return 1;
	
	progname = gnome_get_program_name(getppid());
	if(progname == NULL)
		return 1;
	
	realfloat = atof(argv[1]);
	pwent = getpwuid(getuid());
	if(!pwent)
		return 1;
	
	username = pwent->pw_name;
	
	return log_score(progname, username, realfloat);
}
#endif
