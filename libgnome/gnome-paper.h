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

#ifndef GNOME_PAPER_H
#define GNOME_PAPER_H

#include <glib.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

typedef struct _GnomePaper GnomePaper;
typedef struct _GnomeUnit  GnomeUnit;
	
/* These definitions are deprecated, they will be gone in the next
 * version of GNOME, 
 */
typedef GnomePaper Paper;
typedef GnomeUnit Unit;

GList*		 gnome_paper_name_list	(void);
const GnomePaper *gnome_paper_with_name	(const gchar *papername);
const GnomePaper *gnome_paper_with_size	(double pswidth, double psheight);

const gchar	 *gnome_paper_name	(const GnomePaper *paper);
gdouble		 gnome_paper_pswidth	(const GnomePaper *paper);
gdouble		 gnome_paper_psheight	(const GnomePaper *paper);
gdouble		 gnome_paper_lmargin	(const GnomePaper *paper);
gdouble		 gnome_paper_tmargin	(const GnomePaper *paper);
gdouble		 gnome_paper_rmargin	(const GnomePaper *paper);
gdouble		 gnome_paper_bmargin	(const GnomePaper *paper);

const gchar	 *gnome_paper_name_default	(void);

GList*		 gnome_unit_name_list	(void);
const GnomeUnit	 *gnome_unit_with_name	(const gchar *unitname);

gdouble		 gnome_paper_convert	(double psvalue, const GnomeUnit *unit);
double           gnome_paper_convert_to_points (double othervalue, const GnomeUnit *unit);

END_GNOME_DECLS

#endif 
