#include "config.h"

/* By Elliot Lee */

#include "gnome-triggers.h"
#include "gnome-config.h"
#include "gnome-util.h"
#include "gnome-string.h"
#include "gnome-sound.h"
#ifdef HAVE_LIBESD
#include <esd.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* TYPE DECLARATIONS */

typedef struct _TriggerList * TriggerList;

struct _TriggerList {
  char *nodename;
  TriggerList *subtrees;
  gint numsubtrees;
  GnomeTrigger *actions;
  gint numactions;
};

typedef void (*GnomeTriggerTypeFunction)(GnomeTrigger t, char *msg, char *level, char *supinfo[]);

/* PROTOTYPES */
static GnomeTrigger
gnome_trigger_dup(GnomeTrigger dupme);
static TriggerList
gnome_triggerlist_new(char *nodename);
static void
gnome_triggerlist_free(TriggerList t);
static void
gnome_trigger_free(GnomeTrigger t);
static void
gnome_trigger_do(GnomeTrigger t, const char *msg, const char *level,
		 const char *supinfo[]);
static void
gnome_trigger_do_function(GnomeTrigger t,
			  const char *msg, const char *level,
			  const char *supinfo[]);
static void
gnome_trigger_do_command(GnomeTrigger t,
			 const char *msg, const char *level,
			 const char *supinfo[]);
static void
gnome_trigger_do_mediaplay(GnomeTrigger t,
			   const char *msg,
			   const char *level,
			   const char *supinfo[]);

/* FILEWIDE VARIABLES */

static TriggerList topnode = NULL;
static int trigger_msg_sample_ids[] = {
  -1, /* info */
  -1, /* warning */
  -1, /* error */
  -1, /* question */
  -1, /* generic */
};

static GnomeTriggerTypeFunction actiontypes[] =
/* This list should have entries for all the trigger types in
   gnome-triggers.h */
{
  (GnomeTriggerTypeFunction)NULL,
  gnome_trigger_do_function,
  gnome_trigger_do_command,
  gnome_trigger_do_mediaplay,
  (GnomeTriggerTypeFunction)NULL
};

/* IMPLEMENTATIONS */
void
gnome_triggers_init(void)
{
  char *fn;
  char *val;
  int n;

  fn = gnome_datadir_file("gnome/triggers/list");
  if(fn) {
    gnome_triggers_readfile(fn);
    g_free(fn);
  }

  fn = gnome_util_home_file("triggers/list");
  if(fn) {
    gnome_triggers_readfile(fn);
    g_free(fn);
  }

#ifdef HAVE_LIBESD
  val = gnome_config_get_string("/sounds/standard events/info");
  if(val)
    trigger_msg_sample_ids[0] = gnome_sound_sample_load("info", val);
  g_free(val);

  val = gnome_config_get_string("/sounds/standard events/warning");
  if(val)
    trigger_msg_sample_ids[1] = gnome_sound_sample_load("warning", val);
  g_free(val);

  val = gnome_config_get_string("/sounds/standard events/error");
  if(val)
    trigger_msg_sample_ids[2] = gnome_sound_sample_load("error", val);
  g_free(val);

  val = gnome_config_get_string("/sounds/standard events/question");
  if(val)
    trigger_msg_sample_ids[3] = gnome_sound_sample_load("question", val);
  g_free(val);

  val = gnome_config_get_string("/sounds/standard events/generic");
  if(val)
    trigger_msg_sample_ids[4] = gnome_sound_sample_load("generic", val);
  g_free(val);
#endif
}

gint
gnome_triggers_readfile(gchar *infilename)
{
  GnomeTrigger nt;
  char aline[512];
  char **subnames = NULL;
  char **parts = NULL;
  FILE *infile;
  int i;

  infile = fopen(infilename, "r");
  if(infile == NULL)
    return 1;

  nt = gnome_trigger_dup(NULL);  
  while(fgets(aline, sizeof(aline), infile)) {
    i = strlen(aline) - 1;
    while(isspace(aline[i])) aline[i--] = '\0';

    if(aline[0] == '\0' || aline[0] == '#')
      continue;

    parts = gnome_string_split(aline, " ", -1);
    if(!parts || !parts[0] || !parts[1] || !parts[2] || !parts[3]) {
      gnome_string_array_free(parts);
      g_warning("Invalid triggers line \'%s\'\n", aline);
      continue;
    }

    if(!strcmp(parts[1], "NULL")) {
      subnames = g_malloc(sizeof(gchar *));
      subnames[0] = NULL;
    } else
      subnames = gnome_string_split(parts[1], ":", -1);

    if(!strcmp(parts[2], "command"))
      nt->type = GTRIG_COMMAND;
    else if(!strcmp(parts[2], "play"))
      nt->type = GTRIG_MEDIAPLAY;
    nt->u.command = parts[3];
    if(!strcmp(parts[0], "NULL"))
      nt->level = NULL;
    else
      nt->level = parts[0];
    gnome_triggers_vadd_trigger(nt, subnames);

    gnome_string_array_free(subnames);
    gnome_string_array_free(parts);
  }
  g_free(nt);

  return 0;
}

