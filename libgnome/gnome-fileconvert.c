#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <glib.h>

#include "gnome-defs.h"
#include "gnome-string.h"
#include "gnome-fileconvert.h"

#define TYPES_CVT_FILENAME "types.cvt"

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
  gchar *ccmd;

  convlist = gfc_get_path(fromtype, totype);

  for(l = convlist, infd = fd; l; l = l->next) {
    ccmd = l->data;
    outfd = gfc_run_pipe(ccmd, infd);
    infd = outfd;
  }
  return infd;
}

static GList *
gfc_get_path(gchar *fromtype, gchar *totype)
{
  GList *retval = NULL;
  gchar aline[512];
  gchar **parts, *cmd;
  FILE *conffile;

  conffile = fopen(TYPES_CVT_FILENAME, "r");

  if(conffile) {
    while(fgets(aline, sizeof(aline), conffile)) {
      gnome_chomp_string(aline, TRUE);
      if(aline[0] == '#'
	 || aline[0] == '\0')
	continue;
      parts = gnome_split_string(aline, " ", 3);

      if(!strcmp(fromtype, parts[0])
	 && !strcmp(totype, parts[1])) {
	cmd = parts[2];
	g_free(parts[0]); g_free(parts[1]); g_free(parts);
	return g_list_append(retval, cmd);
      }
    }
  }
  return retval;
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
    return fds[0];

  /* else */
  parts = gnome_split_string(acmd, " ", -1);
  dup2(infd, 0);
  dup2(fds[1], 1);
  execv(parts[0], parts);

  /* ERROR IF REACHED */
  close(0); close(1);
  exit(69);
}
