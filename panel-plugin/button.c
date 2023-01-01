/*  xfce4-places-plugin
 *
 *  Provides the widget that sits on the panel
 *
 *  Note that, while this extends GtkToggleButton, much of the gtk_button_*()
 *  functions shouldn't be used.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
 *  
 *  Some code adapted from libxfce4panel for the togglebutton configuration
 *    (xfce-panel-convenience.c)
 *    Copyright (c) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 *  Some code adapted from gtk+ for the properties
 *    (gtkbutton.c)
 *    Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *    Modified by the GTK+ Team and others 1997-2001
 *
 *  May also contain code adapted from:
 *   - notes plugin
 *     panel-plugin.c - (xfce4-panel plugin for temporary notes)
 *     Copyright (c) 2006 Mike Massonnet <mmassonnet@gmail.com>
 *
 *   - xfdesktop menu plugin
 *     desktop-menu-plugin.c - xfce4-panel plugin that displays the desktop menu
 *     Copyright (C) 2004 Brian Tarricone, <bjt23@cornell.edu>
 *  
 *   - launcher plugin
 *     launcher.c - (xfce4-panel plugin that opens programs)
 *     Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *     Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <string.h>

#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>

#include "button.h"

#define BOX_SPACING 2

enum
{
    PROP_0,
    PROP_PIXBUF_FACTORY,
    PROP_LABEL
};

static void
places_button_dispose(GObject*);

static void
places_button_resize(PlacesButton*);

static void
places_button_mode_changed(XfcePanelPlugin*, XfcePanelPluginMode, PlacesButton*);

static gboolean
places_button_size_changed(XfcePanelPlugin*, gint size, PlacesButton*);

static void
places_button_theme_changed(PlacesButton*);

G_DEFINE_TYPE(PlacesButton, places_button, GTK_TYPE_TOGGLE_BUTTON);

void
places_button_set_label(PlacesButton *self, const gchar *label)
{
    g_assert(PLACES_IS_BUTTON(self));

    if (label == NULL && self->label_text == NULL)
        return;

    if (label != NULL && self->label_text != NULL &&
        strcmp(label, self->label_text) == 0) {
        return;
    }

    DBG("new label text: %s", label);

    if (self->label_text != NULL)
        g_free(self->label_text);
    
    self->label_text = g_strdup(label);

    places_button_resize(self);
}

void
places_button_set_pixbuf_factory(PlacesButton *self,
                                 places_button_image_pixbuf_factory *factory)
{
    g_assert(PLACES_IS_BUTTON(self));

    if (self->pixbuf_factory == factory)
        return;

    DBG("new pixbuf factory: %p", factory);
    self->pixbuf_factory = factory;

    places_button_resize(self);
}

static void
places_button_set_property(GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    PlacesButton *self;

    self = PLACES_BUTTON(object);

    switch (property_id) {

        case PROP_PIXBUF_FACTORY:
            places_button_set_pixbuf_factory(self, g_value_get_pointer(value));
            break;
        case PROP_LABEL:
            places_button_set_label(self, g_value_get_string(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

places_button_image_pixbuf_factory*
places_button_get_pixbuf_factory(PlacesButton *self)
{
    g_assert(PLACES_IS_BUTTON(self));

    DBG("returning %p", self->pixbuf_factory);
    return self->pixbuf_factory;
}


const gchar*
places_button_get_label(PlacesButton *self)
{
    g_assert(PLACES_IS_BUTTON(self));

    DBG("returning %s", self->label_text);
    return self->label_text;
}

static void
places_button_get_property(GObject      *object,
                           guint         property_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
    PlacesButton *self;

    self = PLACES_BUTTON(object);

    switch (property_id) {

        case PROP_PIXBUF_FACTORY:
            g_value_set_pointer(value, places_button_get_pixbuf_factory(self));
            break;

        case PROP_LABEL:
            g_value_set_string(value, places_button_get_label(self));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}



static void
places_button_class_init(PlacesButtonClass *klass)
{
    GObjectClass *gobject_class;
    
    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = places_button_dispose;
  
    gobject_class->set_property = places_button_set_property;
    gobject_class->get_property = places_button_get_property;

    g_object_class_install_property(gobject_class,
        PROP_LABEL,
        g_param_spec_string("label",
            "Label",
            "Button text",
            NULL,
            EXO_PARAM_READWRITE));

    g_object_class_install_property(gobject_class,
        PROP_PIXBUF_FACTORY,
        g_param_spec_object("pixbuf-factory",
            "Pixbuf factory",
            "Factory to create icons for image to appear next to button text",
            GTK_TYPE_WIDGET,
            EXO_PARAM_READWRITE));
}

static void
places_button_init(PlacesButton *self)
{
    self->plugin = NULL;
    self->box = NULL;
    self->label = NULL;
    self->image = NULL;
}

static void
places_button_construct(PlacesButton *self, XfcePanelPlugin *plugin)
{
    GtkOrientation orientation;
    GtkIconTheme *icon_theme;

    g_assert(XFCE_IS_PANEL_PLUGIN(plugin));

    g_object_ref(plugin);
    self->plugin = plugin;

    /* from libxfce4panel */
    gtk_widget_set_can_default (GTK_WIDGET(self), FALSE);
    gtk_widget_set_can_focus (GTK_WIDGET(self), FALSE);
    gtk_button_set_relief(GTK_BUTTON(self), GTK_RELIEF_NONE);
    gtk_widget_set_focus_on_click(GTK_WIDGET(self), FALSE);

    self->alignment = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign (self->alignment, GTK_ALIGN_START);
    gtk_widget_set_valign (self->alignment, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(self), self->alignment);
    gtk_widget_show(self->alignment);

    orientation = xfce_panel_plugin_get_orientation(self->plugin);
    self->box = gtk_box_new(orientation, BOX_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(self->box), 0);
    gtk_container_add(GTK_CONTAINER(self->alignment), self->box);
    gtk_widget_show(self->box);

    places_button_resize(self);

    g_signal_connect(G_OBJECT(plugin), "mode-changed",
                     G_CALLBACK(places_button_mode_changed), self);
    g_signal_connect(G_OBJECT(plugin), "size-changed",
                     G_CALLBACK(places_button_size_changed), self);

    icon_theme = gtk_icon_theme_get_default ();
    g_signal_connect_swapped(icon_theme, "changed",
                             G_CALLBACK(places_button_theme_changed), self);
    self->screen_changed_id = g_signal_connect(G_OBJECT(self), "screen-changed",
                     G_CALLBACK(places_button_theme_changed), NULL);

}


