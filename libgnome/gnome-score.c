/*
 * gnome-score.c
 * originally by Elliot Lee, subsequently bashed around by Nathan Bryant
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <pwd.h>
#include <ctype.h>

#include "gnome-defs.h"
#include "gnome-score.h"
#include "gnome-util.h"

#ifndef NSCORES
#define NSCORES 10
#endif

#define SCORE_PATH GNOMELOCALSTATEDIR "/games"

struct command
{
   gfloat score;
   int level;			/* length of level arg to gnome_score_log
				 * including null term */
   gboolean ordering;
};

struct ascore_t
{
   gchar *username;
   time_t scoretime;
   gfloat score;
};

static int outfd = -1, infd = -1;
static const gchar *defgamename;

/********************** internal functions ***********************************/

static gchar *
gnome_get_score_file_name (const gchar * progname, const gchar * level)
{
   if (level)
     return g_copy_strings (SCORE_PATH "/",
			    progname, ".", level, ".scores", NULL);
   else
     return g_copy_strings (SCORE_PATH "/",
			    progname, ".scores", NULL);
}

static void
print_ascore (struct ascore_t *ascore, FILE * outfile)
{
   fprintf (outfile, "%f %ld %s\n", ascore->score,
	    (long int) ascore->scoretime, ascore->username);
}

static void
free_ascore(struct ascore_t *data)
{
   g_free(data->username);
   g_free(data);
}

static gint
log_score (const gchar * progname, const gchar * level, gchar * username,
	   gfloat score, int ordering)
{
   FILE *infile;
   FILE *outfile;
   gchar buf[512], *buf2;
   GList *scores = NULL, *anode;
   gchar *name, *game_score_file;
   gfloat ascore;
   time_t atime;
   struct ascore_t *anitem, *curscore;
   int i;
   gint retval = 1;
   gint pos;
   
   game_score_file = gnome_get_score_file_name (progname, level);
   
   infile = fopen (game_score_file, "r");
   if (infile)
     {
	while (fgets (buf, sizeof (buf), infile))
	  {
	     i = strlen (buf) - 1;	/* Chomp */
	     while (isspace (buf[i]))
	       buf[i--] = '\0';
	     
	     buf2 = strtok (buf, " ");
	     ascore = atof (buf2);
	     buf2 = strtok (NULL, " ");
	     (long int) atime = atoi (buf2);
	     buf2 = strtok (NULL, "\n");
	     name = strdup (buf2);
	     
	     anitem = g_malloc (sizeof (struct ascore_t));
	     anitem->score = ascore;
	     anitem->username = name;
	     anitem->scoretime = atime;
	     scores = g_list_append (scores, (gpointer) anitem);
	  }
	fclose (infile);
     }
   anitem = g_malloc (sizeof (struct ascore_t));
   anitem->score = score;
   anitem->username = g_strdup(username);
   anitem->scoretime = time (NULL);
   
   for (pos = 0, anode = scores;
	pos < NSCORES && anode;
	pos++, anode = anode->next)
     {
	curscore = anode->data;
	if (ordering)
	  {
	     if (curscore->score < anitem->score)
	       break;
	  }
	else
	  {
	     if (curscore->score > anitem->score)
	       break;
	  }
     }
   
   if (pos < NSCORES)
     {
	scores = g_list_insert (scores, anitem, pos);
	if ((anode = g_list_nth (scores, NSCORES)))
	  {
	     free_ascore (anode->data);
	     scores =
	       g_list_remove_link (scores, g_list_nth (scores, NSCORES));
	  }
	retval = pos + 1;
     }
   else
     retval = 0;
   
   /* we dont create the file; it must already exist */
   truncate (game_score_file, 0);
   outfile = fopen (game_score_file, "r+");
   
   if (outfile)
     {
	g_list_foreach (scores, (GFunc) print_ascore, outfile);
	fclose (outfile);
     }
   else
     perror (game_score_file);

   g_free (game_score_file);
   
   g_list_foreach (scores, (GFunc) free_ascore, NULL);
   g_list_free (scores);
   
   return retval;
}

static int
gnome_score_child (void)
{
   struct command cmd;
   gchar *level;
   struct passwd *pwd;
   gint retval;
#ifdef HAVE_SETFSGID
   gid_t gid;
   
   gid = getegid ();
   setgid (getgid ());
   setfsgid (gid);
#endif
   pwd = getpwuid (getuid ());
   strtok(pwd->pw_gecos, ",");
   while (read (STDIN_FILENO, &cmd, sizeof cmd) == sizeof cmd)
     {
	level = g_new (char, cmd.level);
	if (read (STDIN_FILENO, level, cmd.level) != cmd.level)
	  return EXIT_FAILURE;
	if (!*level) {
	   g_free(level);
	   level = NULL;
	}
	retval = log_score (defgamename, level, pwd->pw_gecos, cmd.score,
			    cmd.ordering);
	if (write(STDOUT_FILENO, &retval, sizeof retval) != sizeof retval)
	  return EXIT_FAILURE;
	if (level)
	  g_free(level);
     }
   return EXIT_SUCCESS;
}

