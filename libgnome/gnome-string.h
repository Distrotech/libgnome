#include "gnome-defs.h"
#include <glib.h>
#include <stdarg.h>

gchar **gnome_split_string  (gchar *string,
			     gchar *delim,
			     gint max_tokens);
gchar * gnome_join_strings  (gchar *separator, ...);
gchar * gnome_join_vstrings (gchar *separator,
			     gchar **strings);
