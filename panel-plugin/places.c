/*  xfce4-places-plugin
 *
 *  Copyright (c) 2007 Diego Ongaro <ongardie@gmail.com>
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

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfcegui4/libxfcegui4.h>

#include "places.h"
#include "model.h"

#define PLUGIN_NAME "places"

/********** Initialization **********/
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(places_construct);

static void 
places_construct(XfcePanelPlugin *plugin)
{
    DBG ("Construct: %s", PLUGIN_NAME);
    
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8"); 

    DBG("GETTXT_PACKAGE: %s", GETTEXT_PACKAGE);
    DBG("PACKAGE_LOCALE_DIR: %s", PACKAGE_LOCALE_DIR);

    PlacesData *pd = panel_slice_new0(PlacesData);
    pd->plugin = plugin;

    pd->bookmarks = places_bookmarks_init();
    
    places_init_ui(pd);

    g_signal_connect (pd->panel_button, "button-release-event",
                      G_CALLBACK (places_cb_button_act), NULL);
   
    g_signal_connect (pd->panel_menu, "deactivate", 
                      G_CALLBACK(places_cb_menu_close), pd);
   
    g_signal_connect (pd->panel_arrow, "button-press-event",
                      G_CALLBACK (places_cb_menu_open), pd);

    g_signal_connect (pd->plugin, "free-data", 
                      G_CALLBACK (places_cb_free_data), pd);

    g_signal_connect (pd->plugin, "size-changed", 
                      G_CALLBACK (places_cb_size_changed), 
                      pd);
}

static void
places_init_ui(PlacesData *pd)
{
    DBG("initializing");

    XfceScreenPosition position = xfce_panel_plugin_get_screen_position(pd->plugin);
    
    // init pd->panel_size
    pd->panel_size = 0;

    // init pd->panel_box
    
    if(xfce_screen_position_is_horizontal(position))
        pd->panel_box = xfce_hvbox_new(GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
    else
        pd->panel_box = xfce_hvbox_new(GTK_ORIENTATION_VERTICAL, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(pd->plugin), pd->panel_box);
    xfce_panel_plugin_add_action_widget(pd->plugin, pd->panel_box);
    
    // init pd->panel_button
    pd->panel_button = xfce_create_panel_button();    
    gtk_widget_show (pd->panel_button);
    xfce_panel_plugin_add_action_widget(pd->plugin, pd->panel_button);
    gtk_container_add(GTK_CONTAINER(pd->panel_box), pd->panel_button);

    // init pd->panel_arrow
    if(xfce_screen_position_is_left(position))
        pd->panel_arrow = xfce_arrow_button_new(GTK_ARROW_RIGHT);
    else if(xfce_screen_position_is_right(position))
        pd->panel_arrow = xfce_arrow_button_new(GTK_ARROW_LEFT);
    else if(xfce_screen_position_is_bottom(position))
        pd->panel_arrow = xfce_arrow_button_new(GTK_ARROW_UP);
    else
        pd->panel_arrow = xfce_arrow_button_new(GTK_ARROW_DOWN);

    gtk_button_set_relief(GTK_BUTTON(pd->panel_arrow), GTK_RELIEF_NONE);
    gtk_widget_show(pd->panel_arrow);
    xfce_panel_plugin_add_action_widget(pd->plugin, pd->panel_arrow);
    gtk_container_add (GTK_CONTAINER(pd->panel_box), pd->panel_arrow);

    // show the box...
    gtk_widget_show(pd->panel_box);
    
    // init pd->panel_menu, pd->panel_menu_open
    places_init_panel_menu(pd);

}

static void
places_build_menu_item(gpointer _pd, const gchar *label, const gchar *uri, const gchar *icon)
{
    g_assert(_pd);
    g_return_if_fail(label && label != "");
    g_return_if_fail(uri && uri != "");

    PlacesData *pd = (PlacesData*) _pd;
    GtkWidget *item = gtk_image_menu_item_new_with_label(label);

    if(G_LIKELY(icon != NULL)){
        GdkPixbuf *pb = xfce_themed_icon_load(icon, 16);
        
        if(G_LIKELY(pb != NULL)){
            GtkWidget *image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
        }
    }

    g_signal_connect(item, "button-release-event",
                     G_CALLBACK(places_cb_menu_item_act), (gchar*) uri);
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu), item);
}

