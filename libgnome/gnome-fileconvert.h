#ifndef __GNOME_FILECONVERT_H__
#define __GNOME_FILECONVERT_H__ 1

BEGIN_GNOME_DECLS

/* Returns -1 if no conversion is possible */
gint
gnome_file_convert_fd(gint fd, gchar *fromtype, gchar *totype);
/* Convenience wrapper for the above function */
gint
gnome_file_convert(gchar *filename, gchar *fromtype, gchar *totype);

END_GNOME_DECLS

#endif /* __GNOME_FILECONVERT_H__ */
