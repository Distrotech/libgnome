/*
 * gnome-fileconvert.c: Routines for converting file formats
 *
 * Author:
 *   Elliot Lee (sopwith@cuc.edu)
 *
 * Put in production by Miguel de Icaza.
 */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <glib.h>
#include <dirent.h>

#include "gnome-defs.h"
#include "gnome-fileconvert.h"
#include "gnome-util.h"
#include "libgnomeP.h"

typedef gint Cost;
#define UNREACHABLE 32000 /*MAXINT or something*/
#define NOCOST 0
#define MINCOST 1

#define IMPOSSIBLE_PATH NULL

typedef struct _FileConverter FileConverter;
typedef struct _FileType FileType;

struct _FileConverter {
	FileType *fromtype;
	FileType *totype;
	gint cost;
	gchar *cmdline;
};

struct _FileType {
	gchar *name;
	GList *arcs;
	gint cost;
	gint done;
	FileConverter *best_way_here;
};

static GList *gfc_get_path(const char *fromtype, const char *totype);
static gint   gfc_run_pipe(gchar *acmd, gint infd);

/**
 * gnome_file_convert:
 * @filename: file to convert
 * @fromtype: mime type of the file
 * @totype:   target mime type we want
 *
 * Converts @filename to the @totype format.
 *
 * Returns -1 on failure, or a file descriptor to the converted
 * file.
 */
gint
gnome_file_convert (const char *filename, const char *fromtype, const char *totype)
{
	gint fd = open (filename, O_RDONLY);

	if (fd >= 0)
		return gnome_file_convert_fd (fd, fromtype, totype);
	else
		return -1;
}

/**
 * gnome_file_convert_fd:
 * @fd: file desciptor pointing to the file to convert.
 * @fromtype: mime type of the file
 * @totype:   target mime type we want
 *
 * Converts the file opened by @fd to the @totype format.
 *
 * Returns -1 on failure, or a file descriptor to the converted
 * file.
 */
gint
gnome_file_convert_fd (gint fd, const char *fromtype, const char *totype)
{
	GList *convlist, *l;
	gint infd, outfd;
	FileConverter *converter;
	
	convlist = gfc_get_path (fromtype, totype);
	if (!convlist)
		return -1;
	
	for (l = convlist, infd = fd; l; l = l->next){
		if (!(converter = l->data)){
			infd = -1;
			break;
		}
#ifdef GNOME_ENABLE_DEBUG
		g_print("%s %s: %s\n", converter->fromtype->name, converter->totype->name,
			converter->cmdline);
#endif
		if (converter->cmdline == NULL)
			continue;
		outfd = gfc_run_pipe (converter->cmdline, infd);
		if (infd != fd)
			close (infd);
		infd = outfd;
	}
	g_list_free (convlist);
	return infd;
}

/* Probably could use gnome_config routines for this,
   as soon as miguel redoes it */
static void
load_types_from (GHashTable *file_types, char *fname)
{
	gchar aline[512];
	gchar **parts;
	FILE *conffile;
	FileConverter *newarc;
	FileType *fromtype, *totype;

	conffile = fopen (fname, "r");
	
	if (!conffile)
		return;
	
	while (fgets(aline, sizeof(aline), conffile)){
		g_strchomp (aline);
		if (aline[0] == '#' || aline[0] == '\0')
			continue;
		
		parts = g_strsplit (aline, " ", 3);
		
		if (! (fromtype = g_hash_table_lookup(file_types, parts[0])))
		{
			fromtype = g_new(FileType, 1);
			fromtype->name = parts[0];
			fromtype->arcs = NULL;
			g_hash_table_insert(file_types, fromtype->name, fromtype);
		}
		if (! (totype = g_hash_table_lookup(file_types, parts[1])))
		{
			totype = g_new(FileType, 1);
			totype->name = parts[1];
			totype->arcs = NULL;
			g_hash_table_insert(file_types, totype->name, totype);
		}
		
		newarc = g_new(FileConverter, 1);
		newarc->fromtype = fromtype;
		newarc->totype = totype;
		newarc->cost = 1; /* replace this with something from the config file */
		if(newarc->cost < MINCOST) /* non-positive costs cause infinite loops */
			newarc->cost = MINCOST;
		newarc->cmdline = parts[2];
		fromtype->arcs = g_list_prepend(fromtype->arcs, newarc);
		
		g_free(parts);
	}
	fclose(conffile);
}

