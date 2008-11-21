/*  xfce4-places-plugin
 *
 *  This file handles the GUI. It "owns" the model and cfg.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
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
 *  Popup command code adapted from:
 *   - windowlist plugin
 *     windowlist.c - (xfce4-panel plugin that lists open windows)
 *     Copyright (c) 2002-2006  Olivier Fourdan
 *     Copyright (c) 2007       Mike Massonnet <mmassonnet@xfce.com>
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

#define USE_GTK_TOOLTIP_API     GTK_CHECK_VERSION(2,12,0)

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfcegui4/libxfcegui4.h>

#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>

#include <string.h>

#include "view.h"
#include "support.h"
#include "cfg.h"
#include "model.h"
#include "model_system.h"
#include "model_volumes.h"
#include "model_user.h"
#include "xfce4-popup-places.h"
#include "button.h"

struct _PlacesViewCfgIface
{
    PlacesView          *places_view;

    void                (*update_menu)              (PlacesView*);
    void                (*update_button)            (PlacesView*);
    void                (*reconfigure_model)        (PlacesView*);
};

struct _PlacesView
{
    /* plugin */
    XfcePanelPlugin           *plugin;
    
    /* configuration */
    PlacesCfg                 *cfg;
    PlacesViewCfgIface        *view_cfg_iface;
    
    /* view */
    GtkWidget                 *button;
    GtkWidget                 *menu;

#if !USE_GTK_TOOLTIP_API
    GtkTooltips               *tooltips;
#endif

#if USE_RECENT_DOCUMENTS
    gulong                     recent_manager_changed_handler;
#endif

    gboolean                   needs_separator;
    guint                      menu_timeout_id;
    
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
                places_bookmark_group_destroy(bookmark_group);
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
        view->bookmark_groups = g_list_append(view->bookmark_groups, places_bookmarks_volumes_create(view->cfg->mount_open_volumes));

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


static void
pview_bookmark_action_call_wrapper(PlacesView *view, PlacesBookmarkAction *action)
{
    g_assert(action != NULL);

    if(action->may_block){

        gtk_widget_set_sensitive(view->button, FALSE);

        while(gtk_events_pending())
            gtk_main_iteration();

        places_bookmark_action_call(action);
    
        gtk_widget_set_sensitive(view->button, TRUE);

    }else{
        places_bookmark_action_call(action);
    }
}

/********** Gtk Callbacks **********/

/* Panel callbacks */

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
        places_load_file_browser(NULL);

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
    while(gtk_events_pending())
        gtk_main_iteration();

    action = (PlacesBookmarkAction*) g_object_get_data(G_OBJECT(item), "action");
    DBG("Calling action %s", action->label);
    pview_bookmark_action_call_wrapper(pd, action);

}

