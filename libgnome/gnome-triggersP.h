#ifndef GNOME_TRIGGERSP_H
#define GNOME_TRIGGERSP_H 1

/* Yes, this mechanism is lame, that's why it's hidden :) */
typedef struct _GnomeTriggerList GnomeTriggerList;

struct _GnomeTriggerList {
  char *nodename;
  GnomeTriggerList **subtrees;
  GnomeTrigger **actions;
  gint numsubtrees;
  gint numactions;
};

#endif
