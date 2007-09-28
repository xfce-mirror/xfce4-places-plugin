/*  xfce4-places-plugin
 *
 *  This file handles the GUI. It "owns" the model and cfg.
 *
 *  Copyright (c) 2007 Diego Ongaro <ongardie@gmail.com>
 *
 *  Largely based on:
 *
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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfcegui4/libxfcegui4.h>

#include <string.h>

#include "view.h"
#include "support.h"
#include "cfg.h"
#include "model.h"
#include "model_system.h"
#include "model_volumes.h"
#include "model_user.h"

struct _PlacesView
{
    /* plugin */
    XfcePanelPlugin           *plugin;
    
    /* configuration */
    PlacesCfg                 *cfg;
    PlacesCfgViewIface        *cfg_iface;
    PlacesViewCfgIface        *view_cfg_iface;
    
    /* view */
    GtkWidget                 *button;
    GtkWidget                 *button_image;
    GtkWidget                 *button_label;
    GtkWidget                 *menu;
    GtkTooltips               *tooltips;
    gboolean                  needs_separator;
    guint                     menu_timeout_id;
    
    GtkOrientation            orientation;
    gint                      size;
    gboolean                  show_button_icon;
    gboolean                  show_button_label;
    gchar                     *label_tooltip_text;
    gboolean                  force_update_theme;

    /* model */
    GList                     *bookmark_groups;
};

/* Debugging */
#if defined(DEBUG) && (DEBUG > 0)

static guint places_debug_menu_timeout_count = 0;
#define PLACES_DEBUG_MENU_TIMEOUT_COUNT(delta)                              \
    G_STMT_START{                                                           \
        places_debug_menu_timeout_count += delta;                           \
        DBG("Menu timeout count: %d", places_debug_menu_timeout_count);     \
        g_assert(places_debug_menu_timeout_count == 0 ||                    \
                 places_debug_menu_timeout_count == 1);                     \
        if(pd != NULL){                                                     \
            if(places_debug_menu_timeout_count == 0)                        \
                    g_assert(pd->menu_timeout_id == 0);                     \
            else                                                            \
                    g_assert(pd->menu_timeout_id > 0);                      \
        }                                                                   \
    }G_STMT_END

#else

#define PLACES_DEBUG_MENU_TIMEOUT_COUNT(delta)                              \
    G_STMT_START{                                                           \
        (void) 0;                                                           \
    }G_STMT_END

#endif

/* UI Helpers */
static void     pview_open_menu(PlacesView*);
static void     pview_update_menu(PlacesView*);
static void     pview_destroy_menu(PlacesView*);
static void     pview_button_update(PlacesView*);

/********** Interface for Cfg's Use **********/

inline void
places_view_cfg_iface_update_menu(PlacesViewCfgIface *iface)
{
    iface->update_menu(iface->places_view);
}

inline void
places_view_cfg_iface_update_button(PlacesViewCfgIface *iface)
{
    iface->update_button(iface->places_view);
}

inline void
places_view_cfg_iface_reconfigure_model(PlacesViewCfgIface *iface)
{
    iface->reconfigure_model(iface->places_view);
}

inline GtkWidget*
places_view_cfg_iface_make_empty_cfg_dialog(PlacesViewCfgIface *iface)
{
    return iface->make_empty_cfg_dialog(iface->places_view);
}

/********** Model Management **********/
static void
pview_destroy_model(PlacesView *view)
{
    GList *bookmark_group_li;
    PlacesBookmarkGroup *bookmark_group;

    /* we don't want the menu items holding on to any references */
    pview_destroy_menu(view);
   
    if(view->bookmark_groups != NULL){

        bookmark_group_li = view->bookmark_groups;
        do{
            if(bookmark_group_li->data != NULL){
                bookmark_group = (PlacesBookmarkGroup*) bookmark_group_li->data;
                places_bookmark_group_finalize(bookmark_group);
            }

            bookmark_group_li = bookmark_group_li->next;

        }while(bookmark_group_li != NULL);

        g_list_free(view->bookmark_groups);
        view->bookmark_groups = NULL;
    }

}

