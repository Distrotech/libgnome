#ifndef __GNOME_FILECONVERT_H__
#define __GNOME_FILECONVERT_H__ 1

/* WARNING ____ IMMATURE API ____ liable to change */

BEGIN_GNOME_DECLS

/* Returns -1 if no conversion is possible */
gint
gnome_file_convert_fd(gint fd, const char *fromtype, const char *totype);
/* Convenience wrapper for the above function */
gint
gnome_file_convert(const char *filename, const char *fromtype, const char *totype);

END_GNOME_DECLS

#endif /* __GNOME_FILECONVERT_H__ */
