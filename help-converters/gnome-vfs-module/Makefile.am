INCLUDES =						\
	$(LIBGNOME_CFLAGS)				

modules_LTLIBRARIES=libvfs-help.la

module_flags = -export_dynamic -avoid-version
modulesdir   = $(libdir)/gnome-vfs-2.0/modules

libvfs_help_la_SOURCES = 				\
	help-method.c		help-method.h		

libvfs_help_la_LDFLAGS = $(module_flags)

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA = help-methods.conf

EXTRA_DIST=help-methods.conf
