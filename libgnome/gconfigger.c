/* by Elliot Lee. */

#include <stdio.h>
#include <string.h>
#include <libgnome.h>

static void
do_option(poptContext ctx, enum poptCallbackReason reason,
	  const struct poptOption *opt,
	  const char *arg, void *data)
{
  char *ctmp;
  char **catmp;

  switch(opt->val) {
  case 'g':
    ctmp = gnome_config_get_string(arg);
    printf("%s\n", ctmp);
    g_free(ctmp);
    break;
  case 's':
    catmp = g_strsplit(arg, ",", 2);
    if(catmp && catmp[0] && catmp[1])
      gnome_config_set_string(catmp[0], catmp[1]);
    g_strfreev(catmp);
    break;
  }
}

struct poptOption options[] = {
  {NULL, '\0', POPT_ARG_CALLBACK, do_option, 0, NULL, NULL},
  {N_("get"), 'g', POPT_ARG_STRING, NULL, 'g', N_("Get config entry"), N_("CONFIG-PATH")},
  {N_("set"), 's', POPT_ARG_STRING, NULL, 's', N_("Set config entry"), N_("CONFIG-PATH")},
  POPT_AUTOHELP
  {NULL, '\0', 0, NULL, 0}
};

#define USAGE() printf("Usage: config-access get|set <config path>\n")

int main(int argc, char *argv[])
{
  gnomelib_register_popt_table(options, "config-access options");
  gnomelib_init("config-access", VERSION);
  poptFreeContext(gnomelib_parse_args(argc, argv, 0));

  gnome_config_sync();

  return 0;
}
