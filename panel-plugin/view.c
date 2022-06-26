/*  xfce4-places-plugin
 *
 *  This file handles the GUI. It "owns" the model and cfg.
 *
 *  Copyright (c) 2007-2009 Diego Ongaro <ongardie@gmail.com>
 *  Copyright (c) 2012 Andrzej <ndrwrdck@gmail.com>
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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>

#include <string.h>

#include "view.h"
#include "support.h"
#include "cfg.h"
#include "model.h"
#include "model_system.h"
#include "model_volumes.h"
#include "model_user.h"
#include "button.h"

#ifdef HAVE_LIBNOTIFY
#include "model_volumes_notify.h"
#endif

struct _PlacesView
{
    /* plugin */
    XfcePanelPlugin           *plugin;

    /* configuration */
    PlacesCfg                 *cfg;

    /* view */
    GtkWidget                 *button;
    GtkWidget                 *menu;

    gulong                     recent_manager_changed_handler;
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
pview_cb_menu_timeout(gpointer user_data)
{
    PlacesView *pd = user_data;

    if(!pd->menu_timeout_id)
        goto killtimeout;

    if(pd->menu == NULL || !gtk_widget_is_visible(pd->menu))
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

static void
pview_cb_menu_context_deact(PlacesView *pd, GtkWidget *context_menu)
{
    g_assert(pd != NULL);
    g_assert(pd->menu != NULL && GTK_IS_WIDGET(pd->menu));

    DBG("Context menu deactivate");
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(pd->menu));
}

static void
open_menu_at_pointer (GtkMenu *menu)
{
    GdkDevice    *mouse_device;
    GdkRectangle  rect;
    GdkSeat      *seat;
    GdkWindow    *window;
    gint          x, y;

    window = gdk_display_get_default_group (gdk_display_get_default ());
    seat = gdk_display_get_default_seat (gdk_display_get_default ());
    mouse_device = gdk_seat_get_pointer (seat);
    gdk_window_get_device_position (window,
                                    mouse_device,
                                    &x, &y,
                                    NULL);
    rect.x = x;
    rect.y = y;
    rect.width = gdk_window_get_width (window);
    rect.height = gdk_window_get_height (window);

    gtk_menu_popup_at_rect (menu,
                            window,
                            &rect,
                            GDK_GRAVITY_NORTH_WEST,
                            GDK_GRAVITY_NORTH_WEST,
                            NULL);
}

static void
open_menu_at_widget (GtkMenu   *menu,
                     GtkWidget *widget)
{
    gtk_menu_popup_at_widget (menu,
                              widget,
                              GDK_GRAVITY_SOUTH_WEST,
                              GDK_GRAVITY_NORTH_WEST,
                              NULL);
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
        open_menu_at_pointer (GTK_MENU (context));

        g_signal_connect_swapped(context, "deactivate",
                                 G_CALLBACK(pview_cb_menu_context_deact), pd);

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
      /* Disabled */
      /* Causes plugin to crash when search-cmd text field is edited */
      /* (backspace key is pressed) */
      /* while (gtk_events_pending())*/
      /*      gtk_main_iteration();*/

        gtk_menu_reposition(GTK_MENU(recent_menu));
    }
}

static gboolean
pview_cb_recent_items_clear(GtkWidget *clear_item, GtkWidget *recent_menu)
{
    GtkRecentManager *manager = gtk_recent_manager_get_default();

    DBG("Cleared %d recent items", gtk_recent_manager_purge_items(manager, NULL));

    pview_cb_recent_changed(manager, recent_menu);

    return TRUE;
}

static gboolean
pview_cb_recent_items_clear3(GtkWidget *clear_item, GdkEventButton *event, GtkWidget *recent_menu)
{
    return pview_cb_recent_items_clear(clear_item, recent_menu);
}


/********** UI Helpers **********/

static GdkPixbuf *
pview_get_icon(GIcon *icon)
{
    GtkIconTheme *itheme = gtk_icon_theme_get_default();
    GdkPixbuf *pix = NULL;
    gint width, height, size;

    g_return_val_if_fail(icon != NULL, NULL);

    if (gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height))
        size = MAX(width, height);
    else
        size = 32;

    if (G_IS_THEMED_ICON(icon)) {
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_by_gicon(itheme,
                                                                icon, size,
                                                                GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_FORCE_SIZE);
        if (icon_info) {
            GdkPixbuf *pix_theme = gtk_icon_info_load_icon(icon_info, NULL);
            pix = gdk_pixbuf_copy(pix_theme);
            g_object_unref(G_OBJECT(icon_info));
            g_object_unref(G_OBJECT(pix_theme));
        }
    } else if(G_IS_LOADABLE_ICON(icon)) {
        GInputStream *stream = g_loadable_icon_load(G_LOADABLE_ICON(icon),
                                                    size, NULL, NULL, NULL);
        if (stream) {
            pix = gdk_pixbuf_new_from_stream(stream, NULL, NULL);
            g_object_unref(stream);
        }
    }

    return pix;
}

