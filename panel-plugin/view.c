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

#include <string.h>

#include "view.h"
#include "places.h"
#include "model.h"
#include "model_system.h"
#include "model_volumes.h"
#include "model_user.h"
#include "cfg.h"

// UI Helpers
static void     places_view_update_menu(PlacesData*);

// GTK Callbacks

//  - Panel
static gboolean places_view_cb_size_changed(PlacesData*, gint size);
static void     places_view_cb_orientation_changed(PlacesData *pd, GtkOrientation orientation,
                                                   XfcePanelPlugin *panel);

static gboolean places_view_cb_theme_changed(GSignalInvocationHint*,
                             guint n_param_values, const GValue *param_values,
                             PlacesData*);

//  - Menu
static void     places_view_cb_menu_position(GtkMenu*, 
                                             gint *x, gint *y, 
                                             gboolean *push_in, 
                                             PlacesData*);
static void     places_view_cb_menu_deact(PlacesData*, GtkWidget *menu);

//  - Button
static gboolean places_view_cb_button_pressed(PlacesData*, GdkEventButton *);

//  - Recent Documents
#if USE_RECENT_DOCUMENTS
static void     places_view_cb_recent_item_open(GtkRecentChooser*, PlacesData*);
static gboolean places_view_cb_recent_items_clear(GtkWidget *clear_item);
#endif

// Model Visitor Callbacks
static void     places_view_add_menu_item(PlacesData*, PlacesBookmark*);

/********** Initialization & Finalization **********/
void
places_view_init(PlacesData *pd)
{
    DBG("initializing");
    
    gpointer icon_theme_class;

    pd->view_needs_separator = FALSE;
    pd->view_menu = NULL;

    pd->cfg = places_cfg_new(pd);
    places_view_reconfigure_model(pd);
    
    pd->view_tooltips = g_object_ref_sink(gtk_tooltips_new());

    // init button

    // create the box
    if(xfce_panel_plugin_get_orientation(pd->plugin) == GTK_ORIENTATION_HORIZONTAL)
        pd->view_button_box = gtk_hbox_new(FALSE, BORDER);
    else
        pd->view_button_box = gtk_vbox_new(FALSE, BORDER);
    gtk_container_set_border_width(GTK_CONTAINER(pd->view_button_box), 0);
    gtk_widget_show(pd->view_button_box);

    // create the button
    pd->view_button = xfce_create_panel_toggle_button();
    gtk_widget_show(pd->view_button);
    gtk_container_add(GTK_CONTAINER(pd->view_button), pd->view_button_box);
    gtk_container_add(GTK_CONTAINER(pd->plugin), pd->view_button);
    gtk_tooltips_set_tip(pd->view_tooltips, pd->view_button, pd->cfg->label, NULL);
    xfce_panel_plugin_add_action_widget(pd->plugin, pd->view_button);
    
    // create the image
    if(pd->cfg->show_button_icon){
        pd->view_button_image = g_object_ref(gtk_image_new());
        gtk_widget_show(pd->view_button_image);
        gtk_box_pack_start(GTK_BOX(pd->view_button_box), pd->view_button_image, TRUE, TRUE, 0);
    }else{
        pd->view_button_image = NULL;
    }

    // create the label
    if(pd->cfg->show_button_label){
        pd->view_button_label = g_object_ref(gtk_label_new(pd->cfg->label));
        gtk_widget_show(pd->view_button_label);
        gtk_box_pack_end(GTK_BOX(pd->view_button_box), pd->view_button_label, TRUE, TRUE, 0);
    }else{
        pd->view_button_label = NULL;
    }


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
                             
    g_signal_connect_swapped(G_OBJECT(pd->plugin), "orientation-changed",
                             G_CALLBACK(places_view_cb_orientation_changed), pd);

    places_cfg_init_signals(pd);
}


