#include <stdio.h>
#include <glib.h>

#include "gnome-defs.h"
#include "gnome-util.h"
#include "gnome-string.h"
#include "gnome-history.h"

#define NUM_ENTS 10

static void write_history(GList *ents);
static void free_history_list_entry(gpointer data,
				    gpointer user_data);

void
gnome_history_recently_used(GnomeHistoryEntry ent)
{
  GList *ents;
  ents = gnome_history_get_recently_used();
  ents = g_list_append(ents, ent);
  write_history(ents);
  gnome_history_free_recently_used_list(ents);
}

GList *gnome_history_get_recently_used(void)
{
  GnomeHistoryEntry anent;
  GList *retval = NULL;
  FILE *infile;
  gchar *filename = gnome_util_home_file("document_history");
  gchar aline[512], **parts;

  infile = fopen(filename, "r");
  if(infile)
    {
      while(fgets(aline, sizeof(aline), infile))
	{
	  gnome_chomp_string(aline, TRUE);
	  if(aline[0] == '\0') continue;

	  parts = gnome_split_string(aline, " ", 4);

	  anent = g_malloc(sizeof(struct _GnomeHistoryEntry));
	  anent->filename = parts[0];
	  anent->filetype = parts[1];
	  anent->creator = parts[2];
	  anent->desc = parts[3];

	  g_free(parts);

	  retval = g_list_append(retval, anent);
	}
      fclose(infile);
    }
  g_free(filename);
  return retval;
}

static void
write_history_entry(GnomeHistoryEntry ent, FILE *outfile)
{
  fprintf(outfile, "%s %s %s %s\n",
	  ent->filename, ent->filetype, ent->creator, ent->desc);
}

static void write_history(GList *ents)
{
  FILE *outfile;
  gchar *filename = gnome_util_home_file("document_history");
  GList *t;
  gint n;

  outfile = fopen(filename, "w");
  if(outfile) {
    n = g_list_length(ents) - NUM_ENTS;
    if(n < 0) n = 0;
    g_list_foreach(g_list_nth(ents, n),
		   (GFunc)write_history_entry, outfile);
  }
  fclose(outfile);
  g_free(filename);
}

void
gnome_history_free_recently_used_list(GList *alist)
{
  g_list_foreach(alist, (GFunc) free_history_list_entry, NULL);
  g_list_free(alist);
}

static void free_history_list_entry(gpointer data,
				    gpointer user_data)
{
  GnomeHistoryEntry anent;
  anent = data;
  g_free(anent->filename);
  g_free(anent->filetype);
  g_free(anent->creator);
  g_free(anent->desc);
  g_free(anent);
}