GtkWidget*
places_button_new(XfcePanelPlugin *plugin)
{
    PlacesButton *button;

    g_assert(XFCE_IS_PANEL_PLUGIN(plugin));

    button = (PlacesButton*) g_object_new(PLACES_TYPE_BUTTON, NULL);
    places_button_construct(button, plugin);

    return GTK_WIDGET (button);
}

static void
places_button_dispose(GObject *object)
{
    PlacesButton *self = PLACES_BUTTON(object);

    if (self->screen_changed_id != 0) {
        g_signal_handler_disconnect(self, self->screen_changed_id);
        self->screen_changed_id = 0;
    }

    if (self->plugin != NULL) {
        g_object_unref(self->plugin);
        self->plugin = NULL;
    }

    (*G_OBJECT_CLASS(places_button_parent_class)->dispose) (object);
}

static void
places_button_destroy_image(PlacesButton *self)
{
    if (self->image != NULL) {
        gtk_widget_destroy(self->image);
        g_object_unref(self->image);
        self->image = NULL;
    }
}
static void
places_button_resize_image(PlacesButton *self, gint new_size)
{
    GdkPixbuf *icon;
    cairo_surface_t *surface;
    gint scale_factor;

    if (self->pixbuf_factory == NULL) {
        places_button_destroy_image(self);
        return;
    }

    scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self));
    icon = self->pixbuf_factory(new_size, scale_factor);
    
    if (G_UNLIKELY(icon == NULL)) {
        DBG("Could not load icon for button");
        places_button_destroy_image(self);
        return;
    }

    surface = gdk_cairo_surface_create_from_pixbuf(icon, scale_factor, NULL);
    if (self->image == NULL) {
        self->image = g_object_ref(gtk_image_new_from_surface(surface));
        gtk_box_pack_start(GTK_BOX(self->box), self->image, FALSE, FALSE, 0);
    } else {
        gtk_image_set_from_surface(GTK_IMAGE(self->image), surface);
    }

    gtk_widget_set_halign (GTK_WIDGET (self->image), GTK_ALIGN_CENTER);
    gtk_widget_set_valign (GTK_WIDGET (self->image), GTK_ALIGN_CENTER);
    gtk_widget_show(self->image);
    cairo_surface_destroy(surface);
    g_object_unref(G_OBJECT(icon));
}

static void
places_button_destroy_label(PlacesButton *self)
{
    if (self->label != NULL) {
        gtk_widget_destroy(self->label);
        g_object_unref(self->label);
        self->label = NULL;
    }
}

