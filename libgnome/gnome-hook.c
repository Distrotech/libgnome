/*
 * Hook management routines.
 * Copyright (C) 1994, 1995, 1996 the Free Software Foundation.
 * Written 1994, 1995, 1996 by:
 * Miguel de Icaza, Janne Kukonlehto, Dugan Porter,
 * Jakub Jelinek
 */
#include <config.h>
#include <glib.h>
#include "gnome-defs.h"
#include "gnome-hook.h"

void
gnome_add_hook (GnomeHook **hook_list, GnomeHookFunc func, void *data)
{
	GnomeHook *new_hook = g_malloc (sizeof (GnomeHook));
	
	new_hook->hook_fn   = func;
	new_hook->hook_data = data;
	new_hook->next      = *hook_list;
	
	*hook_list = new_hook;
}

void
gnome_execute_hooks (GnomeHook *hook_list)
{
	GnomeHook *new_hook = NULL;
	GnomeHook *p;
	
	/* 
	 * We copy the hook list first so tahat we let the hook
	 * function call delete_hook
	 */
	
	while (hook_list) {
		gnome_add_hook (&new_hook, hook_list->hook_fn, hook_list->hook_data);
		hook_list = hook_list->next;
	}
	
	p = new_hook;
	
	while (new_hook) {
		(*new_hook->hook_fn) (new_hook->hook_data);
		new_hook = new_hook->next;
	}
	
	for (hook_list = p; hook_list;) {
		p = hook_list;
		hook_list = hook_list->next;
		g_free (p);
	}
}


void
gnome_delete_hook (GnomeHook **hook_list, GnomeHookFunc func)
{
	GnomeHook *current, *new_list, *next;
	
	new_list = NULL;
	
	for (current = *hook_list; current; current = next) {
		next = current->next;
		
		if (current->hook_fn == func)
			g_free (current);
		else
			gnome_add_hook (&new_list, current->hook_fn, current->hook_data);
	}
	
	*hook_list = new_list;
}

int
gnome_hook_present (GnomeHook *hook_list, GnomeHookFunc func)
{
	GnomeHook *p;
	
	for (p = hook_list; p; p = p->next)
		if (p->hook_fn == func)
			return TRUE;

	return FALSE;
}
