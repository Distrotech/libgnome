/*
 * Handles all of the internationalization configuration options.
 * Author: Tom Tromey <tromey@creche.cygnus.com>
 */

#ifndef __GNOME_I18N_H__
#define __GNOME_I18N_H__

BEGIN_GNOME_DECLS

#ifdef HAVE_LIBINTL_H
#    include <libintl.h>
#    define _(String) gettext (String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif

END_GNOME_DECLS

#endif /* __GNOME_UTIL_H__ */
