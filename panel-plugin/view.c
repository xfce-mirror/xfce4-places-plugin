/*  xfce4-places-plugin
 *
 *  Copyright (c) 2007 Diego Ongaro <ongardie@gmail.com>
 *
 *  Largely based on:
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

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfcegui4/libxfcegui4.h>

#include "view.h"
#include "places.h"
#include "model.h"

#if GTK_CHECK_VERSION(2,10,0)
#  define USE_RECENT_DOCUMENTS TRUE
#endif

// UI Helpers
static void     places_view_update_menu(PlacesData*);
static void     places_view_open_menu(PlacesData*);
static void     places_view_destroy_menu(PlacesData*);
static void     places_view_button_update(PlacesData*);

// GTK Callbacks

//  - Panel
static gboolean places_view_cb_size_changed(PlacesData*, guint size);
static gboolean places_view_cb_theme_changed(GSignalInvocationHint*,
                             guint n_param_values, const GValue *param_values,
                             PlacesData*);

//  - Menu
static void     places_view_cb_menu_position(GtkMenu*, 
                                             gint *x, gint *y, 
                                             gboolean *push_in, 
                                             PlacesData*);
static void     places_view_cb_menu_deact(PlacesData*, GtkWidget *menu);
static void     places_view_cb_menu_item_open(GtkWidget *item, const gchar *uri);

//  - Button
static gboolean places_view_cb_button_pressed(PlacesData*, GdkEventButton *);

//  - Recent Documents
#if USE_RECENT_DOCUMENTS
static void     places_view_cb_recent_item_open(GtkRecentChooser*, PlacesData*);
static void     places_view_cb_recent_items_clear(GtkWidget *clear_item);
#endif

// Model Visitor Callbacks
static void     places_view_add_menu_item(gpointer _places_data, 
                                   const gchar *label, const gchar *uri, const gchar *icon);
static void     places_view_add_menu_sep(gpointer _places_data);


/********** Initialization & Finalization **********/

void
places_view_init(PlacesData *pd)
{
    DBG("initializing");
    
    gpointer icon_theme_class;

    pd->view_menu = NULL;

    pd->view_tooltips = g_object_ref_sink(gtk_tooltips_new());

    // init button
    pd->view_button = xfce_create_panel_toggle_button();    
    gtk_widget_show (pd->view_button);
    gtk_container_add(GTK_CONTAINER(pd->plugin), pd->view_button);
    gtk_button_set_focus_on_click(GTK_BUTTON(pd->view_button), FALSE);
    gtk_tooltips_set_tip(pd->view_tooltips, pd->view_button, _("Places"), NULL);
    
    pd->view_button_image = gtk_image_new();
    // TODO: why does xfdesktop ref the new image?
    gtk_widget_show(pd->view_button_image);
    gtk_container_add(GTK_CONTAINER(pd->view_button), pd->view_button_image);

    xfce_panel_plugin_add_action_widget(pd->plugin, pd->view_button);


    // signal for icon theme changes
    icon_theme_class = g_type_class_ref(GTK_TYPE_ICON_THEME);
    pd->view_theme_timeout_id = g_signal_add_emission_hook(g_signal_lookup("changed", GTK_TYPE_ICON_THEME),
                                                            0, (GSignalEmissionHook) places_view_cb_theme_changed,
                                                            pd, NULL);
    g_type_class_unref(icon_theme_class);
   
    
    // connect the signals
    g_signal_connect_swapped(pd->view_button, "button-press-event",
                             G_CALLBACK(places_view_cb_button_pressed), pd);

    g_signal_connect_swapped(G_OBJECT(pd->plugin), "size-changed",
                             G_CALLBACK(places_view_cb_size_changed), pd);
}


void 
places_view_finalize(PlacesData *pd)
{
    places_view_destroy_menu(pd);
    g_signal_remove_emission_hook(g_signal_lookup("changed", GTK_TYPE_ICON_THEME),
                                  pd->view_theme_timeout_id);
    g_object_unref(pd->view_tooltips);
}

/********** UI Helpers **********/

