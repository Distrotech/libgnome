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
gnome_score_log(gfloat score)
{
  int exitstatus;
  char buf[64];
  pid_t childpid = fork();

  if(childpid == 0)
    {
      /* We are the child */
      snprintf(buf, sizeof(buf), "%f", score);
      execlp("gnome-score-helper", "gnome-score-helper",
	     buf, NULL);
      exit(99);
    }

  waitpid(childpid, &exitstatus, 0);
  if(WIFEXITED(exitstatus))
    return (WEXITSTATUS(exitstatus)==0)?TRUE:FALSE;
  else
    {
      g_warning("Scorekeeping failed\n");
      return FALSE;
    }
}

gint
gnome_score_get_notable(gchar *gamename,
			gchar ***names,
			gfloat **scores,
			time_t **scoretimes)
{
  gchar *realname, buf[512];
  FILE *infile;
  gint retval;

  g_return_val_if_fail(names != NULL, 0);
  g_return_val_if_fail(scores != NULL, 0);

  if(realname == NULL)
    realname = gnome_get_program_name(getpid());
  else
    realname = g_strdup(gamename);
  if(!realname)
    return 0;

  snprintf(buf, sizeof(buf), "%s/%s.scores", SCORE_PATH, realname);

  infile = fopen(buf, "r");
  if(infile)
    {
      for(retval = 0, *names = NULL, *scores = NULL;
	  fgets(buf, sizeof(buf), infile); retval++) {
	*names = g_realloc(*names, retval * sizeof(gchar *));
	*scores = g_realloc(*scores, retval * sizeof(gfloat));
	*scoretimes = g_realloc(*scores, retval * sizeof(time_t));
	sscanf(buf, "%f %d %a",
	       &((*scores)[retval]),
	       &((*scoretimes)[retval]),
	       &((*names)[retval]));
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
