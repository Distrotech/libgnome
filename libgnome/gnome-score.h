#ifndef __GNOME_SCORE_H__
#define __GNOME_SCORE_H__ 1

#include <time.h>

BEGIN_GNOME_DECLS
gboolean /* Returns TRUE if it was a notable (top ten) score */
gnome_score_log(gfloat score);

gint /* Returns number of items in the arrays */
gnome_score_get_notable(gchar *gamename, /* Will be auto-determined if NULL */
			gchar ***names,
			gfloat **scores,
			time_t **scoretimes);
END_GNOME_DECLS

#endif /* __GNOME_SCORE_H__ */
