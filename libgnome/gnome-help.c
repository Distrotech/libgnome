#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "gnome-defs.h"
#include "gnome-help.h"
#include "gnome-i18n.h"
#include "gnome-util.h"

#define HELP_PROG "gnome-help-browser"

/* I added this because I didnt want to break all apps using 
 * gnome_help_file_path() currently. We need a good solution (if this isnt it)
 * to handle case where locale file didnt exist
 */
gchar *gnome_help_file_find_file (gchar *app, gchar *path)
{
  GList *language_list;
  GString *buf;
  
  gchar *res= NULL;
  gchar *p;
  
  language_list= gnome_i18n_get_language_list ("LC_ALL");
  
  while (!res && language_list)
    {
      const gchar *lang;
      
      lang= language_list->data;

      buf= g_string_new (NULL);
      g_string_sprintf (buf, "gnome/help/%s/%s/%s", app, lang, path);
      res= (gchar *)gnome_unconditional_datadir_file (buf->str);
      p = strrchr(res, '#');
      if (p)
         *p = '\0';
      g_string_free (buf, TRUE);
      
      if (!g_file_exists (res))
	{
	  g_free (res);
	  res= NULL;
	}
      
      language_list= language_list->next;
    }
  
  return res;
}

gchar *gnome_help_file_path(gchar *app, gchar *path)
{
  gchar *res;
  GString *buf;
  
  res= gnome_help_file_find_file (app, path);
  
  /* If we found no document on the language depending datadirs, we
     return a non existing file from a default datadir.  It's non
     existing, because 'C' is always included in a language list.  */

  if (!res)
    {
      buf = g_string_new(NULL);
      g_string_sprintf(buf, "gnome/help/%s/C/%s", app, path);
      res = (gchar *)gnome_unconditional_datadir_file(buf->str);
      g_string_free(buf, TRUE);
    }

    return res;
}

void gnome_help_display(void *ignore, GnomeHelpMenuEntry *ref)
{
    gchar *file, *url;

    file = gnome_help_file_path(ref->name, ref->path);
    if (!file)
	    return;
    url = alloca(strlen(file)+10);
    strcpy(url,"file:");
    strcat(url, file);
    gnome_help_goto(ignore, url);
    g_free(file);
}

void gnome_help_goto(void *ignore, gchar *file)
{
    pid_t pid;

#ifdef GNOME_ENABLE_DEBUG    
    printf("gnome_help_goto: %s\n", (char *)file);
#endif

    if (!(pid = fork())) {
	execlp(HELP_PROG, HELP_PROG, file, NULL);
	g_error("gnome_help_goto: exec failed: %s\n", g_strerror(errno));
    }
}