static void
pview_destroy_menu(PlacesView *view)
{
    GtkRecentManager *recent_manager = gtk_recent_manager_get_default();

    if(view->menu != NULL) {
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(view->menu));

        if (view->recent_manager_changed_handler) {
            g_signal_handler_disconnect(recent_manager,
                                        view->recent_manager_changed_handler);
            view->recent_manager_changed_handler = 0;
        }

        gtk_widget_destroy(view->menu);
        view->menu = NULL;
    }
    view->needs_separator = FALSE;
}

static void
pview_add_menu_item(PlacesView *view, PlacesBookmark *bookmark)
{
    GtkWidget *separator;
    GtkWidget *item;
    GdkPixbuf *pb;
    GtkWidget *image;

    g_assert(view != NULL);
    g_assert(bookmark != NULL);

    /* lazily add separator */
    if(view->needs_separator){

        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), separator);
        gtk_widget_show(separator);

        view->needs_separator = FALSE;
    }

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    item = gtk_image_menu_item_new_with_label(bookmark->label);
    G_GNUC_END_IGNORE_DEPRECATIONS

    /* try to set icon */
    if(view->cfg->show_icons && bookmark->icon != NULL){
        pb = pview_get_icon(bookmark->icon);

        if(G_LIKELY(pb != NULL)){
            image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
            G_GNUC_END_IGNORE_DEPRECATIONS
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
    GtkWidget *recent_menu;
    GtkWidget *clear_item;
    GtkWidget *recent_item;
    GtkRecentManager *recent_manager = gtk_recent_manager_get_default();

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

    /* "Search for Files" or "Recent Documents" -> separator */
    if(pd->cfg->show_recent || (pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0')){
        separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), separator);
        gtk_widget_show(separator);
    }

    /* Search for files */
    if(pd->cfg->search_cmd != NULL && *pd->cfg->search_cmd != '\0'){

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        GtkWidget *search_item = gtk_image_menu_item_new_with_mnemonic(_("Search for Files"));
        G_GNUC_END_IGNORE_DEPRECATIONS

        if(pd->cfg->show_icons){
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            GtkWidget *search_image = gtk_image_new_from_icon_name("system-search", GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(search_item), search_image);
            G_GNUC_END_IGNORE_DEPRECATIONS
        }

        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), search_item);
        gtk_widget_show(search_item);

        g_signal_connect_swapped(search_item, "activate",
                                 G_CALLBACK(places_gui_exec), pd->cfg->search_cmd);

    }

    /* Recent Documents */
    if(pd->cfg->show_recent){

        recent_menu = gtk_recent_chooser_menu_new();

        gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_icons);
        gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent_menu), pd->cfg->show_recent_number);
        gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent_menu), GTK_RECENT_SORT_MRU);

        g_signal_connect(recent_menu, "item-activated",
                         G_CALLBACK(pview_cb_recent_item_open), pd);

        if(pd->cfg->show_recent_clear){

            separator = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), separator);
            gtk_widget_show(separator);

            if(pd->cfg->show_icons){
                G_GNUC_BEGIN_IGNORE_DEPRECATIONS
                clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
                G_GNUC_END_IGNORE_DEPRECATIONS
            }else{
                clear_item = gtk_menu_item_new_with_mnemonic(_("_Clear"));
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

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
        if(pd->cfg->show_icons){
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item),
                                          gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
        }
        G_GNUC_END_IGNORE_DEPRECATIONS

        gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
        gtk_widget_show(recent_menu);

        gtk_menu_shell_append(GTK_MENU_SHELL(pd->menu), recent_item);
        gtk_widget_show(recent_item);

        pd->recent_manager_changed_handler = g_signal_connect(recent_manager, "changed",
                                                              G_CALLBACK(pview_cb_recent_changed), recent_menu);
        pview_cb_recent_changed(recent_manager, recent_menu);
    }

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->menu, "deactivate",
                             G_CALLBACK(pview_cb_menu_deact), pd);

    /* Quit hiding the menu */
    gtk_widget_show(pd->menu);

    /* This helps allocate resources beforehand so it'll pop up faster the first time */
    gtk_widget_realize(pd->menu);
}