static void
pview_reconfigure_model(PlacesView *view)
{
    pview_destroy_model(view);

    /* now re-create it */
    view->bookmark_groups = g_list_append(view->bookmark_groups, places_bookmarks_system_create());

    if(view->cfg->show_volumes)
        view->bookmark_groups = g_list_append(view->bookmark_groups, places_bookmarks_volumes_create());

    if(view->cfg->show_bookmarks){
        view->bookmark_groups = g_list_append(view->bookmark_groups, NULL); /* separator */
        view->bookmark_groups = g_list_append(view->bookmark_groups, places_bookmarks_user_create());
    }
}

static gboolean
pview_model_changed(GList *bookmark_groups)
{
    gboolean ret = FALSE;
    GList *bookmark_group_li = bookmark_groups;
    PlacesBookmarkGroup *bookmark_group;

    while(bookmark_group_li != NULL){

        if(bookmark_group_li->data != NULL){
            bookmark_group = (PlacesBookmarkGroup*) bookmark_group_li->data;
            ret = places_bookmark_group_changed(bookmark_group) || ret;
        }

        bookmark_group_li = bookmark_group_li->next;
    }

    return ret;
}


/********** Gtk Callbacks **********/

/* Panel callbacks */

static gboolean
pview_cb_size_changed(PlacesView *pd)
{
    g_assert(pd != NULL);
    g_assert(pd->button != NULL);

    if(GTK_WIDGET_REALIZED(pd->button))
        pview_button_update(pd);

    return TRUE;
}

static void
pview_cb_theme_changed(PlacesView *view)
{
    g_assert(view != NULL);
    g_assert(view->button != NULL);

    DBG("theme changed");

    /* update the button */
    if(GTK_WIDGET_REALIZED(view->button)){
        view->force_update_theme = TRUE;
        pview_button_update(view);
    }
    
    /* force a menu update */
    pview_destroy_menu(view);
}

/* Menu callbacks */
static gboolean /* return false to stop calling it */
pview_cb_menu_timeout(PlacesView *pd){

    if(!pd->menu_timeout_id)
        goto killtimeout;

    if(pd->menu == NULL || !GTK_WIDGET_VISIBLE(pd->menu))
        goto killtimeout;

    if(pview_model_changed(pd->bookmark_groups))
        pview_open_menu(pd);

    PLACES_DEBUG_MENU_TIMEOUT_COUNT(0);
    return TRUE;   


  killtimeout:
    if(pd->menu_timeout_id){
        pd->menu_timeout_id = 0;
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(-1);
    }else{
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(0);
    }
    return FALSE;

}

/* Copied almost verbatim from notes plugin */
static void
pview_cb_menu_position(GtkMenu *menu,
                       gint *x, gint *y,
                       gboolean *push_in,
                       XfcePanelPlugin *plugin)
{
    GtkRequisition requisition;
    GtkWidget *attach_widget;

    g_return_if_fail(GTK_IS_MENU(menu));
    g_return_if_fail(XFCE_IS_PANEL_PLUGIN(plugin));

    attach_widget = gtk_menu_get_attach_widget(menu);
    g_return_if_fail(GTK_IS_WIDGET(attach_widget));

    gtk_widget_size_request(GTK_WIDGET(menu), &requisition);
    gdk_window_get_origin(attach_widget->window, x, y);

    switch(xfce_panel_plugin_get_orientation(plugin))
    {
        case GTK_ORIENTATION_HORIZONTAL:

            if(*y + attach_widget->allocation.height + requisition.height > gdk_screen_height()){
                /* Show menu above */
                *y -= requisition.height;
            }else{
                /* Show menu below */
                *y += attach_widget->allocation.height;
            }

            if(*x + requisition.width > gdk_screen_width()){
                /* Adjust horizontal position */
                *x = gdk_screen_width () - requisition.width;
            }

            break;

        case GTK_ORIENTATION_VERTICAL:

            if(*x + attach_widget->allocation.width + requisition.width > gdk_screen_width()){
                /* Show menu on the right */
                *x -= requisition.width;
            }else{
                /* Show menu on the left */
                *x += attach_widget->allocation.width;
            }

            if(*y + requisition.height > gdk_screen_height()){
                /* Adjust vertical position */
                *y = gdk_screen_height() - requisition.height;
            }

            break;

        default:
            break;
    }
    
    *push_in = FALSE;
}