void 
places_view_finalize(PlacesData *pd)
{
    places_view_destroy_menu(pd);
    g_signal_remove_emission_hook(g_signal_lookup("changed", GTK_TYPE_ICON_THEME),
                                  pd->view_theme_timeout_id);
    if(pd->view_button_image != NULL)
        g_object_unref(pd->view_button_image);
    if(pd->view_button_label != NULL)
        g_object_unref(pd->view_button_label);
    g_object_unref(pd->view_tooltips);

    if(pd->bookmark_groups != NULL){
        GList *bookmark_group = pd->bookmark_groups;
        while(bookmark_group != NULL){
            if(bookmark_group->data != NULL)
               places_bookmark_group_finalize((PlacesBookmarkGroup*) bookmark_group->data);
            bookmark_group = bookmark_group->next;
        }
        g_list_free(pd->bookmark_groups);
        pd->bookmark_groups = NULL;
    }

    places_cfg_finalize(pd);
}

void
places_view_reconfigure_model(PlacesData *pd)
{
    /* destroy first */
    if(pd->bookmark_groups != NULL){
        GList *bookmark_group = pd->bookmark_groups;
        while(bookmark_group != NULL){
            if(bookmark_group->data != NULL)
                places_bookmark_group_finalize((PlacesBookmarkGroup*) bookmark_group->data);
            bookmark_group = bookmark_group->next;
        }
        g_list_free(pd->bookmark_groups);
        pd->bookmark_groups = NULL;
    }

    /* now create */
    pd->bookmark_groups = g_list_append(pd->bookmark_groups, places_bookmarks_system_create());

    if(pd->cfg->show_volumes)
        pd->bookmark_groups = g_list_append(pd->bookmark_groups, places_bookmarks_volumes_create());

    if(pd->cfg->show_bookmarks){
        pd->bookmark_groups = g_list_append(pd->bookmark_groups, NULL); /* separator */
        pd->bookmark_groups = g_list_append(pd->bookmark_groups, places_bookmarks_user_create());
    }
}

/********** UI Helpers **********/

static void
places_view_update_menu(PlacesData *pd)
{
#if USE_RECENT_DOCUMENTS
    GtkWidget *recent_menu;
    GtkWidget *clear_item;
    GtkWidget *recent_item;
#endif

    /* destroy the old menu, if it exists */
    places_view_destroy_menu(pd);

    // Create a new menu
    pd->view_menu = gtk_menu_new();
    
    /* make sure the menu popups up in right screen */
    gtk_menu_set_screen (GTK_MENU (pd->view_menu),
                         gtk_widget_get_screen (GTK_WIDGET (pd->plugin)));

    GList *bookmark_group = pd->bookmark_groups;
    GList *bookmarks;
    PlacesBookmark *bookmark;
    while(bookmark_group != NULL){
        
        if(bookmark_group->data == NULL){ /* separator */

            pd->view_needs_separator = TRUE;

        }else{

            bookmarks = places_bookmark_group_get_bookmarks((PlacesBookmarkGroup*) bookmark_group->data);
    
            while(bookmarks != NULL){
                bookmark = (PlacesBookmark*) bookmarks->data;
                places_view_add_menu_item(pd, bookmark);
                bookmarks = bookmarks->next;
            }

            g_list_free(bookmarks);

        }

        bookmark_group = bookmark_group->next;
    }

    // Recent Documents
#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent || (pd->cfg->search_cmd != NULL && strlen(pd->cfg->search_cmd))){
#else
    if(pd->cfg->search_cmd != NULL && strlen(pd->cfg->search_cmd)){
#endif
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu),
                              gtk_separator_menu_item_new());
    }

    if(pd->cfg->search_cmd != NULL && strlen(pd->cfg->search_cmd)){
        GtkWidget *search_item = gtk_image_menu_item_new_with_mnemonic(_("Search for Files"));
        if(pd->cfg->show_icons){
            GtkWidget *search_image = gtk_image_new_from_icon_name("system-search", GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(search_item), search_image);
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), search_item);
        g_signal_connect_swapped(search_item, "activate",
                                 G_CALLBACK(places_gui_exec), pd->cfg->search_cmd);

    }

