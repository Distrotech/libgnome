#include "gnome-string.h"
#include <string.h>
#include <glib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

gchar **
gnome_string_split (gchar *string, gchar *delim, gint max_tokens)
{
	gchar **retval = NULL;
	GList *items = NULL, *anode;
	gint numitems = 0, dlen, i;
	gchar *src, *cur, *nxt;

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(delim != NULL, NULL);

	if(max_tokens < 0)
		max_tokens = INT_MAX;

	dlen = strlen(delim);
	nxt = strstr(string, delim);
	if(!nxt) {
		retval = g_malloc(sizeof(gchar *) * 2);
		retval[0] = g_strdup(string);
		retval[1] = NULL;
		return retval;
	}
	src = cur = g_strdup(string);
	nxt = strstr(src, delim);
	
	while(nxt && numitems < (max_tokens - 1)) {
		*nxt = '\0';
		items = g_list_append(items, g_strdup(cur));
		cur = nxt + dlen;
		nxt = strstr(cur, delim);
		numitems++;
	}
	/* We have to take the rest of the string and put it as last token */
	if(*cur) {
		items = g_list_append(items, g_strdup(cur));
		numitems++;
	}
	g_free(src);

	retval = g_malloc(sizeof(gchar *) * (numitems + 1));
	for(anode = items, i = 0; anode; anode = anode->next, i++)
		retval[i] = anode->data;
	retval[i] = NULL;
	g_list_free(items);

	return retval;
}

gchar *
gnome_string_join(gchar *separator, ...)
{
	va_list l;
	gint nstrings, i;
	gchar **strings;
	gchar *retval;

	/* Count number of strings */

	va_start(l, separator);
	for (nstrings = 0; va_arg(l, gchar *); nstrings++);
	va_end(l);

	/* Build list */

	strings = g_new(gchar *, nstrings+1);

	va_start(l, separator);

	for (i = 0; i < nstrings; i++)
		strings[i] = va_arg(l, gchar *);

	va_end(l);

	strings[nstrings] = NULL;
	
	/* And pass them to the real function */

	retval = gnome_string_joinv(separator, strings);
	g_free(strings);

	return retval;
}

gchar *
gnome_string_joinv(gchar *separator, gchar **strings)
{
	gchar *retval;
	gint nstrings;
	gint len;
	gint i;

	g_return_val_if_fail(separator != NULL, NULL);
	g_return_val_if_fail(strings != NULL, NULL);

	/* Count number of strings and their accumulated length */
	
	len = 1; /* start with space for null char */
	
	for (nstrings = 0; strings[nstrings]; nstrings++)
		len += strlen(strings[nstrings]);

	if (nstrings == 0)
		return g_strdup(""); /* No strings means we return an empty string */

	len += strlen(separator) * (nstrings - 1);

	retval = g_malloc(len * sizeof(gchar));

	strcpy(retval, strings[0]);

	for (i = 1; i < nstrings; i++) {
		strcat(retval, separator);
		strcat(retval, strings[i]);
	}

	return retval;
}

gchar *
gnome_string_chomp(gchar *astring, gboolean in_place)
{
	int i;
	gchar *retval, *end;

	g_return_val_if_fail(astring != NULL, NULL);

	if(in_place == FALSE)
		retval = g_strdup(astring);
	else
		retval = astring;

	i = strlen (retval);
	if (!i)
		return retval;

	end = retval + i - 1;
 	for (; end >= retval && isspace (*end); end--)
		*end = '\0';

	return retval;
}

void
gnome_string_array_free(gchar **strarray)
{
  int i;

  if(strarray == NULL) return; /* Don't use g_warning because this is legal */

  for(i = 0; strarray[i]; i++)
    g_free(strarray[i]);

  g_free(strarray);
}
