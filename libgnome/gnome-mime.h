#ifndef __GNOME_MIME_H__
#define __GNOME_MIME_H__

BEGIN_GNOME_DECLS

/* do not free() any of the returned values */
const char  *gnome_mime_type                    (const gchar* filename);
const char  *gnome_mime_type_or_default         (const gchar *filename,
						 const gchar *defaultv);
const char  *gnome_mime_type_of_file            (const char *existing_filename);
const char  *gnome_mime_type_or_default_of_file (const char *existing_filename,
						 const gchar *defaultv);
const char  *gnome_mime_type_from_magic         (const gchar *filename);

GList       *gnome_uri_list_extract_filenames   (const gchar* uri_list);
GList       *gnome_uri_list_extract_uris        (const gchar* uri_list);
void         gnome_uri_list_free_strings        (GList *list);

END_GNOME_DECLS

#endif

