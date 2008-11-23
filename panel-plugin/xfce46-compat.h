#ifndef _XFCE46_COMPAT
#define _XFCE46_COMPAT

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_LIBXFCE4PANEL_46

#include <gtk/gtk.h>

void                 xfce_panel_plugin_position_menu        (GtkMenu          *menu,
                                                             gint             *x,
                                                             gint             *y,
                                                             gboolean         *push_in,
                                                             gpointer          panel_plugin);
#endif
#endif
