
#ifndef __GNOME_NET_H__
#define __GNOME_NET_H__

#define GNOME_NET_OK  0
#define GNOME_NET_EOF 1
#define GNOME_NET_ERROR 2
#define GNOME_NET_AGAIN 3

#define GNOME_BUFFERING_NONE 0
#define GNOME_BUFFERING_LINE 1

typedef void (*GnomeNetFn)(gint sock);

struct NetSocketData {
	gint tag;
        GnomeNetFn connectfn;
        GnomeNetFn readfn;
        GString *s;
        gint buffering;
};

gchar *gnome_hostname(void);

gint gnome_net_connect_tcp(gchar *host, gint port,
		     gchar *myhost, gint myport, GnomeNetFn connectfn);
void gnome_net_close(gint sock);

void gnome_net_read_callback_create(gint sock, GnomeNetFn readfn);
void gnome_net_read_callback_remove(gint sock);

gint gnome_net_gets(gint sock, GString *gs);
gint gnome_net_printf(gint sock, gchar *format, ...);

#endif