static void 
pview_cb_menu_deact(PlacesView *pd, GtkWidget *menu)
{
    /* deactivate button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->button), FALSE);

    /* remove the timeout to save a tick */
    if(pd->menu_timeout_id){
        g_source_remove(pd->menu_timeout_id);
        pd->menu_timeout_id = 0;
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(-1);
    }else{
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(0);
    }
}

/* Button */
static gboolean
pview_cb_button_pressed(PlacesView *pd, GdkEventButton *evt)
{
    /* (it's the way xfdesktop menu does it...) */
    if((evt->state & GDK_CONTROL_MASK) && !(evt->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK)))
        return FALSE;

    if(evt->button == 1)
        pview_open_menu(pd);
    else if(evt->button == 2)
        places_load_thunar(NULL);

    return FALSE;
}

static void
pview_cb_menu_item_context_act(GtkWidget *item, PlacesView *pd)
{
    PlacesBookmarkAction *action;

    g_assert(pd != NULL);
    g_assert(pd->menu != NULL && GTK_IS_WIDGET(pd->menu));

    /* we want the menu gone - now - since it prevents mouse grabs */
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(pd->menu));
    while(g_main_context_iteration(NULL, FALSE))
        /* no op */;

    action = (PlacesBookmarkAction*) g_object_get_data(G_OBJECT(item), "action");
    DBG("Calling action %s", action->label);
    places_bookmark_action_call(action);
    
}

static gboolean
pview_cb_menu_item_do_alt(PlacesView *pd, GtkWidget *menu_item)
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
                             G_CALLBACK(pview_cb_menu_item_context_act), pd);
            gtk_menu_shell_append(GTK_MENU_SHELL(context), context_item);

            actions = actions->next;
        }

        gtk_menu_popup(GTK_MENU(context),
                       NULL, NULL,
                       NULL, NULL,
                       0, gtk_get_current_event_time());

        g_signal_connect_swapped(context, "deactivate",
                                 G_CALLBACK(pview_open_menu), pd);

    }

    return TRUE;
}

static gboolean
pview_cb_menu_item_press(GtkWidget *menu_item, GdkEventButton *event, PlacesView *pd)
{

    gboolean ctrl =  (event->state & GDK_CONTROL_MASK) && 
                    !(event->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK));
    gboolean sensitive = GTK_WIDGET_IS_SENSITIVE(gtk_bin_get_child(GTK_BIN(menu_item)));

    if(event->button == 3 || (event->button == 1 && (ctrl || !sensitive)))
        return pview_cb_menu_item_do_alt(pd, menu_item);
    else
        return FALSE;
}

/* Recent Documents */

#if USE_RECENT_DOCUMENTS
static void
pview_cb_recent_item_open(GtkRecentChooser *chooser, PlacesView *pd)
{
    gchar *uri = gtk_recent_chooser_get_current_uri(chooser);
    places_load_file(uri);
    g_free(uri);
}

static gboolean
pview_cb_recent_items_clear(GtkWidget *clear_item)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    gint removed = gtk_recent_manager_purge_items(manager, NULL);
    DBG("Cleared %d recent items", removed);
    return TRUE;
}
#endif


/********** UI Helpers **********/

static void
pview_destroy_menu(PlacesView *view)
{
    if(view->menu != NULL){
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(view->menu));
        gtk_widget_destroy(view->menu);
        view->menu = NULL;
    }
    view->needs_separator = FALSE;
}

