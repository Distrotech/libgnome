/* GnomePaper
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Dirk Luetjens <dirk@luedi.oche.de>
 * a few code snippets taken from libpaper written by 
 * Yves Arrouye <Yves.Arrouye@marin.fdn.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "libgnomeP.h"

struct _Paper {
  char* name;
  double pswidth, psheight;
  double lmargin, tmargin, rmargin, bmargin;
};

struct _Unit {
  char* name;
  char* unit;
  float factor;
};

/*
const Paper*	paper_info		(gchar *papername);
const char*	paper_name		(const Paper*);
double		paper_ps_width		(const Paper*);
double		paper_ps_height		(const Paper*);

Paper*		paper_with_size		(const double pswidth, 
					 const double psheight);
double		paper_width_with_unit	(const Paper* paper, 
					 const Unit* unit);
double		paper_height_with_unit	(const Paper* spaper, 
					 const Unit* sunit);

gchar*		systempapername		(void);

const Unit*	unit_info		(gchar* unitname);
*/

static const Unit units[] = {
  /*
  { "Inch",       "in",	1. },
  { "Feet",       "ft",	12. },
  { "Point",      "pt",	1. / 72. },
  { "Meter",      "m",  100. / 2.54 },
  { "Decimeter",  "dm",	10. / 2.54 },
  { "Centimeter", "cm",	1. / 2.54 },
  { "Millimeter", "mm",	.1 / 2.54 },
  { 0 }
  */

  { "Inch",       "in",	1. / 72. },
  { "Feet",       "ft",	1. / (72. * 12.) },
  { "Point",      "pt",	1. },
  { "Meter",      "m",  100. / 2.54 }, /* XXX fix metric */
  { "Decimeter",  "dm",	10. / 2.54 },
  { "Centimeter", "cm",	1. / 2.54 },
  { "Millimeter", "mm",	.1 / 2.54 },
  { 0 }
};

static GList* paper_list = NULL;
static GList* unit_list = NULL;
static GList* paper_name_list = NULL;
static GList* unit_name_list = NULL;

static void
paper_init (void)
{
  void	*config_iterator;
  gchar *name, *size;
  Paper	*paper;
  const Unit *unit;
  char *str;

  config_iterator =
    gnome_config_init_iterator("="GNOMESYSCONFDIR"/paper.config=/Paper/");
  
  if (!config_iterator)
    return;

  while ((config_iterator =
	  gnome_config_iterator_next(config_iterator, &name, &size)))
    {
      paper = g_new (Paper, 1);

      paper->name = name;
      g_strdelimit (size, "{},", ' ');
      paper->pswidth  = g_strtod (size, &str);
      paper->psheight = g_strtod (str, &str);
      paper->lmargin = g_strtod (str, &str);
      paper->tmargin = g_strtod (str, &str);
      paper->rmargin = g_strtod (str, &str);
      paper->bmargin = g_strtod (str, NULL);
      g_free(size);

      paper_list = g_list_prepend(paper_list, paper);
      paper_name_list = g_list_prepend(paper_name_list, paper->name);
    }

  for (unit=units; unit->name; unit++) {
    unit_list = g_list_prepend(unit_list, (gpointer) unit);
    unit_name_list = g_list_prepend(unit_name_list, unit->name);
  }
}

static int 
paper_name_compare (const Paper* a, const gchar *b)
{
  return (g_strcasecmp(a->name, b));
}

static int 
unit_name_compare (const Unit* a, const gchar *b)
{
  return (g_strcasecmp(a->name, b));
}




GList*
gnome_paper_name_list (void)
{
  if (!paper_name_list)
    paper_init();
  return paper_name_list;
}

const Paper*
gnome_paper_with_name (const gchar *papername)
{
  GList	*l;
  
  if (!paper_list)
    paper_init();

  l = g_list_find_custom (paper_list, (gpointer) papername, (GCompareFunc)paper_name_compare);

  return l ? l->data : NULL;
}

const Paper*
gnome_paper_with_size (const double pswidth, const double psheight)
{
  GList *l = paper_list;
  Paper	*pp;

  if (!paper_list)
    paper_init();

  while (l) {
    pp = l->data;
    if (pp->pswidth == pswidth && pp->psheight == psheight)
      return pp;
    l = l->next;
  }
  return NULL;
}

const gchar*
gnome_paper_name_default(void)
{
  static gchar *systempapername = "a4";
  
  return systempapername;
}

const gchar*
gnome_paper_name (const Paper *paper)
{
  g_return_val_if_fail(paper, NULL);
  
  return paper->name;
}

gdouble
gnome_paper_pswidth (const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->pswidth;
}

gdouble
gnome_paper_psheight (const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->psheight;
}

gdouble
gnome_paper_lmargin	(const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->lmargin;
}

gdouble
gnome_paper_tmargin	(const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->tmargin;
}

gdouble
gnome_paper_rmargin	(const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->rmargin;
}

gdouble
gnome_paper_bmargin	(const Paper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->bmargin;
}

GList*
gnome_unit_name_list (void)
{
  if (!unit_name_list)
    paper_init();
  return unit_name_list;
}

const Unit* 
gnome_unit_with_name(const gchar* unitname)
{
  GList	*l;
  
  if (!unit_list)
    paper_init();

  l = g_list_find_custom (unit_list, (gpointer) unitname, (GCompareFunc)unit_name_compare);

  return l ? l->data : NULL;
}

double 
gnome_paper_convert (double psvalue, const Unit* unit)
{
  g_return_val_if_fail(unit, psvalue);
  
  /*return psvalue * unit->factor * 72;*/
  return psvalue * unit->factor;
}