#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent){

        recent_menu = gtk_recent_chooser_menu_new();
        gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_icons);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_recent_number);
        g_signal_connect(recent_menu, "item-activated", 
                         G_CALLBACK(places_view_cb_recent_item_open), pd);
    
        if(pd->cfg->show_recent_clear){
    
            gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu),
                                  gtk_separator_menu_item_new());
        
            if(pd->cfg->show_icons){
                clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
            }else{
                GtkStockItem clear_stock_item;
                gtk_stock_lookup(GTK_STOCK_CLEAR, &clear_stock_item);
                clear_item = gtk_menu_item_new_with_mnemonic(clear_stock_item.label);
            }
            gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), clear_item);
            /* try button-release-event to catch mouse clicks and not hide the menu after */
            g_signal_connect(clear_item, "button-release-event",
                             G_CALLBACK(places_view_cb_recent_items_clear), NULL);
            /* use activate when button-release-event doesn't catch it (e.g., enter key pressed) */
            g_signal_connect(clear_item, "activate",
                             G_CALLBACK(places_view_cb_recent_items_clear), NULL);
    
        }
    
        recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
        if(pd->cfg->show_icons){
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                          gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), recent_item);
    }
#endif

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->view_menu, "deactivate",
                             G_CALLBACK(places_view_cb_menu_deact), pd);

    // Quit hiding the menu
    gtk_widget_show_all(pd->view_menu);

    // This helps allocate resources beforehand so it'll pop up faster the first time
    gtk_widget_realize(pd->view_menu);
}

static gboolean
places_view_bookmarks_changed(GList *bookmark_groups)
{
    gboolean ret = FALSE;
    GList *bookmark_group = bookmark_groups;

    while(bookmark_group != NULL){

        if(bookmark_group->data != NULL)
            ret = places_bookmark_group_changed((PlacesBookmarkGroup*) bookmark_group->data) || ret;

        bookmark_group = bookmark_group->next;
    }

    return ret;
}

void
places_view_open_menu(PlacesData *pd)
{
    /* check if menu is needed, or it needs an update */
    if(pd->view_menu == NULL || places_view_bookmarks_changed(pd->bookmark_groups))
        places_view_update_menu(pd);

    /* toggle the button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pd->view_button), TRUE);

    // Register this menu (for focus, transparency, auto-hide, etc)
    xfce_panel_plugin_register_menu(pd->plugin, GTK_MENU(pd->view_menu));

    /* popup menu */
    gtk_menu_popup (GTK_MENU (pd->view_menu), NULL, NULL,
                    (GtkMenuPositionFunc) places_view_cb_menu_position,
                    pd, 0,
                    gtk_get_current_event_time ());
}

void
places_view_destroy_menu(PlacesData *pd)
{
    if(pd->view_menu != NULL){
        g_signal_emit_by_name(G_OBJECT(pd->view_menu), "deactivate");
        gtk_widget_destroy(pd->view_menu);
        pd->view_menu = NULL;
    }
    pd->view_needs_separator = FALSE;
}

void
places_view_button_update(PlacesData *pd)
{
    GdkPixbuf *icon;
    gint wsize, size, width, height;
    gint pix_w = 0, pix_h = 0;
    GtkOrientation orientation = xfce_panel_plugin_get_orientation(pd->plugin);
    
    wsize = xfce_panel_plugin_get_size(pd->plugin);
    size = wsize - 2 - 2 * MAX(pd->view_button->style->xthickness,
                         pd->view_button->style->ythickness);

    if(pd->view_button_image != NULL){
        icon = xfce_themed_icon_load_category(2, size);
        if(G_LIKELY(icon != NULL)){
            
            pix_w = gdk_pixbuf_get_width(icon);
            pix_h = gdk_pixbuf_get_height(icon);
            
            gtk_image_set_from_pixbuf(GTK_IMAGE(pd->view_button_image), icon);
            g_object_unref(G_OBJECT(icon));
        }
    }

    width = pix_w + (wsize - size);
    height = pix_h + (wsize - size);

    if(pd->view_button_label != NULL){
        GtkRequisition req;
        gtk_widget_size_request(pd->view_button_label, &req);
        if(orientation == GTK_ORIENTATION_HORIZONTAL)
            width += req.width + BORDER;
        else {
            if(width <= req.width)
                width = req.width + GTK_WIDGET(pd->view_button_label)->style->xthickness;
            height += req.height + BORDER;
        }
    }

    if(pd->view_button_image != NULL && pd->view_button_label != NULL){
        gint delta = gtk_box_get_spacing(GTK_BOX(pd->view_button_box));
        if(orientation == GTK_ORIENTATION_HORIZONTAL)
            width += delta;
        else
            height += delta;
    }

    gtk_widget_set_size_request(pd->view_button, width, height);
}

