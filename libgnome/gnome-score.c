#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <glib.h>

#include "gnome-defs.h"
#include "gnome-score.h"

#define GNOME_SCORE_C 1
#include "gnome-score-helper.c"
#undef GNOME_SCORE_C

gint
gnome_score_log(gfloat score,
		gchar *level,
		gboolean higher_to_lower_score_order)
{
  int exitstatus;
  char buf[64];
  char buf2[64];

  pid_t childpid = fork();

  if(childpid == 0)
    {
      /* We are the child */
      g_snprintf(buf, sizeof(buf), "%f", score);
      g_snprintf(buf2, sizeof(buf2), "%d", higher_to_lower_score_order);
      execlp("gnome-score-helper", "gnome-score-helper",
	     buf, level?level:"", buf2, NULL);
      _exit(99);
    }

  waitpid(childpid, &exitstatus, 0);
  if(WIFEXITED(exitstatus) && (WEXITSTATUS(exitstatus) != -1) )
    return (WEXITSTATUS(exitstatus));
  else
    {
      g_warning("Scorekeeping failed\n");
      return 0;
    }
}

gint
gnome_score_get_notable(gchar *gamename,
			gchar *level,
			gchar ***names,
			gfloat **scores,
			time_t **scoretimes)
{
  gchar *realname, buf[512], *buf2;
  gchar *infile_name;
  FILE *infile;
  gint retval;

  g_return_val_if_fail(names != NULL, 0);
  g_return_val_if_fail(scores != NULL, 0);

  if (gamename == NULL)
    realname = gnome_get_program_name(getpid());
  else
    realname = g_strdup(gamename);
  if(!realname)
    return -1;

  infile_name = gnome_get_score_file_name(realname, level);

  infile = fopen(infile_name, "r");
  g_free(infile_name);
  g_free(realname);

  if(infile)
    {
      *names = g_malloc((NSCORES + 1) * sizeof(gchar*));
      *scores = g_malloc((NSCORES + 1) * sizeof(gfloat));
      *scoretimes = g_malloc((NSCORES + 1) * sizeof(time_t));
      
      for(retval = 0;
	  fgets(buf, sizeof(buf), infile) && retval < NSCORES; 
	  retval++)
	{ 
	  buf[strlen(buf)-1]=0;
	  buf2 = strtok(buf, " ");
	  (*scores)[retval] = atof(buf2);
	  buf2 = strtok(NULL, " ");
	  (*scoretimes)[retval] = atoi(buf2);
	  buf2 = strtok(NULL, "\n");
	  (*names)[retval] = g_strdup(buf2);
	}
      (*names)[retval] = NULL;
      (*scores)[retval] = 0.0;
      fclose(infile);
    }
  else
    {
      *names = NULL;
      *scores = NULL;
      retval = 0;
    }

  return retval;
}
