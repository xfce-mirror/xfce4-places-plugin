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
#include <glib/gstdio.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfcegui4/libxfcegui4.h>

#include "places.h"
#include "unescape_uri.c"

#define PLUGIN_NAME "places"

/********** Structs **********/
static BookmarkInfo*
places_construct_BookmarkInfo(gchar* label, gchar* uri, gchar* icon)
{
    BookmarkInfo *info = g_new(BookmarkInfo, 1);
    info->label = label;
    info->uri = uri;
    info->icon = icon;
    return info;
}


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
    places_init_bookmarks(pd);
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
places_init_bookmarks(PlacesData *pd)
{
    DBG("initializing");

    pd->bookmarks_system = g_ptr_array_new();
    places_init_bookmarks_system(pd);

    pd->bookmarks_user_filename = g_build_filename(xfce_get_homedir(), ".gtk-bookmarks", NULL);
    pd->bookmarks_user_loaded = -1;
    pd->bookmarks_user = g_ptr_array_new();
    places_init_bookmarks_user(pd);

}

static void
places_init_bookmarks_system(PlacesData *pd)
{
    DBG("initializing");

    const gchar *home_dir = xfce_get_homedir();

    // These icon names are consistent with Thunar.

    // Home
    g_ptr_array_add(pd->bookmarks_system, places_construct_BookmarkInfo(g_strdup(g_get_user_name()),
                                                g_strdup(home_dir), "gnome-fs-home"));

    // Trash
    g_ptr_array_add(pd->bookmarks_system, places_construct_BookmarkInfo(_("Trash"), 
                                                "trash:///", "gnome-fs-trash-full"));
    
    // Desktop
    g_ptr_array_add(pd->bookmarks_system, places_construct_BookmarkInfo(_("Desktop"), 
                                                g_build_filename(home_dir, "Desktop", NULL), "gnome-fs-desktop"));
    
    // File System (/)
    g_ptr_array_add(pd->bookmarks_system, places_construct_BookmarkInfo(_("File System"), 
                                                "/", "gnome-dev-harddisk"));
}

static void
places_init_bookmarks_user(PlacesData *pd)
{
    DBG("initializing");

    gchar *contents;
    gchar **split;
    gchar **lines;
    int i;
    
    if (!g_file_get_contents(pd->bookmarks_user_filename, &contents, NULL, NULL)) {
        DBG("Error opening gtk bookmarks file");
    }else{
    
        pd->bookmarks_user_loaded = places_get_bookmarks_user_mtime(pd);

        lines = g_strsplit (contents, "\n", -1);
        g_free (contents);
  
        for (i = 0; lines[i]; i++) {
            if(!lines[i][0])
                continue;
    
            // See if the line is in the form "file:///path" or "file:///path friendly-name"
            split = g_strsplit(lines[i], " ", 2);
            if(split[1]){
                g_ptr_array_add(pd->bookmarks_user, places_construct_BookmarkInfo(g_strdup(split[1]), 
                                                                   g_strdup(split[0]), "gnome-fs-directory"));
            }else{
                g_ptr_array_add(pd->bookmarks_user, places_construct_BookmarkInfo(places_unescape_uri_string(g_strrstr(lines[i], "/") + sizeof(gchar)), 
                                                                   g_strdup(lines[i]), "gnome-fs-directory"));
            }
            g_free(split);

        }

        g_strfreev(lines);
    }
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
places_init_panel_menu(PlacesData *pd)
{
    DBG("initializing");

    pd->panel_menu = gtk_menu_new();
    pd->panel_menu_open = FALSE;
    
    places_init_panel_menu_system(pd);
    
    // separator
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu),
                          places_bookmark_info_to_gtk_menu_item(NULL));

    places_init_panel_menu_user(pd);


    gtk_widget_show(pd->panel_menu);
    // This helps allocate resources beforehand so it'll pop up faster the first time
    gtk_widget_realize(pd->panel_menu);
    
    gtk_menu_attach_to_widget(GTK_MENU(pd->panel_menu), pd->panel_arrow, NULL);

}


static void
places_init_panel_menu_system(PlacesData *pd)
{
    DBG("initializing");

    int k;
    for(k = 0; k < pd->bookmarks_system->len; k++){
        BookmarkInfo *i = g_ptr_array_index(pd->bookmarks_system, k);
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu), 
                              places_bookmark_info_to_gtk_menu_item(i));
     }
}

static void
places_init_panel_menu_user(PlacesData *pd)
{
    DBG("initializing");

    int k;
    for(k = 0; k < pd->bookmarks_user->len; k++){
        BookmarkInfo *i = g_ptr_array_index(pd->bookmarks_user, k);
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->panel_menu), 
                              places_bookmark_info_to_gtk_menu_item(i));

    }
}



/********** Library **********/

static void
places_load_thunar(const gchar * path)
{
    gchar * cmd = g_strconcat("thunar ", path, NULL);
    xfce_exec(cmd, FALSE, FALSE, NULL);
    g_free(cmd);
}

static time_t
places_get_bookmarks_user_mtime(PlacesData *pd)
{
    struct stat buf;
    if(g_stat(pd->bookmarks_user_filename, &buf) == 0)
        return buf.st_mtime;
    return 0;
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

static GtkWidget*
places_bookmark_info_to_gtk_menu_item(BookmarkInfo *i)
{
    GtkWidget *item;
    GtkWidget *image;

    if(i != NULL) { // regular item
        item = gtk_image_menu_item_new_with_label(i->label);
        
        GdkPixbuf *pb = xfce_themed_icon_load(i->icon, 16);
        if(pb != NULL){
            image = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
        }

        g_signal_connect (item, "button-release-event",
                          G_CALLBACK(places_cb_menu_item_act), i);

    } else { //separator
        item = gtk_menu_item_new();
    }
        
    gtk_widget_show(item);

    return item;
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

        if(places_get_bookmarks_user_mtime(pd) > pd->bookmarks_user_loaded){
            DBG("bookmarks file changed");

            gtk_widget_destroy(pd->panel_menu);
            
            g_ptr_array_free(pd->bookmarks_user, TRUE);
            pd->bookmarks_user = g_ptr_array_new();
            places_init_bookmarks_user(pd);

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
places_cb_menu_item_act(GtkWidget *widget, GdkEventButton *event, BookmarkInfo *item)
{
    DBG("load thunar for item");

    places_load_thunar((const gchar*) item->uri);
    
    return FALSE;
}

static gboolean
places_cb_button_act(GtkWidget *button, GdkEventButton *event, gpointer nu)
{
    if(event->button == 3) // Let the right-click menu come up
        return FALSE;
    
    DBG("load thunar at home directory");
    
    // TODO: don't open if they dragged mouse away
    places_load_thunar(g_get_home_dir());

    return FALSE;
}

static void 
places_cb_free_data(XfcePanelPlugin *plugin, PlacesData *pd)
{
    DBG ("Free data: %s", PLUGIN_NAME);
    g_assert(pd != NULL);
    
    g_ptr_array_free(pd->bookmarks_system, TRUE);
    g_ptr_array_free(pd->bookmarks_user, TRUE);
    g_free(pd->bookmarks_user_filename);

    panel_slice_free(PlacesData, pd);
}

// vim: ai et tabstop=4
