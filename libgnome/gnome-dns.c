/* 
 * Copyright (C) 1997 Ian Main
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>	
#include <netinet/in.h>	
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "gnome-defs.h"
#include "gnome-dns.h"

#undef VERBOSE

/* the maximum nuber of servers to spawn.. just as a safty measure
 * in case something goes wrong -- so we don't have 
 * gnome_fork_bombs() */

#define GNOME_DNS_MAX_SERVERS       8

static void gnome_dns_callback(gpointer server_num, gint source,
			       GdkInputCondition condition);
static gint gnome_dns_create_server(void);

typedef struct
{
	/* boolean to tell if server is doing a lookup */
	gboolean in_use;
	/* pipefd's to communicate to server with */
	gint pipefd[2];
} DnsServer;

static DnsServer dns_server[GNOME_DNS_MAX_SERVERS];
static int num_servers;
static int server_init_cnt;

typedef struct
{
	/* host hame for cache */
	char *hostname;
	gint server; /* -1 if complete, -2 if waiting for server,
		      otherwise index to dns_server */
	/* address of host - may be 0 on failure. */
	guint32 ip_addr;
} GnomeDnsCache;

static GnomeDnsCache *dns_cache;
static int dns_cache_size, dns_cache_size_max;

typedef struct
{
	gint tag;
	gint server; /* -1 if waiting, otherwise index to dns_server[] */
	char *hostname;
	void (*callback) (guint32 ip_addr, void *callback_data);
	void *callback_data;
} GnomeDnsCon;

static GnomeDnsCon *dns_con;
static gint dns_con_size, dns_con_size_max;

static gint dns_con_tag = 0;


/*
 *--------------------------------------------------------------
 * gnome_dns_init
 *
 *   Initialize the dns functions for use.
 *
 * Arguments:
 *   server_count specifies the number of servers to fork() at
 *   init, or <= 0 to do dynamic server forking.  If you are
 *   concerned about virtual mem usage, fork your servers as one of the
 *   first things in your program.
 * 
 *   Note that it will still do dynamic forking if you specify > 0. 
 *   when it runs out of servers.. a good init value may be 1 or 2.
 * 
 * Results:
 *   gnome_dns_lookup() will be ready for use.
 *
 * Side effects:
 *   The library is initialized.
 *
 *--------------------------------------------------------------
 */


void gnome_dns_init (gint server_count)
{
	dns_con_size = 0;
	dns_con_size_max = 16;
	dns_con = g_new (GnomeDnsCon, dns_con_size_max);
	
	dns_cache_size = 0;
	dns_cache_size_max = 16;
	dns_cache = g_new (GnomeDnsCache, dns_cache_size_max);
	
	num_servers = 0;
	while (num_servers < server_count) {
		if ( (gnome_dns_create_server()) < 0) {
			g_error ("Unable to fork: %s", g_strerror(errno));
		}
	}
}


/* Send the request to the server. */
void
gnome_dns_server_req (gint server, const char *hostname) {
	dns_server[server].in_use = TRUE;
	gdk_input_add(dns_server[server].pipefd[0],
		      GDK_INPUT_READ,
		      (GdkInputFunction) gnome_dns_callback,
		      (gpointer) server);
	write (dns_server[server].pipefd[1], hostname, strlen (hostname) + 1);
}

/*
 *--------------------------------------------------------------
 * gnome_dns_lookup
 *
 * looks up an address and returns a tag for use with
 * gnome_dns_abort() if desired.  May not return -1 if
 * hostname was in cache.
 *
 * Arguments:
 * char *hostname - hostname to lookup
 * callback - function to call when dns lookup is complete.
 * callback_data - data to pass to this function.
 *   
 * Results:
 * callback function is called when dns_lookup is complete.
 * returns a tag identifying this lookup or -1 if lookup was
 * in cache.
 * 
 * Side effects:
 * a new dns server may be spawned if all the current servers
 * are in use.
 *
 *--------------------------------------------------------------
 */


