#ifndef __GNOME_MIME_H__
#define __GNOME_MIME_H__

BEGIN_GNOME_DECLS

typedef struct {
        gchar* program;
        gchar* description;
        gchar* nametemplate;
        gchar* test;
        gchar* composetyped;
        gint copiousoutput;
        gint needsterminal;
        /* where are the specs for the mailcap format? */
} GnomeMailCap;

void          gnome_mime_init          (void);

/* do not free() any of the returned values */
gchar*        gnome_mime_type          (gchar* filename);
gchar*        gnome_mime_type_or_default (gchar *filename, gchar *ddef);
GnomeMailCap* gnome_mime_default_entry (gchar* mime_type);
GList*        gnome_mime_entries       (gchar* mime_type);
gchar*        gnome_mime_program       (gchar* mime_type);
gchar*        gnome_mime_description   (gchar* mime_type);
gchar*        gnome_mime_nametemplate  (gchar* mime_type);
gchar*        gnome_mime_test          (gchar* mime_type);
gchar*        gnome_mime_composetyped  (gchar* mime_type);
gint          gnome_mime_copiousoutput (gchar* mime_type);
gint          gnome_mime_needsterminal (gchar* mime_type);

END_GNOME_DECLS

#endif

