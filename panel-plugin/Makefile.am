plugindir = $(libexecdir)/xfce4/panel-plugins

plugin_PROGRAMS =							\
	xfce4-places-plugin

xfce4_places_plugin_SOURCES =						\
	places.c							\
	places.h							\
	model.c								\
	model.h								\
	model_system.c							\
	model_system.h							\
	model_volumes.c							\
	model_user.c


xfce4_places_plugin_CFLAGS =						\
	-I$(top_builddir)						\
	-I$(top_srcdir)							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GTK_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
        $(THUNAR_VFS_CFLAGS)						\
	$(PLATFORM_CFLAGS)

xfce4_places_plugin_LDADD =						\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCE4PANEL_LIBS)						\
	@THUNAR_VFS_LIBS@


#
# Desktop file
#
desktopdir =								\
	$(datadir)/xfce4/panel-plugins

desktop_in_in_files =							\
	places.desktop.in.in

desktop_in_files =							\
	$(desktop_in_in_files:.desktop.in.in=.desktop.in)

%.desktop.in: %.desktop.in.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =								\
	$(desktop_in_in_files)

CLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
