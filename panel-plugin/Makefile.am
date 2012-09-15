
plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libplaces.la

if HAVE_LIBNOTIFY
libplaces_la_notify_sources =					\
	model_volumes_notify.c						\
	model_volumes_notify.h
endif

libplaces_la_SOURCES =						\
	$(libplaces_la_notify_sources)				\
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



libplaces_la_CFLAGS =							\
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
	$(XFCONF_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libplaces_la_LIBADD =							\
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
	$(EXO_LIBS)							\
	$(XFCONF_LIBS)

libplaces_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

#
# xfce4-popup-places script
#
bin_SCRIPTS = \
	xfce4-popup-places

xfce4-popup-places: xfce4-popup-places.sh Makefile
	$(AM_V_GEN) $(SED) -e "s,\@bindir\@,$(bindir),g" \
	-e "s,\@localedir\@,$(localedir),g" $< >$@

#
# Desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = places.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_files) \
	xfce4-popup-places.sh

CLEANFILES = $(desktop_DATA) \
	xfce4-popup-places

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
