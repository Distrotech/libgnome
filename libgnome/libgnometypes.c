#include <config.h>
#include <gobject/genums.h>
#include <gobject/gboxed.h>
#include "libgnome.h"

#include "libgnometypebuiltins.h"
#include "libgnometypebuiltins_vars.c"
#include "libgnometypebuiltins_evals.c"
void libgnome_type_init(void);

void
libgnome_type_init(void) {
  static gboolean initialized = FALSE;

  if (!initialized) {
    int i;

    static struct {
      gchar           *type_name;
      GType           *type_id;
      GType            parent;
      gconstpointer    pointer1;
      gconstpointer    pointer2;
      gconstpointer    pointer3;
      gboolean         boolean1;
    } builtin_info[GNOME_TYPE_NUM_BUILTINS + 1] = {
#include "libgnometypebuiltins_ids.c"
      { NULL }
    };

    initialized = TRUE;

    g_type_init (G_TYPE_DEBUG_NONE);

    for (i = 0; i < GNOME_TYPE_NUM_BUILTINS; i++)
      {
	GType type_id = G_TYPE_INVALID;
	g_assert (builtin_info[i].type_name != NULL);
	if ( builtin_info[i].parent == G_TYPE_ENUM )
	  type_id = g_enum_register_static (builtin_info[i].type_name, (GEnumValue *)builtin_info[i].pointer1);
	else if ( builtin_info[i].parent == G_TYPE_FLAGS )
	  type_id = g_flags_register_static (builtin_info[i].type_name, (GFlagsValue *)builtin_info[i].pointer1);
	else if ( builtin_info[i].parent == G_TYPE_BOXED )
	  type_id = g_boxed_type_register_static (builtin_info[i].type_name, (GBoxedInitFunc)builtin_info[i].pointer1, (GBoxedCopyFunc)builtin_info[i].pointer2, (GBoxedFreeFunc)builtin_info[i].pointer3, builtin_info[1].boolean1);

	g_assert (type_id != G_TYPE_INVALID);
	(*builtin_info[i].type_id) = type_id;
      }
  }
}

