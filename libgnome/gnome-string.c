#include "gnome-string.h"
#include <string.h>
#include <glib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

gchar **gnome_split_string(gchar *string, gchar *delim, gint max_tokens)
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
gnome_join_strings(gchar *separator, ...)
{
	va_list l;
	va_start(l, separator);
	/* Elliot: this can not be done like this: */
	/*	return gnome_join_vstrings(separator, l);*/
}

gchar *
gnome_join_vstrings(gchar *separator, gchar **strings)
{
	gchar *retval;
	gint total_size, i, seplen;

	g_return_val_if_fail(separator != NULL, NULL);
	g_return_val_if_fail(strings != NULL, NULL);

	/* While it's not an error to have no strings to join, it
	   still needs to be handled differently */
	if(!strings[0])
		return g_strdup("");

	total_size = strlen(strings[0]) + 1;
	retval = g_malloc(total_size);
	strcpy(retval, strings[0]);
	seplen = strlen(separator);

	for(i = 1; strings[i]; i++)
	{
		total_size += seplen + strlen(strings[i]);
		retval = g_realloc(retval, total_size);
		strcat(retval, separator);
		strcat(retval, strings[i]);
	}

	return retval;
}

inline
gchar *
gnome_chomp_string(gchar *astring, gboolean in_place)
{
	int i;
	gchar *retval;

	g_return_val_if_fail(astring != NULL, NULL);

	if(in_place == FALSE)
		retval = g_strdup(astring);
	else
		retval = astring;

	i = strlen(retval) - 1;
	while(isspace(retval[i])) retval[i--] = '\0';
}
