#ifndef __GNOME_SCORE_H__
#define __GNOME_SCORE_H__ 1

#include <time.h>

BEGIN_GNOME_DECLS
/* Returns the position in the top-ten starting from 1, or 0 if it isn't in the table */
gint
gnome_score_log(gfloat score,
		gchar *level, /* Pass in NULL unless you want to keep
				 per-level scores for the game */
		/* Pass in TRUE if higher scores are "better"
		   in the game */
		gboolean higher_to_lower_score_order);


gint /* Returns number of items in the arrays */
gnome_score_get_notable(gchar *gamename, /* Will be auto-determined if NULL */
			gchar *level,
			gchar ***names,
			gfloat **scores,
			time_t **scoretimes);
END_GNOME_DECLS

#endif /* __GNOME_SCORE_H__ */