static gboolean
pview_cb_menu_item_do_alt(PlacesView *pd, GtkWidget *menu_item)
{
    
    PlacesBookmark *bookmark = (PlacesBookmark*) g_object_get_data(G_OBJECT(menu_item), "bookmark");
    GList *actions = bookmark->actions;
    GtkWidget *context, *context_item;
    PlacesBookmarkAction *action;

    if(actions != NULL){

        context = gtk_menu_new();

        do{
            action = (PlacesBookmarkAction*) actions->data;

            context_item = gtk_menu_item_new_with_label(action->label);

            g_object_set_data(G_OBJECT(context_item), "action", action);
            g_signal_connect(context_item, "activate",
                             G_CALLBACK(pview_cb_menu_item_context_act), pd);
            
            gtk_menu_shell_append(GTK_MENU_SHELL(context), context_item);
            gtk_widget_show(context_item);

            actions = actions->next;
        }while(actions != NULL);

        gtk_widget_show(context);
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
    PlacesBookmark *bookmark = (PlacesBookmark*) g_object_get_data(G_OBJECT(menu_item), "bookmark");

    if(event->button == 3 || (event->button == 1 && (ctrl || bookmark->primary_action == NULL)))
        return pview_cb_menu_item_do_alt(pd, menu_item);
    else
        return FALSE;
}

static void
pview_cb_menu_item_activate(GtkWidget *menu_item, PlacesView *view)
{
    PlacesBookmark *bookmark = (PlacesBookmark*) g_object_get_data(G_OBJECT(menu_item), "bookmark");

    pview_bookmark_action_call_wrapper(view, bookmark->primary_action);
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

static void
pview_cb_recent_changed(GtkRecentManager *recent_manager, GtkWidget *recent_menu)
{
    GtkWidget *recent_item;
    int recent_count = 0;
    
    g_object_get(recent_manager,
                 "size", &recent_count,
                 NULL);

    recent_item = gtk_menu_get_attach_widget(GTK_MENU(recent_menu));
    if (GTK_IS_WIDGET(recent_item))
        gtk_widget_set_sensitive(recent_item, recent_count > 0);

    if (recent_count == 0) {
        gtk_menu_popdown(GTK_MENU(recent_menu));
    }
    else {
        while (gtk_events_pending())
            gtk_main_iteration();

        gtk_menu_reposition(GTK_MENU(recent_menu));
    }
}

static gboolean
pview_cb_recent_items_clear(GtkWidget *clear_item, GtkWidget *recent_menu)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();
    
    gint removed = gtk_recent_manager_purge_items(manager, NULL);
    DBG("Cleared %d recent items", removed);

    pview_cb_recent_changed(manager, recent_menu);
    
    return TRUE;
}

static gboolean
pview_cb_recent_items_clear3(GtkWidget *clear_item, GdkEventButton *event, GtkWidget *recent_menu)
{
    return pview_cb_recent_items_clear(clear_item, recent_menu);
}

#endif


/********** UI Helpers **********/

static void
pview_destroy_menu(PlacesView *view)
{
#if USE_RECENT_DOCUMENTS
    GtkRecentManager *recent_manager = gtk_recent_manager_get_default();
#endif

    if(view->menu != NULL) {
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(view->menu));

#if USE_RECENT_DOCUMENTS
        if (view->recent_manager_changed_handler) {
            g_signal_handler_disconnect(recent_manager,
                                        view->recent_manager_changed_handler);
            view->recent_manager_changed_handler = 0;
        }
#endif

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

    GtkWidget *separator;
    GtkWidget *item;
    GdkPixbuf *pb;
    GtkWidget *image;

    /* lazily add separator */
    if(view->needs_separator){

        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), separator);
        gtk_widget_show(separator);

        view->needs_separator = FALSE;
    }

    item = gtk_image_menu_item_new_with_label(bookmark->label);

    /* try to set icon */
    if(view->cfg->show_icons && bookmark->icon != NULL){
        pb = xfce_themed_icon_load(bookmark->icon, 16);
        
        if(G_LIKELY(pb != NULL)){
            image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
        }
    }

    g_object_set_data(G_OBJECT(item), "bookmark", bookmark);

    /* do this always so that the menu doesn't close on right-clicks */
    g_signal_connect(item, "button-release-event",
                     G_CALLBACK(pview_cb_menu_item_press), view);

    if(bookmark->primary_action != NULL){

        g_signal_connect(item, "activate",
                         G_CALLBACK(pview_cb_menu_item_activate),
                         view);

    }
    if(bookmark->force_gray || bookmark->primary_action == NULL){

        /* Gray it out. */
        gtk_widget_set_sensitive(gtk_bin_get_child(GTK_BIN(item)),
                                 FALSE);

    }

    g_signal_connect_swapped(item, "destroy",
                     G_CALLBACK(places_bookmark_destroy), bookmark);

    gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), item);
    gtk_widget_show(item);

}

