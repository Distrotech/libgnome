

schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = \
	desktop_gnome_accessibility_keyboard.schemas.in		\
	desktop_gnome_accessibility_startup.schemas.in		\
	desktop_gnome_applications_at_visual.schemas.in		\
	desktop_gnome_applications_at_mobility.schemas.in	\
	desktop_gnome_applications_browser.schemas.in		\
	desktop_gnome_applications_help_viewer.schemas.in	\
	desktop_gnome_applications_terminal.schemas.in		\
	desktop_gnome_applications_window_manager.schemas.in	\
	desktop_gnome_background.schemas.in			\
	desktop_gnome_file_views.schemas.in			\
	desktop_gnome_interface.schemas.in			\
	desktop_gnome_lockdown.schemas.in			\
	desktop_gnome_peripherals_mouse.schemas.in		\
	desktop_gnome_peripherals_keyboard.schemas.in		\
	desktop_gnome_sound.schemas.in				\
	desktop_gnome_typing_break.schemas.in			\
	desktop_gnome_thumbnailers.schemas.in

desktop_gnome_background.schemas.in: desktop_gnome_background.schemas.in.in Makefile
	sed -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/desktop_gnome_background.schemas.in.in > desktop_gnome_background.schemas.in.tmp \
	&& mv desktop_gnome_background.schemas.in.tmp desktop_gnome_background.schemas.in

schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

# get rid of schemas during clean
CLEANFILES = $(schema_DATA) desktop_gnome_interface.schemas.in desktop_gnome_background.schemas.in

if GCONF_SCHEMAS_INSTALL
# don't do this if we are building in eg. rpm
if OS_WIN32
# Work around MSYS feature. If we pass GCONF_CONFIG_SOURCE as an
# environment variable, MSYS is "helpful" and thinks the colons in the
# variable means that is is a search path, and changes the colons into
# semicolons and slashes into backslashes. Usually this feature is
# useful, but not this time.
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			(echo set GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE); \
			 echo $(GCONFTOOL) --makefile-install-rule $$p) >_temp.bat; \
			cmd /c _temp.bat; \
		done \
	fi
else
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
endif
else
install-data-local:
endif

noinst_DATA = desktop_gnome_background.schemas.in
EXTRA_DIST = $(schema_in_files) $(schema_DATA) desktop_gnome_interface.schemas.in desktop_gnome_background.schemas.in.in
