#include <glib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <gnome.h>

/* `g_string_length' doesn't appear in the CVS version of `gtk+/glib',
   and I can't find a reference to it in its ChangeLog */
#define g_string_length(pGString_x) ((pGString_x)->len)
extern char* g_vsprintf (gchar *fmt, va_list *args, va_list *args2);

static struct NetSocketData *sockets = NULL;
static max_sockets = 0;

gchar *gnome_hostname(void)
{
	static gchar *hostname = NULL;

	if (hostname == NULL) {
		hostname = g_new(gchar, 128);
		gethostname(hostname, 128);
	}
	return hostname;
}

static gint make_inetaddr(gchar *host, gint port, struct sockaddr_in *sa)
{
	struct hostent *he;
	
	if ((he = gethostbyname(host)) != NULL) {
		bzero(sa, sizeof(struct sockaddr_in));
		bcopy(he->h_addr, &sa->sin_addr, he->h_length);
		sa->sin_family = he->h_addrtype;
	} else if ((sa->sin_addr.s_addr = inet_addr(host)) != -1) {
		sa->sin_family = AF_INET;
	} else {
		return -1;
	}
	sa->sin_port = htons(port);
	return 0;
}

static gint net_socket_new(gchar *myhost, gint myport, gint domain, gint type,
			   gint proto, gint nonblock)
{
	struct hostent *he;
	struct sockaddr_in sa;

	gint sock, fl;

	if (myhost != NULL) {
		if (make_inetaddr(myhost, myport, &sa) == -1) {
			return -1;
		}
	}


	if ((sock = socket(domain, type, proto)) < 0) {
		return -2;
	}

	if (myhost != NULL) {
		if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
			return -3;
		}
	}

	if (nonblock) {
		fcntl(sock, F_GETFL, &fl);
		fcntl(sock, F_SETFL, fl | O_NONBLOCK);
	}

	return sock;
}

static void net_connect_callback(gpointer data, gint sock, GdkInputCondition cond)
{
	struct NetSocketData *nsd = (struct NetSocketData *)data;

	gdk_input_remove(nsd->tag);
	(nsd->connectfn)(sock);
}

static void gnome_net_read_callback(gpointer data, gint sock, GdkInputCondition cond)
{
	struct NetSocketData *nsd = (struct NetSocketData *)data;
        (nsd->readfn)(sock);
}

gint gnome_net_connect_tcp(gchar *host, gint port,
		           gchar *myhost, gint myport,
		           GnomeNetFn connectfn)
{
	gint sock;
	gint res;
	struct sockaddr_in sa;

	if ((sock = net_socket_new(myhost, myport, PF_INET, SOCK_STREAM, IPPROTO_TCP,
				   connectfn != NULL)) == -1) {
		return sock; /* error */
	}

	if (make_inetaddr(host, port, &sa) == -1) {
		return -4;
	}

	res = connect(sock, (struct sockaddr *)&sa, sizeof(sa));

	if (res == -1 && errno != EINPROGRESS) {
		return -errno;
	}

	if (sock > max_sockets) {
		max_sockets = (sock+7) & ~7;
		sockets = g_realloc(sockets, max_sockets*sizeof(struct NetSocketData));
	}
	sockets[sock].tag = 0;
        sockets[sock].s = g_string_new(NULL);
	if (connectfn) {
		sockets[sock].connectfn = connectfn;
		sockets[sock].tag = gdk_input_add(sock, GDK_INPUT_WRITE, net_connect_callback, &sockets[sock]);
	}

        return sock;
}

void gnome_net_read_callback_create(gint sock, GnomeNetFn readfn)
{
        sockets[sock].readfn = readfn;
        sockets[sock].tag = gdk_input_add(sock, GDK_INPUT_READ, gnome_net_read_callback, &sockets[sock]);
}


void gnome_net_read_callback_remove(gint sock)
{
	gdk_input_remove(sockets[sock].tag);
        sockets[sock].tag = 0;
}

gint gnome_net_gets(gint sock, GString *gs)
{
	struct NetSocketData *nsd = sockets+sock;
        gchar c;
        gint res;

        g_string_truncate(gs, 0);
        
        for (;;) {
                res = read(sock, &c, 1);
                switch (res) {
                case 1:
                        if (c == '\n') {
                                g_string_append(gs, nsd->s->str);
                                g_string_truncate(nsd->s, 0);
                                return GNOME_NET_OK;
                        }
                        if (c != '\r') g_string_append_c(nsd->s, c);
                        break;
                case 0:
                        if (g_string_length(nsd->s) == 0) return GNOME_NET_EOF;
                        return GNOME_NET_OK;
                case -1:
                        if (errno = EAGAIN) return GNOME_NET_AGAIN;
                        return GNOME_NET_ERROR;
                }
        }
}

gint gnome_net_printf(gint sock, gchar *format, ...)
{
	va_list args, args2;
	char *buf;

	va_start(args, format);
	va_start(args2, format);
	buf = g_vsprintf(format, &args, &args2);
	va_end(args);
	va_end(args2);
	return write(sock, buf, strlen(buf));
}

void gnome_net_close(gint sock)
{
	if (sockets[sock].tag != 0) {
		gnome_net_read_callback_remove(sock);
        }
        g_string_free(sockets[sock].s, TRUE);
	close(sock);
}

