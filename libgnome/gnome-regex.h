/* gnome-regex.h - Regular expression cache object.

   Copyright (C) 1998 Tom Tromey
   All rights reserved.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

#ifndef GNOME_REGEX_H
#define GNOME_REGEX_H


G_BEGIN_DECLS

#include <sys/types.h>
#include <regex.h>

typedef struct _GnomeRegexCache GnomeRegexCache;
struct _GnomeRegexCache {
	char **regexs;		/* Regular expression strings.  */
	regex_t *patterns;	/* Compiled expressions.  */
	int size;		/* Total number of cache slots.  */
	int next;		/* Next available slot.  */
	/* FIXME: probably should cache compilation flags along with
	   regex and use those to determine caching.  For now we
	   assume that the flags never change.  Another option would
	   be to put the flag info into this structure and just not
	   let the user ever change it.  */
};

/* Create a new regular expression cache with default number of
   items.  */
GnomeRegexCache *gnome_regex_cache_new (void);

/* Free a regular expression cache.  */
void gnome_regex_cache_destroy (GnomeRegexCache *rxc);

/* Change number of cache slots.  */
void gnome_regex_cache_set_size (GnomeRegexCache *rxc, int new_size);

/* Compile a regular expression.  */
regex_t *gnome_regex_cache_compile (GnomeRegexCache *rxc, const char *pattern,
				    int flags);


G_END_DECLS

#endif /* GNOME_REGEX_H */