void gnome_triggers_add_trigger(GnomeTrigger nt, ...)
{
  va_list l;
  gint nstrings, i;
  gchar **strings;
  
  /* Count number of strings */
  
  va_start(l, nt);
  for (nstrings = 0; va_arg(l, gchar *); nstrings++);
  va_end(l);
  
  /* Build list */
  
  strings = g_new(gchar *, nstrings + 1);
  
  va_start(l, nt);
  
  for (i = 0; i < nstrings; i++)
    strings[i] = va_arg(l, gchar *);
  strings[i] = NULL;
  
  va_end(l);
  
  /* And pass them to the real function */
  
  gnome_triggers_vadd_trigger(nt, strings);
  g_free(strings);
}

static GnomeTrigger
gnome_trigger_dup(GnomeTrigger dupme)
{
  GnomeTrigger retval;
  retval = g_malloc(sizeof(struct _GnomeTrigger));
  if(dupme) {
    *retval = *dupme;
    if(dupme->level)
      retval->level = g_strdup(dupme->level);
    else
      retval->level = NULL;
    switch(retval->type) {
    case GTRIG_COMMAND:
      retval->u.command = g_strdup(dupme->u.command);
      break;
    default:
      break;
    }
  } else {
    retval->level = NULL;
    retval->type = GTRIG_NONE;
    memset(&retval->u, 0, sizeof(retval->u));
  }
  return retval;
}

static TriggerList
gnome_triggerlist_new(char *nodename)
{
  TriggerList retval;
  retval = g_malloc0(sizeof(struct _TriggerList));
  retval->nodename = g_strdup(nodename);
  return retval;
}

void gnome_triggers_vadd_trigger(GnomeTrigger nt,
				 char *supinfo[])
{
  g_return_if_fail(nt != NULL);
  if(!topnode)
    topnode = gnome_triggerlist_new(NULL);

  if(supinfo == NULL || supinfo[0] == NULL) {
    topnode->actions = g_realloc(topnode->actions, ++topnode->numactions);
    topnode->actions[topnode->numactions - 1] = gnome_trigger_dup(nt);
  } else {
    int i, j;
    TriggerList curnode;

    for(i = 0, curnode = topnode;
	supinfo[i]; i++) {
      for(j = 0;
	  j < curnode->numsubtrees
	    && strcmp(curnode->subtrees[j]->nodename, supinfo[i]);
	  j++) /* Do nothing */ ;

      if(j < curnode->numsubtrees) {
	curnode = curnode->subtrees[j];
      } else {
	curnode->subtrees = g_realloc(curnode->subtrees,
				      ++curnode->numsubtrees
				      * sizeof(TriggerList));
	curnode->subtrees[curnode->numsubtrees - 1] =
	  gnome_triggerlist_new(supinfo[i]);
	curnode = curnode->subtrees[curnode->numsubtrees - 1];
      } /* end for j */
    } /* end for i */

    curnode->actions = g_realloc(curnode->actions,
				 ++curnode->numactions
				 * sizeof(GnomeTrigger));
    curnode->actions[curnode->numactions - 1] = gnome_trigger_dup(nt);
  } /* end if */
}

void
gnome_triggers_do(const char *msg, const char *level, ...)
{
  va_list l;
  gint nstrings, i;
  gchar **strings;
  
  /* Count number of strings */
  va_start(l, level);
  for (nstrings = 0; va_arg(l, gchar *); nstrings++);
  va_end(l);
  
  /* Build list */
  
  strings = g_new(gchar *, nstrings + 1);
  
  va_start(l, level);
  
  for (i = 0; i < nstrings; i++)
    strings[i] = va_arg(l, gchar *);
  strings[i] = NULL;
  
  va_end(l);
  
  /* And pass them to the real function */
  
  gnome_triggers_vdo(msg, level, strings);
  g_free(strings);
}