static void
pview_add_menu_item(PlacesView *view, PlacesBookmark *bookmark)
{
    g_assert(view != NULL);
    g_assert(bookmark != NULL);

    GtkWidget *item;
    GdkPixbuf *pb;
    GtkWidget *image;

    if(view->needs_separator){
        gtk_menu_shell_append(GTK_MENU_SHELL(view->menu),
                              gtk_separator_menu_item_new());
        view->needs_separator = FALSE;
    }

    item = gtk_image_menu_item_new_with_label(bookmark->label);

    if(view->cfg->show_icons && bookmark->icon != NULL){
        pb = xfce_themed_icon_load(bookmark->icon, 16);
        
        if(G_LIKELY(pb != NULL)){
            image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
        }
    }

    if(bookmark->actions != NULL)
        g_object_set_data(G_OBJECT(item), "actions", bookmark->actions);

    /* do this always so that the menu doesn't close on right-clicks */
    g_signal_connect(item, "button-release-event",
                     G_CALLBACK(pview_cb_menu_item_press), view);

    if(bookmark->primary_action != NULL){

        g_signal_connect_swapped(item, "activate",
                                 G_CALLBACK(places_bookmark_action_call), 
                                 bookmark->primary_action);

    }else{

        /* Gray it out. */
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(item)),
                                 FALSE);

    }

    g_signal_connect_swapped(item, "destroy",
                     G_CALLBACK(places_bookmark_free), bookmark);

    gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), item);

}

static void
pview_update_menu(PlacesView *pd)
{
    GList *bookmark_group_li;
    PlacesBookmarkGroup *bookmark_group;
    GList *bookmarks;
    PlacesBookmark *bookmark;

#if USE_RECENT_DOCUMENTS
    GtkWidget *recent_menu;
    GtkWidget *clear_item;
    GtkWidget *recent_item;
#endif

    DBG("destroy menu");

    /* destroy the old menu, if it exists */
    pview_destroy_menu(pd);

    DBG("building new menu");

    /* Create a new menu */
    pd->menu = gtk_menu_new();
    
    /* make sure the menu popups up in right screen */
    gtk_menu_attach_to_widget(GTK_MENU(pd->menu), pd->button, NULL);
    gtk_menu_set_screen(GTK_MENU(pd->menu),
                        gtk_widget_get_screen(pd->button));

    /* add bookmarks */
    bookmark_group_li = pd->bookmark_groups;
    while(bookmark_group_li != NULL){
        
        if(bookmark_group_li->data == NULL){ /* separator */

            pd->needs_separator = TRUE;

        }else{

            bookmark_group = (PlacesBookmarkGroup*) bookmark_group_li->data;
            bookmarks = places_bookmark_group_get_bookmarks(bookmark_group);
    
            while(bookmarks != NULL){
                bookmark = (PlacesBookmark*) bookmarks->data;
                pview_add_menu_item(pd, bookmark);
                bookmarks = bookmarks->next;
            }

            g_list_free(bookmarks);

        }

        bookmark_group_li = bookmark_group_li->next;
    }

    /* Recent Documents */
#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent || (pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0')){
#else
    if(pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0'){
#endif
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu),
                              gtk_separator_menu_item_new());
    }

    if(pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0'){
        GtkWidget *search_item = gtk_image_menu_item_new_with_mnemonic(_("Search for Files"));
        if(pd->cfg->show_icons){
            GtkWidget *search_image = gtk_image_new_from_icon_name("system-search", GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(search_item), search_image);
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), search_item);
        g_signal_connect_swapped(search_item, "activate",
                                 G_CALLBACK(places_gui_exec), pd->cfg->search_cmd);

    }

#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent){

        recent_menu = gtk_recent_chooser_menu_new();
        gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_icons);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_recent_number);
        g_signal_connect(recent_menu, "item-activated", 
                         G_CALLBACK(pview_cb_recent_item_open), pd);
    
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
                             G_CALLBACK(pview_cb_recent_items_clear), NULL);
            /* use activate when button-release-event doesn't catch it (e.g., enter key pressed) */
            g_signal_connect(clear_item, "activate",
                             G_CALLBACK(pview_cb_recent_items_clear), NULL);
    
        }
    
        recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
        if(pd->cfg->show_icons){
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                          gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
        }
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), recent_item);
    }
