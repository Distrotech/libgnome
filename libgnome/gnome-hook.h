#ifndef GNOME_HOOK_H
#define GNOME_HOOK_H

BEGIN_GNOME_DECLS

typedef void (*GnomeHookFunc)(void *);

typedef struct GnomeHook {
	GnomeHookFunc    hook_fn;
	void             *hook_data;
	struct GnomeHook *next;
} GnomeHook;


void gnome_add_hook (GnomeHook **hook_list, GnomeHookFunc func, void *data);
void gnome_execute_hooks (GnomeHook *hook_list);
void gnome_delete_hook   (GnomeHook **hook_list, GnomeHookFunc func);
int  gnome_hook_present  (GnomeHook *hook_list, GnomeHookFunc func);

END_GNOME_DECLS

#endif