static void
pview_open_menu_at (PlacesView   *pd,
                    GtkWidget    *button)
{
    /* check if menu is needed, or it needs an update */
    if(pd->menu == NULL || pview_model_changed(pd->bookmark_groups))
        pview_update_menu(pd);

    /* toggle the button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pd->button), TRUE);

    /* popup menu */
    DBG("menu: %lu", (gulong)pd->menu);
    if (button == NULL)
        open_menu_at_pointer (GTK_MENU (pd->menu));
    else
        open_menu_at_widget (GTK_MENU (pd->menu), button);

    /* menu timeout to poll for model changes */
    if(pd->menu_timeout_id == 0){
        pd->menu_timeout_id = g_timeout_add_seconds_full(G_PRIORITY_LOW, 2,
                                   pview_cb_menu_timeout, pd,
                                   NULL);
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(1);
    }else{
        PLACES_DEBUG_MENU_TIMEOUT_COUNT(0);
    }
}

static void
pview_open_menu(PlacesView *pd)
{
  if (pd != NULL)
    pview_open_menu_at (pd, pd->button);
}

static GdkPixbuf*
pview_pixbuf_factory(gint size)
{
   static const gchar *icons[] = { "system-file-manager",
                                   "xfce-filemanager",
                                   "file-manager",
                                   "folder",
                                   NULL };
   int i = 0;
   GdkPixbuf *pb = NULL;

   while (icons[i] && !pb) {
      pb = xfce_panel_pixbuf_from_source(icons[i], NULL, size);
      i++;
   }
   return pb;
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
        gtk_widget_set_tooltip_text(view->button, cfg->label);
    }
    tooltip_text_hash = new_tooltip_text_hash;

}
/********** Handle user message **********/

/* copied from xfce4-panel/common/utils.c (panel_utils_grab_available) */
static gboolean
pview_grab_available (void)
{
    /* TODO fix for gtk3 */
    return TRUE;
}


static gboolean
pview_remote_event(XfcePanelPlugin *panel_plugin,
                   const gchar     *name,
                   const GValue    *value,
                   PlacesView      *view)
{
  g_return_val_if_fail (value == NULL || G_IS_VALUE (value), FALSE);

  DBG("remote event: %s, %lu", name, (gulong) view);

  if (strcmp (name, "popup") == 0
      && gtk_widget_is_visible (GTK_WIDGET (panel_plugin))
      && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (view->button))
      && pview_grab_available ()) /* checking if there is another menu on the screen */
    {
      if (value != NULL
          && G_VALUE_HOLDS_BOOLEAN (value)
          && g_value_get_boolean (value))
        {
          /* popup the menu under the pointer */
          pview_open_menu_at (view, NULL);
        }
      else
        {
          /* show the menu */
          pview_open_menu(view);
        }
      /* don't popup another menu */
      return TRUE;
    }

  return FALSE;
}

/********** Initialization & Finalization **********/
PlacesView*
places_view_init(XfcePanelPlugin *plugin)
{
    PlacesView   *view;                   /* internal use in this file */
    GtkIconTheme *icon_theme;

    DBG("initializing");
    g_assert(plugin != NULL);

    view            = g_new0(PlacesView, 1);
    view->plugin    = plugin;

    view->cfg      = places_cfg_new(view->plugin);
    g_signal_connect_swapped (G_OBJECT (view->cfg), "button-changed",
                              G_CALLBACK (pview_button_update), view);
    g_signal_connect_swapped (G_OBJECT (view->cfg), "menu-changed",
                              G_CALLBACK (pview_update_menu), view);
    g_signal_connect_swapped (G_OBJECT (view->cfg), "model-changed",
                              G_CALLBACK (pview_reconfigure_model), view);

    pview_reconfigure_model(view);

    /* init button */

    DBG("init GUI");

    /* create the button */
    view->button = g_object_ref(places_button_new(view->plugin));
    xfce_panel_plugin_add_action_widget(view->plugin, view->button);
    gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
    gtk_widget_show(view->button);

    pview_button_update(view);


    /* signals for icon theme/screen changes */
    icon_theme = gtk_icon_theme_get_default ();
    g_signal_connect_swapped (icon_theme, "changed",
                              G_CALLBACK(pview_destroy_menu), view);
    g_signal_connect_swapped(view->button, "screen-changed",
                             G_CALLBACK(pview_destroy_menu), view);

    /* button signal */
    g_signal_connect_swapped(view->button, "button-press-event",
                             G_CALLBACK(pview_cb_button_pressed), view);

    /* remote control signal */
    g_signal_connect(G_OBJECT(view->plugin), "remote-event",
                     G_CALLBACK(pview_remote_event), view);

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

    g_object_unref(view->cfg);
    view->cfg = NULL;

    g_free(view);

#ifdef HAVE_LIBNOTIFY
    pbvol_notify_uninit();
#endif
}

/* vim: set ai et tabstop=4: */
