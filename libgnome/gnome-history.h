#ifndef __GNOME_HISTORY_H__
#define __GNOME_HISTORY_H__ 1

BEGIN_GNOME_DECLS

struct _GnomeHistoryEntry {
  char *filename, *filetype, *creator, *desc;
};
typedef struct _GnomeHistoryEntry * GnomeHistoryEntry;

void
gnome_history_recently_used(GnomeHistoryEntry ent);

GList * /* GList of GnomeHistoryEntrys */
gnome_history_get_recently_used(void);

/* Convenience function */
void
gnome_history_free_recently_used_list(GList *alist /* From the above */);

END_GNOME_DECLS

#endif /* __GNOME_HISTORY_H__ */