#endif

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->menu, "deactivate",
                             G_CALLBACK(pview_cb_menu_deact), pd);

    /* Quit hiding the menu */
    gtk_widget_show_all(pd->menu);

    /* This helps allocate resources beforehand so it'll pop up faster the first time */
    gtk_widget_realize(pd->menu);
}

static void
pview_open_menu(PlacesView *pd)
{
    /* check if menu is needed, or it needs an update */
    if(pd->menu == NULL || pview_model_changed(pd->bookmark_groups))
        pview_update_menu(pd);

    /* toggle the button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->button), TRUE);

    /* Register this menu (for focus, transparency, auto-hide, etc) */
    /* We don't want to register if the menu is visible (hasn't been deactivated) */
    if(!GTK_WIDGET_VISIBLE(pd->menu))
        xfce_panel_plugin_register_menu(pd->plugin, GTK_MENU(pd->menu));

    /* popup menu */
    gtk_menu_popup (GTK_MENU (pd->menu), NULL, NULL,
                    (GtkMenuPositionFunc) pview_cb_menu_position,
                    pd->plugin, 0,
                    gtk_get_current_event_time ());
    
    /* menu timeout to poll for model changes */
    if(pd->menu_timeout_id == 0){
#if GLIB_CHECK_VERSION(2,14,0)
        pd->menu_timeout_id = g_timeout_add_seconds_full(G_PRIORITY_LOW, 2, 
                                   (GSourceFunc) pview_cb_menu_timeout, pd,
                                   NULL);
#else
        pd->menu_timeout_id = g_timeout_add_full(G_PRIORITY_LOW, 2000, 
                                   (GSourceFunc) pview_cb_menu_timeout, pd,
                                   NULL);
#endif
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(1);
    }else{
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(0);
    }
}