static void 
drop_perms (void)
{
   gid_t gid = getegid ();
   
   setregid (getgid (), getgid ());	/* on some os'es (eg linux) this
					 * incantation will also drop the
					 * saved gid */
   /* see if we can set it back -- if we can, saved id wasnt dropped */
   if (!setgid (gid))
     {
	g_warning ("losing saved gid implementation detected, "
		   "get a real OS :)\n");
	setgid (getgid ());
     }
}

/*********************** external functions **********************************/

/*
 * gnome_score_init()
 * creates a child process with which we communicate through a pair of pipes,
 * then drops privileges.
 * this should be called as the first statement in main().
 * returns 0 on success, drops privs and returns -1 on failure
 */

gint
gnome_score_init (const gchar * gamename)
{
   int inpipe[2], outpipe[2];
   
   if (!gamename)
     gamename = "";
   if (!(defgamename = g_strdup (gamename)) ||
       pipe(inpipe))
     {
	drop_perms();
	return -1;
     }
   if (pipe (outpipe))
     {
	close (inpipe[0]);
	close (inpipe[1]);
	drop_perms ();
	return -1;
     }
   outfd = outpipe[1];
   infd = inpipe[0];
   switch (fork ())
     {
      case 0:
	if (dup2 (outpipe[0], STDIN_FILENO) == -1 ||
	    dup2 (inpipe[1], STDOUT_FILENO) == -1)
	  exit (EXIT_FAILURE);
	close(inpipe[0]);
	close(inpipe[1]);
	close(outpipe[0]);
	close(outpipe[1]);
	exit (gnome_score_child ());
      case -1:
	close (inpipe[0]);
	close (inpipe[1]);
	close (outpipe[0]);
	close (outpipe[1]);
	infd = outfd = -1;
	drop_perms ();
	return -1;
     }
   close(outpipe[0]);
   close(inpipe[1]);
   drop_perms ();
   return 0;
}

gint
gnome_score_log (gfloat score,
		 gchar * level,
		 gboolean higher_to_lower_score_order)
{
   struct command cmd;
   gint retval;
   
   if (getgid () != getegid ())
     {
	g_error ("gnome_score_init must be called first thing in main()\n");
	abort ();
     }
   if (infd == -1 || outfd == -1)
     return 0;
   
   cmd.score = score;
   if (!level)
     level = "";
   cmd.level = strlen (level) + 1;
   cmd.ordering = higher_to_lower_score_order;
   
   if (write (outfd, &cmd, sizeof cmd) != sizeof cmd ||
       write (outfd, level, cmd.level) != cmd.level ||
       read (infd, &retval, sizeof retval) != sizeof retval)
     {
	close (outfd);
	close (infd);
	infd = outfd = -1;
	return 0;
     }
   return retval;
}

gint
gnome_score_get_notable (gchar * gamename,
			 gchar * level,
			 gchar *** names,
			 gfloat ** scores,
			 time_t ** scoretimes)
{
   const gchar *realname;
   gchar buf[512], *buf2;
   gchar *infile_name;
   FILE *infile;
   gint retval;
   
   g_return_val_if_fail (names != NULL, 0);
   g_return_val_if_fail (scores != NULL, 0);
   
   if (gamename == NULL)
     realname = defgamename;
   else
     realname = gamename;
   
   infile_name = gnome_get_score_file_name (realname, level);
   
   infile = fopen (infile_name, "r");
   g_free (infile_name);
   
   if (infile)
     {
	*names = g_malloc ((NSCORES + 1) * sizeof (gchar *));
	*scores = g_malloc ((NSCORES + 1) * sizeof (gfloat));
	*scoretimes = g_malloc ((NSCORES + 1) * sizeof (time_t));
	
	for (retval = 0;
	     fgets (buf, sizeof (buf), infile) && retval < NSCORES;
	     retval++)
	  {
	     buf[strlen (buf) - 1] = 0;
	     buf2 = strtok (buf, " ");
	     (*scores)[retval] = atof (buf2);
	     buf2 = strtok (NULL, " ");
	     (*scoretimes)[retval] = atoi (buf2);
	     buf2 = strtok (NULL, "\n");
	     (*names)[retval] = g_strdup (buf2);
	  }
	(*names)[retval] = NULL;
	(*scores)[retval] = 0.0;
	fclose (infile);
     }
   else
     {
	*names = NULL;
	*scores = NULL;
	retval = 0;
     }
   return retval;
}
