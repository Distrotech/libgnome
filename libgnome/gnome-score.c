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

gboolean /* Returns TRUE if it was a notable (top ten) score */
gnome_score_log(gfloat score, gchar *level, int ordering)
{
  int exitstatus;
  char buf[64];
  char buf2[64];
  pid_t childpid = fork();

  if(childpid == 0)
    {
      /* We are the child */
      snprintf(buf, sizeof(buf), "%f", score);
      snprintf(buf2, sizeof(buf2), "%d", ordering);
      execlp("gnome-score-helper", "gnome-score-helper",
	     buf, level, buf2, NULL);
      exit(99);
    }

  waitpid(childpid, &exitstatus, 0);
  if(WIFEXITED(exitstatus) && (WEXITSTATUS(exitstatus) != -1) )
    return (WEXITSTATUS(exitstatus));
  else
    {
      g_warning("Scorekeeping failed\n");
      return FALSE;
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
  FILE *infile;
  gint retval;

  g_return_val_if_fail(names != NULL, 0);
  g_return_val_if_fail(scores != NULL, 0);

  if(realname == NULL)
    realname = gnome_get_program_name(getpid());
  else
    realname = g_strdup(gamename);
  if(!realname)
    return -1;

  snprintf(buf, sizeof(buf), "%s/%s.%s.scores", SCORE_PATH, realname, level);

  infile = fopen(buf, "r");
  if(infile)
    {
    *names = g_malloc(NSCORES*sizeof(gchar*));
    *scores = g_malloc(NSCORES*sizeof(gfloat));
    *scoretimes = g_malloc(NSCORES*sizeof(time_t));

    for(retval = 0; fgets(buf, sizeof(buf), infile) && retval < NSCORES; 
    							     retval++) { 
	buf[strlen(buf)-1]=0;
	buf2 = strtok(buf, " ");
	(*scores)[retval] = atof(buf2);
	buf2 = strtok(NULL, " ");
	(*scoretimes)[retval] = atoi(buf2);
	buf2 = strtok(NULL, "\n");
	(*names)[retval] = strdup(buf2);
      }
      fclose(infile);
    }
  else
    {
      *names = NULL;
      *scores = NULL;
      retval = 0;
    }

  g_free(realname);

  return retval;
}