static void
pview_button_update(PlacesView *view)
{
    
    PlacesCfg *cfg = view->cfg;
    gboolean orientation_changed, size_changed, 
             icon_presence_changed, label_presence_changed, 
             label_tooltip_changed, theme_changed;
    static gboolean first_run = TRUE;

    DBG("button_update (first run: %1x)", first_run);

    if(first_run){
        first_run = FALSE;
        
        orientation_changed = TRUE;
        view->orientation = xfce_panel_plugin_get_orientation(view->plugin);
        
        size_changed = TRUE;
        view->size = xfce_panel_plugin_get_size(view->plugin);
        
        icon_presence_changed = TRUE;
        view->show_button_icon = cfg->show_button_icon;
        
        label_presence_changed = TRUE;
        view->show_button_label = cfg->show_button_label;
        
        label_tooltip_changed = TRUE;
        view->label_tooltip_text = g_strdup(cfg->label);

        theme_changed = TRUE;
        view->force_update_theme = FALSE;

    }else{
        orientation_changed    = (view->orientation != xfce_panel_plugin_get_orientation(view->plugin));
        size_changed           = (view->size != xfce_panel_plugin_get_size(view->plugin));
        icon_presence_changed  = (view->show_button_icon != cfg->show_button_icon);
        label_presence_changed = (view->show_button_label != cfg->show_button_label);
        label_tooltip_changed  = (strcmp(view->label_tooltip_text, cfg->label) != 0);
        theme_changed          = view->force_update_theme;
    }

    DBG("orientation: %1x, size: %1x, icon_pr: %1x, label_pr: %1x, label_tooltip: %1x",
        orientation_changed, size_changed, 
        icon_presence_changed, label_presence_changed, 
        label_tooltip_changed);

    if(orientation_changed){
        GtkWidget *button_box = gtk_bin_get_child(GTK_BIN(view->button));
        
        view->orientation = xfce_panel_plugin_get_orientation(view->plugin);
        xfce_hvbox_set_orientation(XFCE_HVBOX(button_box), view->orientation);

        size_changed = TRUE;
    }
    
    if(label_tooltip_changed){
        g_free(view->label_tooltip_text);
        view->label_tooltip_text = g_strdup(cfg->label);
    }

    if(theme_changed)
        view->force_update_theme = FALSE;

    if(size_changed || icon_presence_changed || label_presence_changed || theme_changed){
        view->size              = xfce_panel_plugin_get_size(view->plugin);
        DBG("Panel size: %d", view->size);
        view->show_button_icon  = cfg->show_button_icon;
        view->show_button_label = cfg->show_button_label;
        
        GdkPixbuf *icon;
        gint width, height;
        gint button_overhead_size, box_overhead_size;
        GtkWidget *button_box = gtk_bin_get_child(GTK_BIN(view->button));
        
        button_overhead_size = 2 + 2 *  MAX(view->button->style->xthickness,
                                            view->button->style->ythickness);
        box_overhead_size = 0;
        width = 0;
        height = 0;

        if(view->show_button_icon){
            icon = xfce_themed_icon_load_category(2, view->size - button_overhead_size);
            if(G_LIKELY(icon != NULL)){
                
                width  = MAX(gdk_pixbuf_get_width(icon),  view->size - button_overhead_size);
                height = MAX(gdk_pixbuf_get_height(icon), view->size - button_overhead_size);
             
                if(view->button_image == NULL){
                    view->button_image = g_object_ref(gtk_image_new_from_pixbuf(icon));
                    gtk_widget_show(view->button_image);
                    gtk_box_pack_start_defaults(GTK_BOX(button_box),
                                                view->button_image);
                }else{
                    g_assert(GTK_IS_WIDGET(view->button_image));
                    gtk_image_set_from_pixbuf(GTK_IMAGE(view->button_image), icon);
                }

                g_object_unref(G_OBJECT(icon));
            }else{
                DBG("Could not load icon for button");
            }

        }else if(view->button_image != NULL){
            g_assert(GTK_IS_WIDGET(view->button_image));
            gtk_widget_destroy(view->button_image);
            g_object_unref(view->button_image);
            view->button_image = NULL;
        }

        if(view->show_button_label){
            GtkRequisition req;
            
            if(view->button_label == NULL){
                view->button_label = g_object_ref(gtk_label_new(cfg->label));
                gtk_widget_show(view->button_label);
                gtk_box_pack_end_defaults(GTK_BOX(button_box), 
                                          view->button_label);
            }else{
                g_assert(GTK_IS_WIDGET(view->button_label));
                if(label_tooltip_changed)
                    gtk_label_set_text(GTK_LABEL(view->button_label), view->label_tooltip_text);
            }

            gtk_widget_size_request(view->button_label, &req);
            if(view->orientation == GTK_ORIENTATION_HORIZONTAL){
                width += req.width;
                height = MAX(height, req.height);
            }else{
                width = MAX(width, req.width);
                height += req.height;
            }

        }else if(view->button_label != NULL){
            g_assert(GTK_IS_WIDGET(view->button_label));
            gtk_widget_destroy(view->button_label);
            g_object_unref(view->button_label);
            view->button_label = NULL;
        }

        width += button_overhead_size;
        height += button_overhead_size;

        if(view->button_image != NULL && view->button_label != NULL){

            box_overhead_size = gtk_box_get_spacing(GTK_BOX(button_box));

            if(view->orientation == GTK_ORIENTATION_HORIZONTAL)
                width += box_overhead_size;
            else
                height += box_overhead_size;
        }

        if(view->orientation == GTK_ORIENTATION_HORIZONTAL)
            height = MAX(height, view->size);
        else
            width = MAX(width, view->size);

        gtk_widget_show_all(view->button);

        DBG("width=%d, height=%d", width, height);
        gtk_widget_set_size_request(view->button, width, height);
    }

    if(label_tooltip_changed)
        gtk_tooltips_set_tip(view->tooltips, view->button, view->label_tooltip_text, NULL);
}

static void
pview_cfg_dialog_close_cb(GtkDialog *dialog, gint response, PlacesView *view)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
    xfce_panel_plugin_unblock_menu(view->plugin);
    places_cfg_view_iface_save(view->cfg_iface);
}

