#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <glib.h>

#include "gnome-defs.h"
#include "gnome-string.h"
#include "gnome-fileconvert.h"

#define TYPES_CVT_FILENAME "types.cvt"

typedef struct _FileConverter FileConverter;
struct _FileConverter {
  gchar *fromtype;
  gchar *totype;
  gchar *cmdline;
  gint tag; /* Used for knowing whether we have visited
	       this node yet */
  GList *links;
};

static GList *FileConverters = NULL; /* A list of all the FileConverter's we
			       read in */

static GList *
gfc_get_path(gchar *fromtype, gchar *totype);
static gint
gfc_run_pipe(gchar *acmd, gint infd);

gint
gnome_file_convert(gchar *filename, gchar *fromtype, gchar *totype)
{
  gint fd = open(filename, O_RDONLY);
  if(fd >= 0)
    return gnome_file_convert_fd(fd, fromtype, totype);
  else
    return -1;
}

gint
gnome_file_convert_fd(gint fd, gchar *fromtype, gchar *totype)
{
  GList *convlist, *l;
  gint infd, outfd;
  FileConverter *anode;

  srand(time(NULL));

  convlist = gfc_get_path(fromtype, totype);

  for(l = convlist, infd = fd; l; l = l->next) {
    anode = l->data;
    g_print("%s %s: %s\n", anode->fromtype, anode->totype,
	    anode->cmdline);
    if(anode->cmdline == NULL)
      continue;
    outfd = gfc_run_pipe(anode->cmdline, infd);
    if(infd != fd) close(infd);
    infd = outfd;
  }
  return infd;
}

/* Probably could use gnome_config routines for this,
   as soon as miguel redoes it */
static void
gfc_read_FileConverters(void)
{
  gchar aline[512];
  gchar **parts;
  FILE *conffile;
  FileConverter *newnode;

  conffile = fopen(TYPES_CVT_FILENAME, "r");

  if(conffile) {
    while(fgets(aline, sizeof(aline), conffile)) {
      gnome_string_chomp(aline, TRUE);
      if(aline[0] == '#'
	 || aline[0] == '\0')
	continue;
      parts = gnome_string_split(aline, " ", 3);
      newnode = g_new(FileConverter, 1);
      newnode->fromtype = parts[0];
      newnode->totype = parts[1];
      newnode->cmdline = parts[2];
      g_free(parts);
      newnode->tag = 0; newnode->links = NULL;
      FileConverters = g_list_append(FileConverters, newnode);
    }
  }
}

static void
gfc_make_links(void)
{
  GList *tmp1, *tmp2;
  FileConverter *anode1, *anode2;
  for(tmp1 = FileConverters; tmp1; tmp1 = tmp1->next)
    {
      for(tmp2 = FileConverters; tmp2; tmp2 = tmp2->next)
	{
	  if(tmp1 == tmp2)
	    break;
	  anode1 = tmp1->data; anode2 = tmp2->data;
	  if(!strcmp(anode1->fromtype, anode2->totype))
	    anode2->links = g_list_append(anode2->links,
					  anode1);
	}
    }
}

/* This is a recursive function. Probably very slow,
   who knows. The anode->tag member helps a lot tho,
   and ensures we never get into any loops */
static GList *
gfc_try_path(GList *inlist,
	     gchar *fromtype,
	     gchar *realtotype,
	     gint tag)
{
  GList *retval, *tmp, *tmp2;
  FileConverter *anode;
  for(tmp = inlist; tmp; tmp = tmp->next)
    {
      anode = tmp->data;
      if(anode->tag == tag)
	continue;
      if(!strcmp(fromtype, anode->fromtype))
	{
	  retval = g_list_append(NULL, anode);
	  if(!strcmp(realtotype, anode->totype))
	      return retval;
	  anode->tag = tag;
	  tmp2 = gfc_try_path(anode->links, anode->totype,
			      realtotype, tag);
	  if(tmp2)
	    return g_list_concat(retval, tmp2);
	  else
	    g_list_free(retval);
	}
    }
  return NULL;
}

/* This algorithm probably isn't optimal
   - it uses the "first-found" rather than "shortest-found"
   path */
static GList *
gfc_get_path(gchar *fromtype, gchar *totype)
{
  static gboolean read_datfile = FALSE;

  if(!read_datfile)
    {
      gfc_read_FileConverters();
      gfc_make_links();
      read_datfile = TRUE;
    }

  if(!strcmp(fromtype, totype))
    return NULL;

  return gfc_try_path(FileConverters, fromtype, totype, rand());
}

static gint
gfc_run_pipe(gchar *acmd, gint infd)
{
  gchar **parts;
  gint childpid;
  gint fds[2];
  
  if(pipe(fds))
    return -1;

  childpid = fork();
  if(childpid < 0)
    return -1;

  if(childpid)
    {
      close(fds[1]);
      waitpid(childpid, &childpid, 0);
      return fds[0];
    }

  /* else */
  parts = gnome_string_split(acmd, " ", -1);
  dup2(infd, 0);
  dup2(fds[1], 1);
  dup2(fds[1], 2);
  if(fds[1] > 2)
    close(fds[1]);
  if(infd > 2)
    close(infd);
  if(fork()) /* Double-forking is good for the (zombified) soul ;-) */
    exit(0);
  else
    execvp(parts[0], parts);

  /* ERROR IF REACHED */
  close(0); close(1);
  exit(69);
}