static void
places_build_menu_separator(gpointer _pd)
{
    g_assert(_pd);
    PlacesData *pd = (PlacesData*) _pd;
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu),
                          gtk_separator_menu_item_new());
}

static void
places_init_panel_menu(PlacesData *pd)
{
    DBG("initializing");
    g_assert(pd && pd->panel_menu == NULL);

    pd->panel_menu = gtk_menu_new();
    pd->panel_menu_open = FALSE;
    
    // System, Volumes, User Bookmarks

    BookmarksVisitor *visitor = g_new0(BookmarksVisitor, 1);
    visitor->pass_thru = pd;
    visitor->item = places_build_menu_item;
    visitor->separator = places_build_menu_separator;
    places_bookmarks_visit(pd->bookmarks, visitor);

    // Recent Documents

    places_build_menu_separator(pd);

    GtkWidget *recent_menu = gtk_recent_chooser_menu_new();
    g_signal_connect(recent_menu, "item-activated", 
                     G_CALLBACK(places_cb_recent_item_activated), pd);

    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu),
                          gtk_separator_menu_item_new());
    GtkWidget *clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), clear_item);
    g_signal_connect(clear_item, "button-release-event",
                     G_CALLBACK(places_cb_recent_clear), NULL);
    
    GtkWidget *recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                  gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu), recent_item);


    gtk_widget_show_all(pd->panel_menu);

    // This helps allocate resources beforehand so it'll pop up faster the first time
    gtk_widget_realize(pd->panel_menu);
    
    gtk_menu_attach_to_widget(GTK_MENU(pd->panel_menu), pd->panel_arrow, NULL);

}

/********** Library **********/

static void
places_load_thunar(const gchar *path)
{
    gchar *cmd = g_strconcat("thunar ", path, NULL);
    xfce_exec(cmd, FALSE, TRUE, NULL);
    g_free(cmd);
}

/********** UI Helpers **********/

static void
places_close_menu(PlacesData *pd)
{
    DBG("closing menu");
    if(pd->panel_menu_open == FALSE)
        DBG("but the menu isn't open");

    gtk_widget_hide(pd->panel_menu);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->panel_arrow), FALSE);
    pd->panel_menu_open = FALSE;  
}

static void
places_ui_redraw(PlacesData *pd)
{
    DBG ("Drawing at size %d", pd->panel_size);

    GtkOrientation orientation = xfce_panel_plugin_get_orientation(pd->plugin);
    XfceScreenPosition position = xfce_panel_plugin_get_screen_position(pd->plugin);
    XfceArrowButton *arrow = XFCE_ARROW_BUTTON(pd->panel_arrow);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (pd->plugin), -1, pd->panel_size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (pd->plugin), pd->panel_size, -1);
    
    xfce_hvbox_set_orientation( XFCE_HVBOX(pd->panel_box), orientation);

    if (xfce_screen_position_is_left (position))
        xfce_arrow_button_set_arrow_type(arrow, GTK_ARROW_RIGHT);
    else if (xfce_screen_position_is_right (position))
        xfce_arrow_button_set_arrow_type(arrow, GTK_ARROW_LEFT);
    else if (xfce_screen_position_is_bottom (position))
        xfce_arrow_button_set_arrow_type(arrow, GTK_ARROW_UP);
    else
        xfce_arrow_button_set_arrow_type(arrow, GTK_ARROW_DOWN);

    GdkPixbuf *pb;
    if(pd->panel_size > 16)
        pb = xfce_themed_icon_load_category(2, pd->panel_size - 8);
    else
        pb = xfce_themed_icon_load_category(2, 8);
        
    GtkWidget *image = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_button_set_image(GTK_BUTTON(pd->panel_button), image);

}

/********** Gtk Callbacks **********/

static void
places_cb_recent_item_activated(GtkRecentChooser *chooser, PlacesData *pd)
{
    gchar *uri = gtk_recent_chooser_get_current_uri(chooser);
    places_load_thunar(uri);
    g_free(uri);
}