guint32 gnome_dns_lookup (const char *hostname, 
			  void (* callback) (guint32 ip_addr, void *callback_data),
			  void *callback_data)
{
	gint i;
	gint tag;
	gint server;
	
	/* check for cache hit. */
	for (i = 0; i < dns_cache_size; i++)
		if (!strcmp (hostname, dns_cache[i].hostname))
			break;
	
	/* if it hit, call the callback immediately. */
	if (i < dns_cache_size && dns_cache[i].server == -1) {
		callback (dns_cache[i].ip_addr, callback_data);
		return 0;
	}
	
	/* It didn't hit in the cache with an answer - need to put request
	 into the dns_con table. */
	if (i < dns_cache_size) {
		/* hit in cache but answer hasn't come back yet. */
		server = dns_cache[i].server;
	} else {
		/* missed in cache -- create a cache entry */
		if (dns_cache_size == dns_cache_size_max) {
			dns_cache_size_max <<= 1;
			dns_cache = g_realloc (dns_cache, dns_cache_size_max * sizeof (GnomeDnsCache));
		}
		dns_cache[dns_cache_size].hostname = g_strdup (hostname);
		/* Find a server we can send the request to. */
		for (server = 0; server < num_servers; server++)
			if (!dns_server[server].in_use) {
				break;
			}
		if (server < num_servers) {
			/* found an unused server - give it the request */
			gnome_dns_server_req (server, hostname);
			dns_cache[dns_cache_size].server = server;
		} else {
			/* no unused servers - fork a new one */
			dns_cache[dns_cache_size].server = gnome_dns_create_server();
			if (dns_cache[dns_cache_size].server < 0) {
				g_error ("Unable to fork: %s", g_strerror(errno));
			}
			
		}
		dns_cache_size++;
	}
	
	if (dns_con_size == dns_con_size_max) {
		dns_con_size_max <<= 1;
		dns_con = g_realloc (dns_con, dns_con_size_max * sizeof (GnomeDnsCon));
	}
	tag = dns_con_tag++;
	dns_con[dns_con_size].tag = tag;
	dns_con[dns_con_size].server = server;
	dns_con[dns_con_size].hostname = g_strdup (hostname);
	dns_con[dns_con_size].callback = callback;
	dns_con[dns_con_size].callback_data = callback_data;
	dns_con_size++;
	
	return tag;
}

/*
 *--------------------------------------------------------------
 * gnome_dns_abort
 *
 * aborts a previous call to gnome_dns_lookup().
 *
 * Arguments:
 * gint tag32 - the tag returned from previous call to gnome_dns_lookup().
 *   
 * Results:
 *   callback function is not called.
 *
 * Side effects:
 * 
 *--------------------------------------------------------------
 */


void
gnome_dns_abort (guint32 tag)
{
	gint i;
	
	for (i = 0; i < dns_con_size; i++) {
		if (dns_con[i].tag == tag) {
			g_free (dns_con[i].hostname);
			dns_con[i] = dns_con[--dns_con_size];
			return;
		}
	}
	g_warning ("gnome_dns_abort: aborting a nonexistent tag\n");
}

/*
 *--------------------------------------------------------------
 * gnome_dns_callback
 *
 * internal function - called when dns lookup completes.. this 
 * function dispatches the callback passed to gnome_dns_lookup().
 *
 * Arguments:
 * gint server_num - the index into the dns_server struct.
 * gint source - unkown.
 * GdkIputCondition - unkown.
 *   
 * Results:
 *   callback function passed to gnome_dns_lookup is called.
 *
 * Side effects:
 * this dns_server[server_num]->in_use set to 0 for future lookups.
 * ie, the lock is removed.
 * 
 *--------------------------------------------------------------
 */