static GHashTable *
gfc_read_FileConverters(void)
{
	char *file, *dirname;
	GHashTable *file_types;
	DIR *dir;
	struct dirent *dent;
	const int extlen = sizeof (".convert") - 1;
	
	file_types = g_hash_table_new(g_str_hash, g_str_equal);
	
	dirname = gnome_unconditional_datadir_file ("type-convert");

	dir = opendir (dirname);
	if (dir){
		while ((dent = readdir (dir)) != NULL){
			int len = strlen (dent->d_name);

			if (len <= extlen)
				continue;

			if (strcmp (dent->d_name + len - extlen, ".convert"))
				continue;

			file = g_concat_dir_and_file (dirname, dent->d_name);
			load_types_from (file_types, file);
			g_free (file);
		}
	}
	g_free (dirname);

	file = gnome_util_home_file ("type.convert");
	load_types_from (file_types, file);
	g_free (file);
	

	return file_types;
}


/* Used as callback from g_hash_table_foreach */
static void 
gfc_reset_path(gpointer key, FileType *node, gpointer userdata)
{
	node->best_way_here = NULL;
	node->cost = UNREACHABLE;
}

/* Calculates shortest paths from from_node using Dijkstra's algorithm.
 * Returns list of converters to get from from_node to to_node.
 * The queue is just a GList but that'll be OK for these small graphs. 
 */
static GList *
gfc_shortest_path (GHashTable *file_types,
		   FileType *from_node,  
		   FileType *to_node)
{
	GList *nodes_to_do, *best, *best_route=NULL, *tmp;
	FileType *node, *current_node;
	FileConverter *arc;
	Cost  new_cost;
	
	/* Reset all the path data */
	g_hash_table_foreach(file_types, (GHFunc)gfc_reset_path, NULL);
	
	/* Start with from_node */
	from_node->cost = NOCOST;
	from_node->best_way_here = NULL;
	nodes_to_do = g_list_append(NULL, from_node);
	
	/* Find shortest paths */
	while(nodes_to_do)
	{
		/* Find cheapest remaining reachable node */
		best = NULL;
		for(tmp = nodes_to_do; tmp; tmp = tmp->next)
			if(!best || ((FileType *)tmp->data)->cost < ((FileType *)best->data)->cost)
				best = tmp;
		current_node = ((FileType *)best->data);
		nodes_to_do = g_list_remove_link(nodes_to_do, best);
		g_list_free(best);
		
		/* if to_node is found we are done, retrace the path here and return it*/
		if(current_node == to_node) {
			for(arc = to_node->best_way_here; arc; arc = arc->fromtype->best_way_here)
				best_route = g_list_prepend(best_route, arc);
			return best_route;
		}
		
		/* Explore all converters available from current_node */    
		for(tmp = current_node->arcs; tmp; tmp = tmp->next)
		{
			arc = tmp->data;
			new_cost = current_node->cost + arc->cost;
			node = arc->totype;
			if(new_cost < node->cost)
			{
				if(node->cost == UNREACHABLE)
					nodes_to_do = g_list_prepend(nodes_to_do, node);
				node->cost = new_cost;
				node->best_way_here = arc;
			}
		}
	}
	/* no route to to_node found */
	return IMPOSSIBLE_PATH;
}


/* Get the list of converters required to convert fromtype to totype */
static GList *
gfc_get_path (const char *fromtype, const char *totype)
{
	static GHashTable *file_types;
	static gboolean read_datfile = FALSE;
	FileType *from_node, *to_node;
	
	if(!read_datfile)
	{
		file_types = gfc_read_FileConverters();
		read_datfile = TRUE;
	}
	
	/* Look up the start and goal types */
	from_node = (FileType *) g_hash_table_lookup(file_types,  fromtype);
	to_node = (FileType *) g_hash_table_lookup(file_types,  totype);
	
	/* Give up if asked for an unknown file type */
	if(from_node == NULL || to_node == NULL)
		return IMPOSSIBLE_PATH;
	
	return gfc_shortest_path(file_types, from_node, to_node);
}

static gint
gfc_run_pipe (gchar *acmd, gint infd)
{
	gchar **parts;
	gint childpid;
	gint fds[2];
	
	if (pipe (fds))
		return -1;
	
	childpid = fork ();
	if (childpid < 0)
		return -1;
	
	if (childpid){
		close (fds [1]);
		waitpid (childpid, &childpid, 0);
		return fds [0];
	}
	
	/* else */
	
	parts = g_strsplit (acmd, " ", -1);
	dup2 (infd, 0);
	dup2 (fds [1], 1);
	dup2 (fds [1], 2);
	if (fds [1] > 2)
		close (fds [1]);
	if (infd > 2)
		close (infd);
	if (fork()) /* Double-forking is good for the (zombified) soul ;-) */
		exit (0);
	else
		execvp (parts [0], parts);
	
	/* ERROR IF REACHED */
	close (0);
	close (1);
	exit (69);
}
