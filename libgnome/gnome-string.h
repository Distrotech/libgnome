#include "gnome-defs.h"
#include <glib.h>
#include <stdarg.h>

gchar **gnome_string_split (gchar *string,
			    gchar *delim,
			    gint max_tokens);
gchar * gnome_string_join  (gchar *separator, gchar *first, ...);
gchar * gnome_string_joinv (gchar *separator,
			    gchar **strings);
gchar * gnome_string_chomp (gchar *astring,
			    gboolean in_place);