void gnome_dns_callback(gpointer serv_num, gint source,
			GdkInputCondition condition)
{
	guint32 ip_addr;
	gint server_num;
	gint i;
	gint j;
	
	/* cast it back to an integer */
	server_num = (int) serv_num;
	
#ifdef VERBOSE    
	g_printf("callback called!\n");
#endif
	/* read ip from server.  It's done as a single int rather than a string.
	 * hopefully it works ok */
	if ( (read(dns_server[server_num].pipefd[0], &ip_addr, sizeof(guint32))) < 0)
		g_error("reading from pipe: %s\n", g_strerror(errno));
	
#ifdef VERBOSE
	g_printf("ip_addr in callback is %x\n", ip_addr);
#endif
	
	/* write ip address into cache. */
	for (i = 0; i < dns_cache_size; i++)
		if (dns_cache[i].server == server_num)
			break;
	if (i < dns_cache_size) {
		dns_cache[i].ip_addr = ip_addr;
		dns_cache[i].server = -1;
	} else {
		g_warning ("gnome_dns_callback: no cache item for server\n");
	}
	
	/* Give answer to all callbacks. */
	for (i = 0; i < dns_con_size; i++) {
		if (dns_con[i].server == server_num) {
			dns_con[i].callback (ip_addr, dns_con[i].callback_data);
			g_free (dns_con[i].hostname);
			dns_con[i--] = dns_con[--dns_con_size];
		}
	}
	
	dns_server[server_num].in_use = FALSE;
	
	/* See if there's an outstanding request and, if so, serve it. */
	for (i = 0; i < dns_cache_size; i++) {
		if (dns_cache[i].server == -2) {
			dns_cache[i].server = server_num;
			for (j = 0; j < dns_con_size; j++) {
				if (!strcmp (dns_con[j].hostname, dns_cache[i].hostname))
					dns_con[j].server = server_num;
			}
			gnome_dns_server_req (server_num, dns_cache[i].hostname);
			break;
		}
	}
}


/*
 *--------------------------------------------------------------
 * gnome_dns_create_server
 *
 * internal function - creates a new server (currently using fork()).
 *
 * Arguments:
 *   
 * Results:
 * initializes the first free dns_server structure and returns
 * the index into the dns_server structure which is 
 * also the tag.  Returns -1 on error.
 *
 * Side effects:
 * 
 *--------------------------------------------------------------
 */

gint gnome_dns_create_server(void)
{
	
	int pid;
	int index = 0;
	
	
	/* for talking to dns server.  Once setup, always write to pipefd[1],
	 * and read from pipefd[0]. */
	int pipefd[2];
	int pipefd0[2];
	int pipefd1[2];
	
	/* check that we're not spawning too many servers.. this should really
	 * only be a sanity check..  I'm hoping that we'll never really spawn
	 * anywhere near the max allowed servers. */
	if (num_servers >= GNOME_DNS_MAX_SERVERS) {
		fprintf(stderr, "gnome: spawned too many dns processes - limit set to %d.\n", 
			GNOME_DNS_MAX_SERVERS);
		return(-1);
	}
	
	/* create pipe to write to dns_server with */
	if ( (pipe(pipefd0)) < 0) {
		fprintf(stderr, "gnome: creating pipe: %s\n", strerror(errno));
		return(-1);
	}
	
	/* create pipe to read from dns_server */
	if ( (pipe(pipefd1)) < 0) {
		fprintf(stderr, "gnome: creating pipe: %s\n", strerror(errno));
		return(-1);
	}
	
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "gnome: forking: %s\n", strerror(errno));
		return(-1);
	}
	
	/* start server in child */
	if (pid == 0) {
	  dup2(pipefd0[0], 0);
	  dup2(pipefd1[1], 1);
	  execlp("dns-helper", "dns-helper", NULL);
	  /* does not return */
	}
	/* else */
	
	index = num_servers;
	
	/* initialize structure */
	
	dns_server[index].in_use = FALSE;
	dns_server[index].pipefd[1] = pipefd0[1];
	dns_server[index].pipefd[0] = pipefd1[0];
	close(pipefd0[0]);
	close(pipefd1[1]);
	/* only used as a sanity check to gnome_dns_abort() */
	num_servers++;
	
	return(index);
}