static void
places_view_update_menu(PlacesData *pd)
{
    BookmarksVisitor visitor;
#if USE_RECENT_DOCUMENTS
    GtkWidget *recent_menu;
    GtkWidget *clear_item;
    GtkWidget *recent_item;
#endif

    /* destroy the old menu, if it exists */
    places_view_destroy_menu(pd);

    // Create a new menu
    pd->view_menu = gtk_menu_new();
    
    // Register this menu (for auto-hide)
    xfce_panel_plugin_register_menu(pd->plugin, GTK_MENU(pd->view_menu)); // TODO why does xfdesktop
                                                                          // do this on every menu opening?

    /* make sure the menu popups up in right screen */
    gtk_menu_set_screen (GTK_MENU (pd->view_menu),
                         gtk_widget_get_screen (GTK_WIDGET (pd->plugin)));


    // Add system, volumes, user bookmarks
    visitor.pass_thru  = pd;
    visitor.item       = places_view_add_menu_item;
    visitor.separator  = places_view_add_menu_sep;
    places_bookmarks_visit(pd->bookmarks, &visitor);

    // Recent Documents
#if USE_RECENT_DOCUMENTS
    places_view_add_menu_sep(pd);

    recent_menu = gtk_recent_chooser_menu_new();
    g_signal_connect(recent_menu, "item-activated", 
                     G_CALLBACK(places_view_cb_recent_item_open), pd);

    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu),
                          gtk_separator_menu_item_new());
    clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), clear_item);
    g_signal_connect(clear_item, "activate",
                     G_CALLBACK(places_view_cb_recent_items_clear), NULL);
    
    recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                  gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), recent_item);
#endif

    // Quit hiding the menu
    gtk_widget_show_all(pd->view_menu);

    // This helps allocate resources beforehand so it'll pop up faster the first time
    gtk_widget_realize(pd->view_menu);
}


static void
places_view_open_menu(PlacesData *pd)
{
    /* check if menu is needed, or it needs an update */
    if(pd->view_menu == NULL || places_bookmarks_changed(pd->bookmarks))
        places_view_update_menu(pd);

    /* toggle the button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->view_button), TRUE);

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->view_menu, "deactivate",
                             G_CALLBACK(places_view_cb_menu_deact), pd);
    // TODO: in xfdesktop, why do sig_id stuff? why guint, then int? gulong is what it uses

    /* popup menu */
    gtk_menu_popup (GTK_MENU (pd->view_menu), NULL, NULL,
                    (GtkMenuPositionFunc) places_view_cb_menu_position,
                    pd, 0,
                    gtk_get_current_event_time ());
}

static void
places_view_destroy_menu(PlacesData *pd)
{
    if(pd->view_menu != NULL){
        g_signal_emit_by_name(G_OBJECT(pd->view_menu), "deactivate");
        gtk_widget_destroy(pd->view_menu);
        pd->view_menu = NULL;
    }
}

static void
places_view_button_update(PlacesData *pd)
{
    GdkPixbuf *icon;
    guint size;
    
    size = xfce_panel_plugin_get_size(XFCE_PANEL_PLUGIN(pd->plugin))
           - 2 - 2 * MAX(pd->view_button->style->xthickness,
                         pd->view_button->style->ythickness);

    icon = xfce_themed_icon_load_category(2, size);
    if(G_LIKELY(icon != NULL)){

        gtk_image_set_from_pixbuf(GTK_IMAGE(pd->view_button_image), icon);
        g_object_unref(G_OBJECT(icon));

    }else{
        // I'd use this, but it's new in gtk 2.8:
        // gtk_image_clear(GTK_IMAGE(pd->view_button_image));

        gtk_widget_destroy(pd->view_button_image);
        pd->view_button_image = gtk_image_new();
        gtk_widget_show(pd->view_button_image);
        gtk_container_add(GTK_CONTAINER(pd->view_button), pd->view_button_image);

    }
}

/********** Gtk Callbacks **********/

// Panel callbacks

static gboolean
places_view_cb_size_changed(PlacesData *pd, guint size)
{
    gtk_widget_set_size_request(pd->view_button, size, size);

    if(GTK_WIDGET_REALIZED(pd->view_button))
        places_view_button_update(pd);

    return TRUE;
}

static gboolean
places_view_cb_theme_changed(GSignalInvocationHint *ihint,
                             guint n_param_values, const GValue *param_values,
                             PlacesData *pd)
{
    // update the button
    if(GTK_WIDGET_REALIZED(pd->view_button))
        places_view_button_update(pd);
    
    // force a menu update
    places_view_destroy_menu(pd);

    return TRUE;
}