static void
places_cb_recent_clear(GtkWidget *widget, GdkEventButton *event, gpointer nu)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    gint removed = gtk_recent_manager_purge_items(manager, NULL);
    DBG("Cleared %d recent items", removed);
}

static gboolean
places_cb_size_changed(XfcePanelPlugin *plugin, int size, PlacesData *pd)
{
    pd->panel_size = size;
    places_ui_redraw(pd);
    return TRUE;
}

/* Copied almost verbatim from launcher plugin */
static void
places_cb_menu_position (GtkMenu *menu, int *x, int *y, gboolean *push_in, PlacesData *pd)
{
    g_assert(pd != NULL);

    GtkWidget *widget = pd->panel_box;
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    gdk_window_get_origin (widget->window, x, y);

 
    switch (xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (pd->panel_arrow)))
    {
        case GTK_ARROW_UP:
            *x += widget->allocation.x;
            *y += widget->allocation.y - req.height;
            break;
        case GTK_ARROW_DOWN:
            *x += widget->allocation.x;
            *y += widget->allocation.y + widget->allocation.height;
            break;
        case GTK_ARROW_LEFT:
            *x += widget->allocation.x - req.width;
            *y += widget->allocation.y - req.height
                + widget->allocation.height;
            break;
        case GTK_ARROW_RIGHT:
            *x += widget->allocation.x + widget->allocation.width;
            *y += widget->allocation.y - req.height
                + widget->allocation.height;
            break;
        default:
            break;
    }

    screen = gtk_widget_get_screen (widget);

    num = gdk_screen_get_monitor_at_window (screen, widget->window);

    gdk_screen_get_monitor_geometry (screen, num, &geom);

    if (*x > geom.x + geom.width - req.width)
        *x = geom.x + geom.width - req.width;
    if (*x < geom.x)
        *x = geom.x;

    if (*y > geom.y + geom.height - req.height)
        *y = geom.y + geom.height - req.height;
    if (*y < geom.y)
        *y = geom.y;
        
}

static void 
places_cb_menu_close(GtkMenuShell *menushell, PlacesData *pd)
{
    g_assert(pd != NULL);

    DBG("closing menu");
    places_close_menu(pd);
}

static gboolean
places_cb_menu_open(GtkButton *arrow, GdkEventButton *event, PlacesData *pd){
    g_assert(pd != NULL);

    if(event->button == 3)
        return FALSE;

    DBG("opening menu");

    if(pd->panel_menu_open){
        
        DBG("err...i mean closing...");
        places_close_menu(pd);

    }else{
        // This will make it behave like a mouse release when it's really a mouse press
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(arrow), TRUE);
    
        DBG("Has the model changed?");
        if(places_bookmarks_changed(pd->bookmarks)){
            DBG("Model changed");

            gtk_widget_destroy(pd->panel_menu);
            pd->panel_menu = NULL;

            places_init_panel_menu(pd);
            g_signal_connect (pd->panel_menu, "deactivate", 
                      G_CALLBACK(places_cb_menu_close), pd);

        }
        
        gtk_menu_popup(GTK_MENU(pd->panel_menu), NULL, NULL, 
                       (GtkMenuPositionFunc) places_cb_menu_position, pd,
                       0, gtk_get_current_event_time());
        pd->panel_menu_open = TRUE;
    }
    return TRUE;
}

static gboolean
places_cb_menu_item_act(GtkWidget *widget, GdkEventButton *event, const gchar* uri)
{
    DBG("load thunar for item");

    places_load_thunar(uri);
    
    return FALSE;
}

static gboolean
places_cb_button_act(GtkWidget *button, GdkEventButton *event, gpointer nu)
{
    if(event->button == 3) // Let the right-click menu come up
        return FALSE;
    
    DBG("load thunar at home directory");
    
    places_load_thunar(NULL);
    return FALSE;
}

static void 
places_cb_free_data(XfcePanelPlugin *plugin, PlacesData *pd)
{
    DBG ("Free data: %s", PLUGIN_NAME);
    g_assert(pd != NULL);
   
    places_bookmarks_finalize(pd->bookmarks);

    panel_slice_free(PlacesData, pd);
}

// vim: ai et tabstop=4