static void
pview_update_menu(PlacesView *pd)
{
    GList *bookmark_group_li;
    PlacesBookmarkGroup *bookmark_group;
    GList *bookmarks;
    PlacesBookmark *bookmark;
    GtkWidget *separator;

#if USE_RECENT_DOCUMENTS
    GtkWidget *recent_menu;
    GtkWidget *clear_item;
    GtkWidget *recent_item;
    GtkRecentManager *recent_manager = gtk_recent_manager_get_default();
#endif

    DBG("destroy menu");

    /* destroy the old menu, if it exists */
    pview_destroy_menu(pd);

    DBG("building new menu");

    /* Create a new menu */
    pd->menu = gtk_menu_new();
    
    /* make sure the menu popups up in right screen */
    /* need exo_noop for GTK 2.6 and 2.8; starting with 2.10, NULL is OK */
    gtk_menu_attach_to_widget(GTK_MENU(pd->menu), pd->button, (GtkMenuDetachFunc) exo_noop);
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

    /* "Search for Files" or "Recent Documents" -> separator */
#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent || (pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0')){
#else
    if(pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0'){
#endif
        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), separator);
        gtk_widget_show(separator);
    }

    /* Search for files */
    if(pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0'){

        GtkWidget *search_item = gtk_image_menu_item_new_with_mnemonic(_("Search for Files"));
        
        if(pd->cfg->show_icons){
            GtkWidget *search_image = gtk_image_new_from_icon_name("system-search", GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(search_item), search_image);
        }
        
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), search_item);
        gtk_widget_show(search_item);
        
        g_signal_connect_swapped(search_item, "activate",
                                 G_CALLBACK(places_gui_exec), pd->cfg->search_cmd);

    }

    /* Recent Documents */
#if USE_RECENT_DOCUMENTS
    if(pd->cfg->show_recent){

        recent_menu = gtk_recent_chooser_menu_new();

        gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_icons);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_recent_number);

        g_signal_connect(recent_menu, "item-activated", 
                         G_CALLBACK(pview_cb_recent_item_open), pd);
            
        if(pd->cfg->show_recent_clear){

            separator = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), separator);
            gtk_widget_show(separator);
   
            if(pd->cfg->show_icons){
                clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
            }else{
                GtkStockItem clear_stock_item;
                gtk_stock_lookup(GTK_STOCK_CLEAR, &clear_stock_item);
                clear_item = gtk_menu_item_new_with_mnemonic(clear_stock_item.label);
            }

            gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), clear_item);
            gtk_widget_show(clear_item);

            /* try button-release-event to catch mouse clicks and not hide the menu after */
            g_signal_connect(clear_item, "button-release-event",
                             G_CALLBACK(pview_cb_recent_items_clear3), recent_menu);
            /* use activate when button-release-event doesn't catch it (e.g., enter key pressed) */
            g_signal_connect(clear_item, "activate",
                             G_CALLBACK(pview_cb_recent_items_clear), recent_menu);
    
        }
    
        recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
        if(pd->cfg->show_icons){
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                          gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
        }
        
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
        gtk_widget_show(recent_menu);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), recent_item);
        gtk_widget_show(recent_item);

        pd->recent_manager_changed_handler = g_signal_connect(recent_manager, "changed",
                                                              G_CALLBACK(pview_cb_recent_changed), recent_menu);
        pview_cb_recent_changed(recent_manager, recent_menu);
    }
#endif

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->menu, "deactivate",
                             G_CALLBACK(pview_cb_menu_deact), pd);

    /* Quit hiding the menu */
    gtk_widget_show(pd->menu);

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

static GdkPixbuf*
pview_pixbuf_factory(gint size)
{
    return xfce_themed_icon_load_category(2, size);
}