// Menu callbacks

/* Copied almost verbatim from xfdesktop plugin */
static void
places_view_cb_menu_position(GtkMenu *menu, 
                             gint *x, gint *y, 
                             gboolean *push_in, 
                             PlacesData *pd)
{
    XfceScreenPosition pos;
    GtkRequisition req;

    gtk_widget_size_request(GTK_WIDGET(menu), &req);

    gdk_window_get_origin (GTK_WIDGET (pd->plugin)->window, x, y);

    pos = xfce_panel_plugin_get_screen_position(pd->plugin);

    switch(pos) {
        case XFCE_SCREEN_POSITION_NW_V:
        case XFCE_SCREEN_POSITION_W:
        case XFCE_SCREEN_POSITION_SW_V:
            *x += pd->view_button->allocation.width;
            *y += pd->view_button->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NE_V:
        case XFCE_SCREEN_POSITION_E:
        case XFCE_SCREEN_POSITION_SE_V:
            *x -= req.width;
            *y += pd->view_button->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NW_H:
        case XFCE_SCREEN_POSITION_N:
        case XFCE_SCREEN_POSITION_NE_H:
            *y += pd->view_button->allocation.height;
            break;
        
        case XFCE_SCREEN_POSITION_SW_H:
        case XFCE_SCREEN_POSITION_S:
        case XFCE_SCREEN_POSITION_SE_H:
            *y -= req.height;
            break;
        
        default:  /* floating */
        {
            GdkScreen *screen = NULL;
            gint screen_width, screen_height;

            gdk_display_get_pointer(gtk_widget_get_display(GTK_WIDGET(pd->plugin)),
                                                           &screen, x, y, NULL);
            screen_width = gdk_screen_get_width(screen);
            screen_height = gdk_screen_get_height(screen);
            if ((*x + req.width) > screen_width)
                *x -= req.width;
            if ((*y + req.height) > screen_height)
                *y -= req.height;
        }
    }

    if (*x < 0)
        *x = 0;

    if (*y < 0)
        *y = 0;

    /* "wtf is this ?" -Xfdesktop source */
    *push_in = FALSE;
}

static void 
places_view_cb_menu_deact(PlacesData *pd, GtkWidget *menu)
{
    // deactivate button
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->view_button), FALSE);
}

static void
places_view_cb_menu_item_open(GtkWidget *widget, const gchar* uri)
{
    places_load_thunar(uri);
}

// Button
static gboolean
places_view_cb_button_pressed(PlacesData *pd, GdkEventButton *ev)
{
    if(G_UNLIKELY(ev->button != 1))
        return FALSE;

    // TODO: why does xfdesktop have:
    // if(evt->button != 1 || ((evt->state & GDK_CONTROL_MASK)
    //                         && !(evt->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK))))
    
    places_view_open_menu(pd);

    return FALSE;
}

// Recent Documents

#if USE_RECENT_DOCUMENTS
static void
places_view_cb_recent_item_open(GtkRecentChooser *chooser, PlacesData *pd)
{
    gchar *uri = gtk_recent_chooser_get_current_uri(chooser);
    places_load_thunar(uri);
    g_free(uri);
}

static void
places_view_cb_recent_items_clear(GtkWidget *clear_item)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    gint removed = gtk_recent_manager_purge_items(manager, NULL);
    DBG("Cleared %d recent items", removed);
}
#endif

/********** Model Visitor Callbacks **********/

static void
places_view_add_menu_item(gpointer _pd, const gchar *label, const gchar *uri, const gchar *icon)
{
    g_assert(_pd != NULL);
    g_return_if_fail(label != NULL && label != "");
    g_return_if_fail(uri != NULL && uri != "");

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

    g_signal_connect(item, "activate",
                     G_CALLBACK(places_view_cb_menu_item_open), (gchar*) uri);
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), item);
}

static void
places_view_add_menu_sep(gpointer _pd)
{
    g_assert(_pd != NULL);
    PlacesData *pd = (PlacesData*) _pd;

    gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu),
                          gtk_separator_menu_item_new());
}

// vim: ai et tabstop=4