/********** Gtk Callbacks **********/

// Panel callbacks

static gboolean
places_view_cb_size_changed(PlacesData *pd, gint wsize)
{
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

static void
places_view_cb_orientation_changed(PlacesData *pd, GtkOrientation orientation, XfcePanelPlugin *panel){

    gtk_widget_set_size_request(pd->view_button, -1, -1);

    gtk_container_remove(GTK_CONTAINER(pd->view_button),
                         gtk_bin_get_child(GTK_BIN(pd->view_button)));

    if(orientation == GTK_ORIENTATION_HORIZONTAL)
       pd->view_button_box = gtk_hbox_new(FALSE, BORDER);
    else
       pd->view_button_box = gtk_vbox_new(FALSE, BORDER);
    
    gtk_container_set_border_width(GTK_CONTAINER(pd->view_button_box), 0);
    gtk_widget_show(pd->view_button_box);
    gtk_container_add(GTK_CONTAINER(pd->view_button), pd->view_button_box);

    if(pd->view_button_image != NULL){
        gtk_widget_show(pd->view_button_image);
        gtk_box_pack_start(GTK_BOX(pd->view_button_box), pd->view_button_image, TRUE, TRUE, 0);
    }

    if(pd->view_button_label != NULL){
        gtk_widget_show(pd->view_button_label);
        gtk_box_pack_end(GTK_BOX(pd->view_button_box), pd->view_button_label, TRUE, TRUE, 0);
    }

    places_view_button_update(pd);
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

// Button
static gboolean
places_view_cb_button_pressed(PlacesData *pd, GdkEventButton *evt)
{
    // (it's the way xfdesktop menu does it...)
    if((evt->state & GDK_CONTROL_MASK) && !(evt->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK)))
        return FALSE;

    if(evt->button == 1)
        places_view_open_menu(pd);
    else if(evt->button == 2)
        places_load_thunar(NULL);

    return FALSE;
}

void
places_view_cb_menu_item_context_act(GtkWidget *item, PlacesData *pd)
{
    g_assert(pd != NULL);
    g_assert(pd->view_menu != NULL && GTK_IS_WIDGET(pd->view_menu));

    /* we want the menu gone - now - since it prevents mouse grabs */
    gtk_widget_hide(pd->view_menu);
    while(g_main_context_iteration(NULL, FALSE))
        /* no op */;

    PlacesBookmarkAction *action = (PlacesBookmarkAction*) g_object_get_data(G_OBJECT(item), "action");
    DBG("Calling action %s", action->label);
    places_bookmark_action_call(action);
    
}

gboolean
places_view_cb_menu_item_do_alt(PlacesData *pd, GtkWidget *menu_item)
{
    
    GList *actions = (GList*) g_object_get_data(G_OBJECT(menu_item), "actions");
    GtkWidget *context, *context_item;
    PlacesBookmarkAction *action;


    if(actions != NULL){

        context = gtk_menu_new();
        gtk_widget_show(context);

        while(actions != NULL){
            action = (PlacesBookmarkAction*) actions->data;

            context_item = gtk_menu_item_new_with_label(action->label);
            g_object_set_data(G_OBJECT(context_item), "action", action);
            gtk_widget_show(context_item);
            g_signal_connect(context_item, "activate",
                             G_CALLBACK(places_view_cb_menu_item_context_act), pd);
            gtk_menu_shell_append(GTK_MENU_SHELL(context), context_item);

            actions = actions->next;
        }

        gtk_menu_popup(GTK_MENU(context),
                       NULL, NULL,
                       NULL, NULL,
                       0, gtk_get_current_event_time());

        g_signal_connect_swapped(context, "deactivate", G_CALLBACK(places_view_open_menu), pd);

    }

    return TRUE;
}

gboolean
places_view_cb_menu_item_do_main(PlacesData *pd, GtkWidget *menu_item)
{
        const gchar *uri = (const gchar*) g_object_get_data(G_OBJECT(menu_item), "uri");
        if(uri != NULL){
            places_load_thunar(uri);
            return FALSE;
        }
        return TRUE;
}


gboolean
places_view_cb_menu_item_press(GtkWidget *menu_item, GdkEventButton *event, PlacesData *pd)
{

    gboolean ctrl =  (event->state & GDK_CONTROL_MASK) && 
                    !(event->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK));
    
    if(event->button == 3 || (event->button == 1 && ctrl))
        return places_view_cb_menu_item_do_alt(pd, menu_item);
    else if(event->button == 1 && !ctrl)
        /* If it's insensitive, don't close the menu */
        return !GTK_WIDGET_IS_SENSITIVE(gtk_bin_get_child(GTK_BIN(menu_item)));
    else
        return FALSE;
}

