#ifndef GNOME_TRIGGERSP_H
#define GNOME_TRIGGERSP_H 1

/* Yes, this mechanism is lame, that's why it's hidden :) */
typedef struct _GnomeTriggerList GnomeTriggerList;

struct _GnomeTriggerList {
  char *nodename;
  GnomeTriggerList **subtrees;
  gint numsubtrees;
  GnomeTrigger **actions;
  gint numactions;
};

#endif
