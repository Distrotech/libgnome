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
  GList *l_scores, *anode;
  struct ascore_t *curscore;
  gchar *realname;
  gchar *game_score_section;
  gint retval;
  gint i;

  g_return_val_if_fail(names != NULL, 0);
  g_return_val_if_fail(scores != NULL, 0);

  if (gamename == NULL)
    realname = gnome_get_program_name(getpid());
  else
    realname = g_strdup(gamename);
  if(!realname)
    return -1;

  game_score_section = g_copy_strings ("=" SCORE_PATH "/",
	realname, ".scores=", "/",
	level ? level : "Top Ten",
	"/", NULL);
/*  l_scores = read_scores (game_score_section); */
  g_free(realname);
  g_free(game_score_section);

  retval = g_list_length (l_scores);

  if(retval)
  {
  	*names = g_malloc((retval+1) * sizeof(gchar*));
  	*scores = g_malloc((retval+1) * sizeof(gfloat));
  	*scoretimes = g_malloc((retval+1) * sizeof(time_t));

	for(i = 0, anode = l_scores;
            i < retval && anode;
            i++, anode = anode->next)
        {
                curscore = anode->data;
		(*names)[i]      = curscore->username;
		(*scores)[i]     = curscore->score;
		(*scoretimes)[i] = curscore->scoretime;
        }
        g_list_free (l_scores);

	(*names)[retval] = NULL;
	(*scores)[retval] = 0.0;
	(*scoretimes)[retval] = 0;
  }
  else 
  {
  	*names = NULL;
  	*scores = NULL;
  	*scoretimes = NULL;
  	retval = 0;
  }

  return retval;
}