static void
pview_button_update(PlacesView *view)
{
    PlacesCfg *cfg = view->cfg;
    PlacesButton *button = (PlacesButton*) view->button;

    static guint tooltip_text_hash = 0;
    guint new_tooltip_text_hash;

    places_button_set_label(button,
                  cfg->show_button_label ? cfg->label : NULL);

    places_button_set_pixbuf_factory(button,
                  cfg->show_button_icon  ? pview_pixbuf_factory : NULL);

    /* tooltips */
    new_tooltip_text_hash = g_str_hash(cfg->label);
    if (new_tooltip_text_hash != tooltip_text_hash) {

#if USE_GTK_TOOLTIP_API
        gtk_widget_set_tooltip_text(view->button, cfg->label);
#else
        gtk_tooltips_set_tip(view->tooltips, view->button, cfg->label, NULL);
#endif

    }
    tooltip_text_hash = new_tooltip_text_hash;

}
/********** Handle user message **********/
static gboolean
pview_popup_command_message_received(GtkWidget *widget,
                                     GdkEventClient *ev,
                                     PlacesView *view)
{
    if (G_LIKELY (ev->data_format == 8 && *(ev->data.b) != '\0'
                  && !g_ascii_strcasecmp (PLACES_MSG_MENU, ev->data.b))){
        DBG ("Message received: '%s'", ev->data.b);

        pview_open_menu (view);
        return TRUE;
    }

    DBG("Bad message received");
    return FALSE;
}

static gboolean
pview_popup_command_set_selection(PlacesView *view)
{
    GdkScreen          *gscreen;
    gchar               selection_name[256];
    Atom                selection_atom;
    Window              id;

    id = GDK_WINDOW_XID (GTK_WIDGET (view->plugin)->window);

    gscreen = gtk_widget_get_screen (GTK_WIDGET(view->plugin));

    g_snprintf (selection_name, 256,
                XFCE_PLACES_SELECTION"%d",
                gdk_screen_get_number (gscreen));
    
    selection_atom = XInternAtom (GDK_DISPLAY (), selection_name, FALSE);

    if (XGetSelectionOwner (GDK_DISPLAY (), selection_atom))
        return FALSE;

    XSetSelectionOwner (GDK_DISPLAY (), selection_atom, id, GDK_CURRENT_TIME);

    g_signal_connect (GTK_WIDGET(view->plugin),
                      "client-event",
                      G_CALLBACK (pview_popup_command_message_received),
                      view);

    return TRUE;
}

/********** Initialization & Finalization **********/
PlacesView*
places_view_init(XfcePanelPlugin *plugin)
{
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
    view->view_cfg_iface = view_cfg_iface;

    view->cfg      = places_cfg_new(view->plugin, view_cfg_iface);

    pview_reconfigure_model(view);
    
#if USE_GTK_TOOLTIP_API
    DBG("using GtkTooltip API");
#else
    DBG("using GtkTooltips API");
    view->tooltips = exo_gtk_object_ref_sink(GTK_OBJECT(gtk_tooltips_new()));
#endif

    /* init button */

    DBG("init GUI");

    /* create the button */
    view->button = g_object_ref(places_button_new(view->plugin));
    xfce_panel_plugin_add_action_widget(view->plugin, view->button);
    gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
    gtk_widget_show(view->button);

    pview_button_update(view);

    /* signals for icon theme/screen changes */
    g_signal_connect_swapped(view->button, "style-set",
                             G_CALLBACK(pview_destroy_menu), view);
    g_signal_connect_swapped(view->button, "screen-changed",
                             G_CALLBACK(pview_destroy_menu), view);
    
    /* button signal */
    g_signal_connect_swapped(view->button, "button-press-event",
                             G_CALLBACK(pview_cb_button_pressed), view);



    /* set selection for xfce4-popup-places */
    pview_popup_command_set_selection(view);

    DBG("done");

    return view;
}

void 
places_view_finalize(PlacesView *view)
{
    pview_destroy_menu(view);
    pview_destroy_model(view);

    if(view->button != NULL) {
        g_signal_handlers_disconnect_by_func(view->button,
                                             pview_destroy_menu,
                                             view);
        g_signal_handlers_disconnect_by_func(view->button,
                                             pview_cb_button_pressed,
                                             view);
        g_object_unref(view->button);
        view->button = NULL;
    }

#if !USE_GTK_TOOLTIP_API
    g_object_unref(view->tooltips);
    view->tooltips = NULL;
#endif

    places_cfg_finalize(view->cfg);
    view->cfg = NULL;

    g_free(view->view_cfg_iface);
    view->view_cfg_iface = NULL;

    g_free(view);
}

/* vim: set ai et tabstop=4: */