void
gnome_triggers_vdo(const char *msg, const char *level, const char *supinfo[])
{
  TriggerList curnode = topnode;
  int i, j;

#ifdef HAVE_LIBESD
  int level_num = -1;

  if(!strcmp(level, "info")) level_num = 0;
  else if(!strcmp(level, "warning")) level_num = 1;
  else if(!strcmp(level, "error")) level_num = 2;
  else if(!strcmp(level, "question")) level_num = 3;
  else if(!strcmp(level, "generic")) level_num = 4;

  if(level_num >= 0
     && trigger_msg_sample_ids[level_num] >= 0)
    esd_sample_play(gnome_sound_connection,
		    trigger_msg_sample_ids[level_num]);
#endif

  for(i = 0; curnode && supinfo[i]; i++)
    {

    for(j = 0; j < curnode->numactions; j++)
      {
	if(!curnode->actions[j]->level
	   || !level
	   || !strcmp(level, curnode->actions[j]->level))
	  gnome_trigger_do(curnode->actions[j], msg, level, supinfo);
      }
    
    for(j = 0;
	j < curnode->numsubtrees
	  && strcmp(curnode->subtrees[j]->nodename,supinfo[i]);
	j++)
      /* Do nothing */ ;
    if(j < curnode->numsubtrees)
      curnode = curnode->subtrees[j];
    else
      curnode = NULL;
  }
  if(curnode)
    {
      for(j = 0; j < curnode->numactions; j++)
	{
	  if(!curnode->actions[j]->level
	     || !level
	     || !strcmp(level, curnode->actions[j]->level))
	    gnome_trigger_do(curnode->actions[j], msg, level, supinfo);
	}
    }
}

void
gnome_triggers_destroy(void)
{
  g_return_if_fail(topnode != NULL);
  gnome_triggerlist_free(topnode);
  topnode = NULL;
}

static void
gnome_trigger_free(GnomeTrigger t)
{
  if(t->level)
    g_free(t->level);
  switch(t->type) {
  case GTRIG_COMMAND:
    g_free(t->u.command); break;
  case GTRIG_MEDIAPLAY:
    g_free(t->u.media.file); break;
  default:
    break;
  }
  g_free(t);
}

static void
gnome_triggerlist_free(TriggerList t)
{
  int i;

  g_free(t->nodename);

  for(i = 0; i < t->numsubtrees; i++) {
    gnome_triggerlist_free(t->subtrees[i]);
  }
  g_free(t->subtrees);

  for(i = 0; i < t->numactions; i++) {
    gnome_trigger_free(t->actions[i]);
  }
  g_free(t->actions);

  g_free(t);
}

static void
gnome_trigger_do(GnomeTrigger t,
		 const char *msg,
		 const char * level,
		 const char *supinfo[])
{
  g_return_if_fail(t != NULL);

  actiontypes[t->type](t, msg, level, supinfo);
}

static void
gnome_trigger_do_function(GnomeTrigger t,
			  const char *msg,
			  const char *level,
			  const char *supinfo[])
{
  t->u.function(msg, level, supinfo);
}

static void
gnome_trigger_do_command(GnomeTrigger t,
			 const char *msg,
			 const char *level,
			 const char *supinfo[])
{
  char **argv;
  int nsupinfos, i;

  for(nsupinfos = 0; supinfo[nsupinfos]; nsupinfos++);

  argv = g_malloc(sizeof(char *) * (nsupinfos + 4));
  argv[0] = t->u.command;
  argv[1] = msg;
  argv[2] = level;

  for(i = 0; supinfo[i]; i++) {
    argv[i + 3] = supinfo[i];
  }
  argv[i + 3] = NULL;

  /* We're all set, let's do it */
  {
    pid_t childpid;
    int status;
    childpid = fork();
    if(childpid)
      waitpid(childpid, &status, 0);
    else
      execv(t->u.command, argv);
  }
  
  g_free(argv);
}

static void
gnome_trigger_do_mediaplay(GnomeTrigger t,
			   const char *msg,
			   const char *level,
			   const char *supinfo[])
{
#ifdef HAVE_ESD
#else
  g_warning("Request to play media file %s - esound support not available\n",
	    t->u.media.file);
#endif
}
