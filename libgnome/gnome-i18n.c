#include "libgnome.h"
#include "gnome-i18n.h"

const char *gnome_i18n_get_language(void)
{
  return getenv("LANG");
}
