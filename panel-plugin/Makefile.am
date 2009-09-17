#
# Popup
#

bin_PROGRAMS = xfce4-popup-places

xfce4_popup_places_SOURCES =						\
	xfce4-popup-places.c						\
	xfce4-popup-places.h

xfce4_popup_places_CFLAGS =						\
	$(LIBX11_CFLAGS)						\
	$(GTK_CFLAGS)

xfce4_popup_places_LDADD =						\
	$(LIBX11_LDFLAGS)						\
	$(LIBX11_LIBS)							\
	$(GTK_LIBS)


#
# Vala sources
#

xfce4_places_plugin_VAPISOURCES =					\
	config.vapi							\
	view.vapi

xfce4_places_plugin_c_VALASOURCES =					\
	places.vala

xfce4_places_plugin_ch_VALASOURCES =					\
	button.vala

xfce4_places_plugin_VALASOURCES =					\
	$(xfce4_places_plugin_c_VALASOURCES)				\
	$(xfce4_places_plugin_ch_VALASOURCES)


xfce4_places_plugin_VALABUILTSOURCES =					\
	$(xfce4_places_plugin_VALASOURCES:.vala=.c)			\
	$(xfce4_places_plugin_ch_VALASOURCES:.vala=.h)

if MAINTAINER_MODE
BUILT_SOURCES = vala.stamp
$(xfce4_places_plugin_VALABUILTSOURCES): vala.stamp
VALA_OPTS = --vapidir=$(srcdir) --pkg=libxfce4panel-1.0 --pkg=config --pkg=view
vala.stamp: $(xfce4_places_plugin_VALASOURCES)
	for vala_file in $(xfce4_places_plugin_ch_VALASOURCES); do \
		header_file=$$( echo $$vala_file | sed s/\.vala$$/.h/ ); \
		$(VALAC) $(VALA_OPTS) -H $$header_file -C $$vala_file; \
	done
	for vala_file in $(xfce4_places_plugin_c_VALASOURCES); do \
		$(VALAC) $(VALA_OPTS) -C $$vala_file; \
	done
	sed -i 's/^\s*gpointer boxed;\s*$$//' $(xfce4_places_plugin_VALASOURCES:.vala=.c) # gnome bug 592166
	touch $@
endif


#
# Plugin
#

plugindir = $(libexecdir)/xfce4/panel-plugins

plugin_PROGRAMS =							\
	xfce4-places-plugin

xfce4_places_plugin_SOURCES =						\
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
	xfce46-compat.c							\
	xfce46-compat.h							\
	$(xfce4_places_plugin_VALABUILTSOURCES)


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

#
# Extra
#

EXTRA_DIST =								\
	$(xfce4_places_plugin_VAPISOURCES)				\
	$(xfce4_places_plugin_VALASOURCES)				\
	$(desktop_in_in_files)

if MAINTAINER_MODE
CLEANFILES =								\
	$(BUILT_SOURCES)						\
	$(xfce4_places_plugin_VALABUILTSOURCES)
endif

DISTCLEANFILES =								\
	$(desktop_in_files)						\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
