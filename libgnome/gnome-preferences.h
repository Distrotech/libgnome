#ifndef GNOME_PREFERENCES_H
#define GNOME_PREFERENCES_H

#include <glib.h>

G_BEGIN_DECLS

/* Whether to use the statusbar instead of dialogs when possible:
   TRUE means use the statusbar */
gboolean          gnome_preferences_get_statusbar_dialog     (void);
void              gnome_preferences_set_statusbar_dialog     (gboolean statusbar);

/* Whether the statusbar can be used for interactive questions 
   TRUE means the statusbar is interactive */
gboolean          gnome_preferences_get_statusbar_interactive(void);
void              gnome_preferences_set_statusbar_interactive(gboolean b);

/* Whether the AppBar progress meter goes on the right or left */
gboolean          gnome_preferences_get_statusbar_meter_on_right (void);
void              gnome_preferences_set_statusbar_meter_on_right (gboolean status_meter_on_right);


/* Whether menubars can be detached */
gboolean          gnome_preferences_get_menubar_detachable   (void);
void              gnome_preferences_set_menubar_detachable   (gboolean b);

/* Whether menubars have a beveled edge */
gboolean          gnome_preferences_get_menubar_relief       (void);
void              gnome_preferences_set_menubar_relief       (gboolean b);

/* Whether toolbars can be detached */
gboolean          gnome_preferences_get_toolbar_detachable   (void);
void              gnome_preferences_set_toolbar_detachable   (gboolean b);

/* Whether toolbars have a beveled edge  */
gboolean          gnome_preferences_get_toolbar_relief       (void);
void              gnome_preferences_set_toolbar_relief       (gboolean b);

/* Whether toolbar buttons have a beveled edge */
gboolean          gnome_preferences_get_toolbar_relief_btn   (void);
void              gnome_preferences_set_toolbar_relief_btn   (gboolean b);

/* Whether toolbars show lines in separators  */
gboolean          gnome_preferences_get_toolbar_lines        (void);
void              gnome_preferences_set_toolbar_lines        (gboolean b);

/* Whether toolbars show labels  */
gboolean          gnome_preferences_get_toolbar_labels       (void);
void              gnome_preferences_set_toolbar_labels       (gboolean b);

/* Whether to try to center dialogs over their parent window.
   If it's possible, dialog_position is ignored. If not,
   fall back to dialog_position. */
gboolean          gnome_preferences_get_dialog_centered      (void);
void              gnome_preferences_set_dialog_centered      (gboolean b);

/* Whether menus can be torn off */
gboolean          gnome_preferences_get_menus_have_tearoff   (void);
void              gnome_preferences_set_menus_have_tearoff   (gboolean b);

/* Whether menu items have icons in them or not */
int               gnome_preferences_get_menus_have_icons (void);
void              gnome_preferences_set_menus_have_icons (int have_icons);

G_END_DECLS

#endif