static GtkWidget*
pview_make_empty_cfg_dialog(PlacesView *view)
{
    GtkWidget *dlg; /* we'll return this */

    xfce_panel_plugin_block_menu(view->plugin);
    
    dlg = xfce_titled_dialog_new_with_buttons(_("Places"),
              GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(view->plugin))),
              GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
              GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT, NULL);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "xfce4-settings");

    g_signal_connect(dlg, "response",
                     G_CALLBACK(pview_cfg_dialog_close_cb), view);

    return dlg;
}

/********** Initialization & Finalization **********/
PlacesView*
places_view_init(XfcePanelPlugin *plugin)
{
    GtkWidget *button_box;
    PlacesView *view;                   /* internal use in this file */
    PlacesViewCfgIface *view_cfg_iface; /* given to cfg */

    DBG("initializing");
    g_assert(plugin != NULL);

    view            = g_new0(PlacesView, 1);
    view->plugin    = plugin;
    
    view_cfg_iface                          = g_new0(PlacesViewCfgIface, 1);
    view_cfg_iface->places_view             = view;
    view_cfg_iface->update_menu             = pview_update_menu;
    view_cfg_iface->update_button           = pview_button_update;
    view_cfg_iface->reconfigure_model       = pview_reconfigure_model;
    view_cfg_iface->make_empty_cfg_dialog   = pview_make_empty_cfg_dialog;
    
    view->view_cfg_iface = view_cfg_iface;
    view->cfg_iface      = places_cfg_new(view_cfg_iface, 
                                          xfce_panel_plugin_lookup_rc_file(view->plugin),
                                          xfce_panel_plugin_save_location(view->plugin, TRUE));
    view->cfg            = places_cfg_view_iface_get_cfg(view->cfg_iface);

    pview_reconfigure_model(view);
    
    view->tooltips = g_object_ref_sink(gtk_tooltips_new());

    /* init button */

    DBG("init GUI");

    /* create the button */
    view->button = g_object_ref(xfce_create_panel_toggle_button());
    gtk_widget_show(view->button);
    gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
    xfce_panel_plugin_add_action_widget(view->plugin, view->button);

    /* create the box */
    button_box = xfce_hvbox_new(xfce_panel_plugin_get_orientation(view->plugin),
                                FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(button_box), 0);
    gtk_container_add(GTK_CONTAINER(view->button), button_box);

    pview_button_update(view);

    /* signals for icon theme/screen changes */
    g_signal_connect_swapped(view->button, "style-set",
                             G_CALLBACK(pview_cb_theme_changed), view);
    g_signal_connect_swapped(view->button, "screen-changed",
                             G_CALLBACK(pview_cb_theme_changed), view);
    
    /* plugin signals */
    g_signal_connect_swapped(G_OBJECT(view->plugin), "size-changed",
                             G_CALLBACK(pview_cb_size_changed), view);
    g_signal_connect_swapped(G_OBJECT(view->plugin), "orientation-changed",
                             G_CALLBACK(pview_button_update), view);

    /* button signal */
    g_signal_connect_swapped(view->button, "button-press-event",
                             G_CALLBACK(pview_cb_button_pressed), view);

    /* cfg-related signals */
    g_signal_connect_swapped(G_OBJECT(view->plugin), "configure-plugin",
                             G_CALLBACK(places_cfg_view_iface_open_dialog), view->cfg_iface);
    g_signal_connect_swapped(G_OBJECT(view->plugin), "save",
                             G_CALLBACK(places_cfg_view_iface_save), view->cfg_iface);
    xfce_panel_plugin_menu_show_configure(view->plugin);

    DBG("done");

    return view;
}

void 
places_view_finalize(PlacesView *view)
{
    pview_destroy_menu(view);
    pview_destroy_model(view);
    
    if(view->button_image != NULL)
        g_object_unref(view->button_image);
    if(view->button_label != NULL)
        g_object_unref(view->button_label);
    if(view->button != NULL)
        g_object_unref(view->button);
    if(view->label_tooltip_text != NULL)
        g_free(view->label_tooltip_text);
    g_object_unref(view->tooltips);

    places_cfg_view_iface_finalize(view->cfg_iface);
    
    g_free(view->view_cfg_iface);
    g_free(view);
}

/* vim: set ai et tabstop=4: */
