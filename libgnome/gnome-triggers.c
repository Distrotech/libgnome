/* By Elliot Lee */

#include "gnome-triggers.h"
#include "gnome-util.h"
#include "gnome-string.h"
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
gnome_trigger_do(GnomeTrigger t, char *msg, char *level, char *supinfo[]);
static void
gnome_trigger_do_function(GnomeTrigger t,
			  char *msg, char *level, char *supinfo[]);
static void
gnome_trigger_do_command(GnomeTrigger t,
			 char *msg, char *level, char *supinfo[]);

/* FILEWIDE VARIABLES */

static TriggerList topnode = NULL;

static GnomeTriggerTypeFunction actiontypes[] =
/* This list should have entries for all the trigger types in
   gnome-triggers.h */
{
  (GnomeTriggerTypeFunction)NULL,
  gnome_trigger_do_function,
  gnome_trigger_do_command,
  (GnomeTriggerTypeFunction)NULL,
  (GnomeTriggerTypeFunction)NULL
};

/* IMPLEMENTATIONS */
void
gnome_triggers_init(void)
{
  char *fn;
  fn = gnome_datadir_file("gnome/triggers/list");
  if(fn) {
    gnome_triggers_readfile(fn);
    g_free(fn);
  }

  fn = gnome_util_prepend_user_home(".gnome/triggers/list");
  if(fn) {
    gnome_triggers_readfile(fn);
    g_free(fn);
  }
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
      if(parts) {
	for(i = 0; parts[i]; i++)
	  g_free(parts[i]);
	g_free(parts);
      }
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

    for(i = 0; subnames[i]; i++)
      g_free(subnames[i]);
    g_free(subnames); subnames = NULL;

    for(i = 0; parts[i]; i++)
      g_free(parts[i]);
    g_free(parts); parts = NULL;
  }
  g_free(nt);

  return 0;
}

void gnome_triggers_add_trigger(GnomeTrigger nt, ...)
{
  va_list l;
  va_start(l, nt);
  /* FIXME: this is incorrect */
  /*  gnome_triggers_vadd_trigger(nt, l);*/
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
gnome_triggers_do(char *msg, char *level, ...)
{
  va_list l;
  va_start(l, level);
  /* FIXME: this is incorrect */
  /*  gnome_triggers_vdo(msg, level, l);*/
}

void
gnome_triggers_vdo(char *msg, char *level, char *supinfo[])
{
  TriggerList curnode = topnode;
  int i, j;

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
    g_free(t->u.mediafile); break;
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
gnome_trigger_do(GnomeTrigger t, char *msg, char * level, char *supinfo[])
{
  g_return_if_fail(t != NULL);

  actiontypes[t->type](t, msg, level, supinfo);
}

static void
gnome_trigger_do_function(GnomeTrigger t,
			  char *msg, char *level, char *supinfo[])
{
  t->u.function(msg, level, supinfo);
}

static void
gnome_trigger_do_command(GnomeTrigger t, char *msg, char *level, char *supinfo[])
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
