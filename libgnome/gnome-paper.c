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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include "libgnomeP.h"

struct _GnomePaper
{
  char* name;
  double pswidth, psheight;
  double lmargin, tmargin, rmargin, bmargin;
};

struct _GnomeUnit
{
  char* name;
  char* unit;
  float factor;
};


static const GnomeUnit units[] =
{
  /* XXX does anyone *really* measure paper size in feet?  meters? */

  /* human name, abreviation, points per unit */
  { "Feet",       "ft",	864 },
  { "Meter",      "m",  2834.6457 },
  { "Decimeter",  "dm",	283.46457 },
  { "Millimeter", "mm",	2.8346457 },
  { "Point",      "pt",	1. },
  { "Centimeter", "cm",	28.346457 },
  { "Inch",       "in",	72 },
  { 0 }
};

static GList* paper_list = NULL;
static GList* unit_list = NULL;
static GList* paper_name_list = NULL;
static GList* unit_name_list = NULL;

/**
 * paper_init: initialize paper data structures
 * 
 **/
static void
paper_init (void)
{
  void	*config_iterator;
  gchar *name, *size;
  GnomePaper	*paper;
  const GnomeUnit *unit;
  char *str;

  gnome_config_push_prefix (NULL);
  
  config_iterator =
    gnome_config_init_iterator("="GNOMESYSCONFDIR"/paper.config=/Paper/");
  gnome_config_pop_prefix ();
  
  if (!config_iterator)
    return;

  while ((config_iterator =
	  gnome_config_iterator_next(config_iterator, &name, &size)))
    {
      paper = g_new (GnomePaper, 1);

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

  for (unit=units; unit->name; unit++)
    {
      unit_list = g_list_prepend(unit_list, (gpointer) unit);
      unit_name_list = g_list_prepend(unit_name_list, unit->name);
    }
}

static int 
paper_name_compare (const GnomePaper* a, const gchar *b)
{
  return (g_strcasecmp(a->name, b));
}

static int 
unit_name_compare (const GnomeUnit* a, const gchar *b)
{
  return (g_strcasecmp(a->name, b));
}


/**
 * gnome_paper_name_list: get internal list of paper specifications
 * 
 * grants access to the hardcoded internal list of paper specifications
 *
 * Returns: internal list of paper specifications
 **/
GList*
gnome_paper_name_list (void)
{
  if (!paper_name_list)
    paper_init();
  return paper_name_list;
}

/**
 * gnome_paper_with_name: get paper specification by name
 * @papername: human name of desired paper type
 * 
 * searches internal list of paper sizes, searching
 * for one with the name 'papername'
 * 
 * Returns: paper specification with given name, or NULL
 **/
const GnomePaper*
gnome_paper_with_name (const gchar *papername)
{
  GList	*l;
  
  if (!paper_list)
    paper_init();

  l = g_list_find_custom (paper_list,
			  (gpointer) papername,
			  (GCompareFunc)paper_name_compare);

  return l ? l->data : NULL;
}

/**
 * gnome_paper_with_size: create paper specification with size
 * @pswidth: width of paper
 * @psheight: height of paper
 * 
 * create a custom paper type with given dimensions
 * 
 * Returns: paper specification
 **/
const GnomePaper*
gnome_paper_with_size (double pswidth, double psheight)
{
  GList *l = paper_list;
  GnomePaper	*pp;

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

/**
 * gnome_paper_name_default: get the name of the default paper
 * 
 * Returns: human readable name for default paper type
 **/
const gchar*
gnome_paper_name_default(void)
{
  static gchar *systempapername = "a4";
  
  return systempapername;
}

/**
 * gnome_paper_name: get name of paper
 * @paper: paper specification
 * 
 * 
 * 
 * Returns: human readable name for paper type
 **/
const gchar*
gnome_paper_name (const GnomePaper *paper)
{
  g_return_val_if_fail(paper, NULL);
  
  return paper->name;
}

/**
 * gnome_paper_pswidth: get width of paper
 * @paper: paper specification
 * 
 * returns the width of the paper, including the margins
 * 
 * Returns: width of paper (in points)
 **/
gdouble
gnome_paper_pswidth (const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->pswidth;
}

/**
 * gnome_paper_psheight: get height of paper
 * @paper: paper specification
 * 
 * returns the height of the paper, including the margins
 * 
 * Returns: height of paper (in points)
 **/
gdouble
gnome_paper_psheight (const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->psheight;
}

/**
 * gnome_paper_lmargin: get size of left margin
 * @paper: paper specification
 * 
 * 
 * 
 * Returns: paper specification
 **/
gdouble
gnome_paper_lmargin	(const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->lmargin;
}

/**
 * gnome_paper_tmargin: get size of top margin
 * @paper: paper specification
 * 
 * 
 * 
 * Returns: size of top margin (in points)
 **/
gdouble
gnome_paper_tmargin	(const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->tmargin;
}

/**
 * gnome_paper_rmargin: get size of right margin
 * @paper: paper specification
 * 
 * 
 * 
 * Returns: size of right margin (in points)
 **/
gdouble
gnome_paper_rmargin	(const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->rmargin;
}

/**
 * gnome_paper_bmargin: get size of bottom margin
 * @paper: paper specification
 * 
 * 
 * 
 * Returns: size of bottom margin (in points)
 **/
gdouble
gnome_paper_bmargin (const GnomePaper *paper)
{
  g_return_val_if_fail(paper, 0.0);
  
  return paper->bmargin;
}

/**
 * gnome_unit_name_list: get internal list of units
 * 
 * grants access to the hardcoded internal list
 * of units
 * 
 * Returns: internal list of units
 **/
GList*
gnome_unit_name_list (void)
{
  if (!unit_name_list)
    paper_init();
  return unit_name_list;
}

/**
 * gnome_unit_with_name: get Unit by name
 * @unitname: name of desired unit
 * 
 * searches internal list of units, searching
 * for one with the name 'unitname'
 * 
 * Returns: Unit with given name or NULL
 **/
const GnomeUnit * 
gnome_unit_with_name (const gchar *unitname)
{
  GList	*l;
  
  if (!unit_list)
    paper_init();

  l = g_list_find_custom (unit_list,
			  (gpointer) unitname,
			  (GCompareFunc)unit_name_compare);

  return l ? l->data : NULL;
}

/**
 * gnome_paper_convert: convert from points to other units
 * @psvalue: value in points
 * @unit: unit to convert to
 * 
 * converts from value represented in points
 * to value represented in given units.
 * 
 * Returns: value in given units
 **/
double 
gnome_paper_convert (double psvalue, const GnomeUnit *unit)
{
  g_return_val_if_fail (unit, psvalue);
  g_return_val_if_fail (unit->factor, psvalue);

  return psvalue / unit->factor;
}

/**
 * gnome_paper_convert_to_points: convert from other units to points.
 * @othervalue: value in points
 * @unit: unit to convert to
 * 
 * Needed for gnome-paper-selector.
 * converts from value represented in points
 * to value represented in given units.
 * 
 * Returns: value in points
 **/
double 
gnome_paper_convert_to_points (double othervalue, const GnomeUnit *unit)
{
  g_return_val_if_fail (unit, othervalue);

  return othervalue * unit->factor;
}
