schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = \
	desktop_gnome_applications_browser.schemas		\
	desktop_gnome_applications_editor.schemas		\
	desktop_gnome_applications_help_viewer.schemas		\
	desktop_gnome_applications_terminal.schemas		\
	desktop_gnome_applications_window_manager.schemas	\
	desktop_gnome_file_views.schemas			\
	desktop_gnome_interface.schemas				\
	desktop_gnome_peripherals_mouse.schemas			\
	desktop_gnome_peripherals_keyboard.schemas		\
	desktop_gnome_background.schemas			\
	desktop_gnome_url_handlers.schemas

# don't do this if we are building in eg. rpm
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$$p; \
		done \
	fi

EXTRA_DIST = $(schema_DATA)