// Recent Documents

#if USE_RECENT_DOCUMENTS
static void
places_view_cb_recent_item_open(GtkRecentChooser *chooser, PlacesData *pd)
{
    gchar *uri = gtk_recent_chooser_get_current_uri(chooser);
    places_load_file(uri);
    g_free(uri);
}

static gboolean
places_view_cb_recent_items_clear(GtkWidget *clear_item)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    gint removed = gtk_recent_manager_purge_items(manager, NULL);
    DBG("Cleared %d recent items", removed);
    return TRUE;
}
#endif

/********** Model Visitor Callbacks **********/


static void
places_view_load_terminal_wrapper(PlacesBookmarkAction *act)
{
    g_assert(act != NULL);
    places_load_terminal((gchar*) act->priv);
}


static void
places_view_add_menu_item(PlacesData *pd, PlacesBookmark *bookmark)
{
    g_assert(pd != NULL);
    g_assert(bookmark != NULL);

    GtkWidget *item;

    if(pd->view_needs_separator){
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu),
                              gtk_separator_menu_item_new());
        pd->view_needs_separator = FALSE;
    }

    item = gtk_image_menu_item_new_with_label(bookmark->label);

    if(pd->cfg->show_icons && bookmark->icon != NULL){
        GdkPixbuf *pb = xfce_themed_icon_load(bookmark->icon, 16);
        
        if(G_LIKELY(pb != NULL)){
            GtkWidget *image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
        }
    }

    if(bookmark->uri != NULL){
        
        g_object_set_data(G_OBJECT(item), "uri", (gchar*) bookmark->uri);

        if(bookmark->uri_scheme != PLACES_URI_SCHEME_TRASH){
            PlacesBookmarkAction *terminal  = g_new0(PlacesBookmarkAction, 1);
            terminal->label                 = "Open Terminal Here";
            terminal->priv                  = (gchar*) bookmark->uri;
            terminal->action                = places_view_load_terminal_wrapper;
            bookmark->actions = g_list_append(bookmark->actions, terminal);
        }

    }else{
        /* Probably an unmounted volume. Gray it out. */
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(item)), FALSE);
    }

    if(bookmark->actions != NULL)
        g_object_set_data(G_OBJECT(item), "actions", bookmark->actions);


    g_signal_connect(item, "button-release-event",
                     G_CALLBACK(places_view_cb_menu_item_press), pd);
    g_signal_connect_swapped(item, "activate",
                     G_CALLBACK(places_view_cb_menu_item_do_main), pd);
    g_signal_connect_swapped(item, "destroy",
                     G_CALLBACK(places_bookmark_free), bookmark);

    gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), item);

}

// vim: ai et tabstop=4
