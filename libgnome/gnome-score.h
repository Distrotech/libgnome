#ifndef __GNOME_SCORE_H__
#define __GNOME_SCORE_H__ 1

#include <time.h>
#include <glib.h>
#include <glib.h>

BEGIN_GNOME_DECLS
/*
 * gnome_score_init()
 * creates a child process with which we communicate through a pair of pipes,
 * then drops privileges.
 * this should be called as the first statement in main().
 * returns 0 on success, drops privs and returns -1 on failure
 */

gint
gnome_score_init (const gchar * gamename);

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