static void
places_button_resize_label(PlacesButton *self,
                           gboolean      show)
{
    gboolean vertical = FALSE;
    gboolean deskbar = FALSE;

    if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        deskbar = TRUE;
    else if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
        vertical = TRUE;

    if (self->label_text == NULL) {
        places_button_destroy_label(self);
        return;
    }

    if (self->label == NULL) {
        self->label = g_object_ref(gtk_label_new(self->label_text));
        gtk_box_pack_end(GTK_BOX(self->box), self->label, TRUE, TRUE, 0);
    }
    else
        gtk_label_set_text(GTK_LABEL(self->label), self->label_text);

    if (deskbar)
      gtk_label_set_ellipsize (GTK_LABEL (self->label), PANGO_ELLIPSIZE_END);
    else
      gtk_label_set_ellipsize (GTK_LABEL (self->label), PANGO_ELLIPSIZE_NONE);

    if (vertical)
      {
        gtk_label_set_angle (GTK_LABEL (self->label), -90);
        if (self->image != NULL) {
            gtk_widget_set_halign (GTK_WIDGET (self->image), GTK_ALIGN_CENTER);
            gtk_widget_set_valign (GTK_WIDGET (self->image), GTK_ALIGN_START);
        }
      }
    else
      {
        gtk_label_set_angle (GTK_LABEL (self->label), 0);
        if (self->image != NULL) {
            gtk_widget_set_halign (GTK_WIDGET (self->image), GTK_ALIGN_START);
            gtk_widget_set_valign (GTK_WIDGET (self->image), GTK_ALIGN_CENTER);
        }
      }
    gtk_widget_show(self->label);
}


static void
places_button_resize(PlacesButton *self)
{
    gboolean show_image, show_label;
    gint new_size, image_size;
#if LIBXFCE4PANEL_CHECK_VERSION(4, 13, 0)
#else
    GtkStyle *style;
    gint border_thickness;
#endif
    gboolean vertical = FALSE;
    gboolean deskbar = FALSE;
    gint nrows = 1;

    if (self->plugin == NULL)
        return;

    new_size = xfce_panel_plugin_get_size(self->plugin);
    DBG("Panel size: %d", new_size);

    show_image = self->pixbuf_factory != NULL;
    show_label = self->label_text != NULL;

    if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        deskbar = TRUE;
    else if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
        vertical = TRUE;
    nrows = xfce_panel_plugin_get_nrows(self->plugin);

    if (show_image && deskbar && nrows == 1)
      show_label = FALSE;

    new_size /= nrows;

    xfce_panel_plugin_set_small (self->plugin, !show_label);

    if (show_image)
        gtk_widget_set_size_request (GTK_WIDGET (self), new_size, new_size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (self), -1, -1);

    if (show_label) {
        if (vertical) {
          gtk_widget_set_halign (self->alignment, GTK_ALIGN_CENTER);
          gtk_widget_set_valign (self->alignment, GTK_ALIGN_START);
        } else {
          gtk_widget_set_halign (self->alignment, GTK_ALIGN_START);
          gtk_widget_set_valign (self->alignment, GTK_ALIGN_CENTER);
        }
    } else {
        gtk_widget_set_halign (self->alignment, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (self->alignment, GTK_ALIGN_CENTER);
    }

    /* image */
#if LIBXFCE4PANEL_CHECK_VERSION(4, 13, 0)
    image_size = xfce_panel_plugin_get_icon_size (self->plugin);
#else
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    style = gtk_widget_get_style(GTK_WIDGET(self));
    G_GNUC_END_IGNORE_DEPRECATIONS
    border_thickness = 2 * MAX(style->xthickness, style->ythickness) + 2;
    image_size = new_size - border_thickness;
#endif
    places_button_resize_image(self, image_size);

    /* label */
    places_button_resize_label(self, show_label);
}

static void
places_button_mode_changed(XfcePanelPlugin *plugin, XfcePanelPluginMode mode, PlacesButton *self)
{
    DBG("orientation changed");
    gtk_orientable_set_orientation (GTK_ORIENTABLE (self->box),
                                    (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
                                    GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
    places_button_resize(self);
}

static gboolean
places_button_size_changed(XfcePanelPlugin *plugin, gint size, PlacesButton *self)
{
    DBG("size changed");
    places_button_resize(self);
    return TRUE;
}

static void
places_button_theme_changed(PlacesButton *self)
{
    DBG("theme changed");
    places_button_resize(self);
}

/* vim: set ai et tabstop=4: */
