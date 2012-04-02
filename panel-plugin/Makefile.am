bin_PROGRAMS = xfce4-popup-places

xfce4_popup_places_SOURCES =						\
	xfce4-popup-places.c						\
	xfce4-popup-places.h

xfce4_popup_places_CFLAGS =						\
	$(LIBX11_CFLAGS)						\
	$(EXO_CFLAGS)							\
	$(GTK_CFLAGS)

xfce4_popup_places_LDADD =						\
	$(LIBX11_LDFLAGS)						\
	$(LIBX11_LIBS)							\
	$(EXO_LIBS)							\
	$(GTK_LIBS)



plugindir = $(libexecdir)/xfce4/panel-plugins

plugin_PROGRAMS =							\
	xfce4-places-plugin

if HAVE_LIBNOTIFY
xfce4_places_plugin_notify_sources =					\
	model_volumes_notify.c						\
	model_volumes_notify.h
endif

xfce4_places_plugin_SOURCES =						\
	$(xfce4_places_plugin_notify_sources)				\
	places.c							\
	places.h							\
	support.c							\
	support.h							\
	view.c								\
	view.h								\
	cfg.c								\
	cfg.h								\
	model.c								\
	model.h								\
	model_system.c							\
	model_system.h							\
	model_user.c							\
	model_user.h							\
	model_volumes.c							\
	model_volumes.h							\
	button.c							\
	button.h



xfce4_places_plugin_CFLAGS =						\
	-I$(top_builddir)						\
	-I$(top_srcdir)							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(LIBNOTIFY_CFLAGS)						\
	$(LIBX11_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(LIBXFCE4UI_CFLAGS)						\
	$(LIBXFCE4PANEL_CFLAGS)						\
	$(EXO_CFLAGS)							\
	$(PLATFORM_CFLAGS)

xfce4_places_plugin_LDADD =						\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(GTK_LIBS)							\
	$(LIBNOTIFY_LIBS)						\
	$(LIBX11_LDFLAGS)						\
	$(LIBX11_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(LIBXFCE4UI_LIBS)						\
	$(LIBXFCE4PANEL_LIBS)						\
	$(LIBX11_LIBS)							\
	$(EXO_LIBS)


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
