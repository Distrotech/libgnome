/* 
 * Copyright 1997 Paolo Molaro 
 *  This is LGPL'ed code.
 */

#include <config.h>

#include <gtk/gtk.h>
#include "libgnomeP.h"
#include "gnome-mime.h"
#include <string.h>
#include <ctype.h>

static void mime_fill_from_file (gchar * filename);
static void mcap_fill_from_file (gchar * filename);

static GHashTable *mime_hash = NULL;
static GHashTable *mcap_hash = NULL;

static void
mime_fill_from_file (gchar * filename)
{
	FILE *f;
	gchar buf[1024];
	gchar *p, *s, *mtype;
	gint used;
	const gchar *blancks = " \t\n\r";
	
	if (!(f = fopen (filename, "r")))
		return;
	
	while (fgets (buf, 1024, f)) {
		p = buf;
		while (*p && isspace (*p))
			++p;
		if (!*p || *p == '#')
			continue;
		used = 0;
		s = g_strdup (p);
		mtype = strtok (s, blancks);
		while ((p = strtok (NULL, blancks))) {
			g_hash_table_insert (mime_hash, p, mtype);
			used = 1;
		}
		if (!used)
			g_free (s);
	}
	fclose (f);
}

gchar *
gnome_mime_type (gchar * filename)
{
	gchar *ext;
	gchar *result;
	gchar *defaultv = "text/plain";	/* maybe NULL? */

	if (!filename)
		return defaultv;
	ext = strrchr (filename, '.');
	if (!ext)
		ext = filename;
	else
		++ext;

	if (!mime_hash) {
		gchar *f;
		mime_hash = g_hash_table_new ((GHashFunc) g_str_hash, (GCompareFunc) g_str_equal);
		mime_fill_from_file ("/etc/mime.types");
		f = gnome_util_prepend_user_home (".mime.types");
		if (f) {
			mime_fill_from_file (f);
			g_free (f);
		}
	}
	result = g_hash_table_lookup (mime_hash, ext);
	if (!result) {
		gchar *s = alloca (strlen (ext) + 1);
		gchar *p = s;
		while (*ext) {
			*p++ = tolower (*ext);
			++ext;
		}
		*p = 0;
		result = g_hash_table_lookup (mime_hash, s);
		if (!result)
			return defaultv;
	}
	return result;
}

static void
mcap_fill_from_file (gchar * filename)
{
	FILE *f;
	gchar buf[1024];
	gchar *p, *s, *mtype;
	GList *list;
	GnomeMailCap *mcap;
	const gchar *blancks = ";";

	if (!(f = fopen (filename, "r")))
		return;
	while (fgets (buf, 1024, f)) {
		p = buf;
		while (*p && isspace (*p))
			++p;
		if (!*p || *p == '#')
			continue;
		s = g_strdup (p);
		mtype = strtok (s, blancks);
		list = g_hash_table_lookup (mcap_hash, mtype);

		mcap = g_new (GnomeMailCap, 1);
		mcap->program = NULL;
		mcap->description = NULL;
		mcap->nametemplate = NULL;
		mcap->test = NULL;
		mcap->composetyped = NULL;
		mcap->copiousoutput = 0;
		mcap->needsterminal = 0;

		list = g_list_append (list, mcap);
		g_hash_table_insert (mcap_hash, mtype, list);

		p = strtok (NULL, blancks);
		while (*p && isspace (*p))
			++p;
		mcap->program = p;

		while ((p = strtok (NULL, blancks))) {
			while (*p && isspace (*p))
				++p;
#ifdef GNOME_ENABLE_DEBUG
			printf("parsing: %s\n", p);
#endif
			if (!strncmp ("test=", p, 5))
				mcap->test = p + 5;
			else if (!strncmp ("description=", p, 12))
				mcap->description = p + 12;
			else if (!strncmp ("nametemplate=", p, 13))
				mcap->nametemplate = p + 13;
			else if (!strncmp ("composetyped=", p, 13))
				mcap->composetyped = p + 13;
			else if (!strncmp ("copiousoutput", p, 13))
				mcap->copiousoutput = 1;
			else if (!strncmp ("needsterminal", p, 13))
				mcap->needsterminal = 1;
		}
	}
	fclose (f);
}

GList *
gnome_mime_entries (gchar * mime_type)
{

	if (!mcap_hash) {
		gchar *f;
		mcap_hash = g_hash_table_new ((GHashFunc) g_str_hash, (GCompareFunc) g_str_equal);
		f = gnome_util_prepend_user_home (".mailcap");
		if (f) {
			mcap_fill_from_file (f);
			g_free (f);
		}
		mcap_fill_from_file ("/etc/mailcap");
	}
	return g_hash_table_lookup (mcap_hash, mime_type);
}

GnomeMailCap *
gnome_mime_default_entry (gchar * mime_type)
{
	GList *list;
	list = gnome_mime_entries (mime_type);
	if (list)
		return (GnomeMailCap *) list->data;
	return NULL;
}

gchar *
gnome_mime_program (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->program;
	return NULL;
}

gchar *
gnome_mime_description (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->description;
	return NULL;
}

gchar *
gnome_mime_nametemplate (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->nametemplate;
	return NULL;
}

gchar *
gnome_mime_test (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->test;
	return NULL;
}

gchar *
gnome_mime_composetyped (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->composetyped;
	return NULL;
}

gint
gnome_mime_copiousoutput (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->copiousoutput;
	return 0;
}

gint
gnome_mime_needsterminal (gchar * mime_type)
{
	GnomeMailCap *mcap;

	mcap = gnome_mime_default_entry (mime_type);
	if (mcap)
		return mcap->needsterminal;
	return 0;
}

#ifdef MIMETEST

int main (int argc, char *argv[])
{
	gint i;
	gchar *mtype;
	gchar *program;
	gchar *desc;

	gnome_mime_init ();

	for (i = 1; i < argc; ++i) {
		mtype = gnome_mime_type (argv[i]);
		printf ("%-24s\t%s\n", mtype ? mtype : "", argv[i]);
		program = gnome_mime_program (mtype);
		desc = gnome_mime_description (mtype);
		printf ("Program: %s\nDescription: %s\n", program, desc);
	}
	return 0;
}

#endif
