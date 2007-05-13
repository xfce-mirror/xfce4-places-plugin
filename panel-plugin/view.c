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

// Config
static void     places_view_load_config(PlacesData*);
static void     places_view_save_config(PlacesData*);
static void     places_view_configure_plugin(PlacesData*);

// UI Helpers
static void     places_view_update_menu(PlacesData*);
static void     places_view_open_menu(PlacesData*);
static void     places_view_destroy_menu(PlacesData*);
static void     places_view_button_update(PlacesData*, gint size);

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

    places_view_load_config(pd);

    pd->view_just_separated = TRUE;
    pd->view_menu = NULL;

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
    gtk_tooltips_set_tip(pd->view_tooltips, pd->view_button, pd->cfg_label, NULL);
    xfce_panel_plugin_add_action_widget(pd->plugin, pd->view_button);
    
    // create the image
    if(pd->cfg_show_image){
        pd->view_button_image = g_object_ref(gtk_image_new());
        gtk_widget_show(pd->view_button_image);
        gtk_box_pack_start(GTK_BOX(pd->view_button_box), pd->view_button_image, TRUE, TRUE, 0);
    }else{
        pd->view_button_image = NULL;
    }

    // create the label
    if(pd->cfg_show_label){
        pd->view_button_label = g_object_ref(gtk_label_new(pd->cfg_label));
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

    g_signal_connect_swapped(G_OBJECT(pd->plugin), "configure-plugin",
                             G_CALLBACK(places_view_configure_plugin), pd);
    g_signal_connect_swapped(G_OBJECT(pd->plugin), "save",
                             G_CALLBACK(places_view_save_config), pd);
    
    xfce_panel_plugin_menu_show_configure(pd->plugin);

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
    
    if(pd->cfg_label != NULL)
        g_free(pd->cfg_label);

}

/********** Config **********/
static void
places_view_default_config(PlacesData *pd)
{   //TODO: header

    pd->cfg_show_image = TRUE;
    pd->cfg_show_label = FALSE;
    pd->cfg_show_icons = TRUE;

    if(pd->cfg_label != NULL){
        g_free(pd->cfg_label);
        pd->cfg_label = g_strdup(_("Places"));
    }

}

static void
places_view_load_config(PlacesData *pd)
{
    XfceRc *rcfile;
    gchar *rcpath;

    rcpath = xfce_panel_plugin_lookup_rc_file(pd->plugin);
    if(rcpath == NULL){
        places_view_default_config(pd);
        return;
    }

    rcfile = xfce_rc_simple_open(rcpath, TRUE);
    g_free(rcpath);
    if(rcfile == NULL){
        places_view_default_config(pd);
        return;
    }

    pd->cfg_show_label = xfce_rc_read_bool_entry(rcfile, "show_label", FALSE);

    if(!pd->cfg_show_label)
        pd->cfg_show_image = TRUE;
    else
        pd->cfg_show_image = xfce_rc_read_bool_entry(rcfile, "show_image", TRUE);

    pd->cfg_show_icons = xfce_rc_read_bool_entry(rcfile, "show_icons", TRUE);

    if(pd->cfg_label != NULL)
        g_free(pd->cfg_label);

    pd->cfg_label = (gchar*) xfce_rc_read_entry(rcfile, "label", NULL);
    if(pd->cfg_label == NULL || *(pd->cfg_label) == '\0')
        pd->cfg_label = _("Places");
    pd->cfg_label = g_strdup(pd->cfg_label);

    xfce_rc_close(rcfile);
}

static void
places_view_save_config(PlacesData *pd)
{
    XfceRc *rcfile;
    gchar *rcpath;
    
    rcpath = xfce_panel_plugin_save_location(pd->plugin, TRUE);
    if(rcpath == NULL)
        return;

    rcfile = xfce_rc_simple_open(rcpath, FALSE);
    g_free(rcpath);
    if(rcfile == NULL)
        return;

    xfce_rc_write_bool_entry(rcfile, "show_image", pd->cfg_show_image);
    xfce_rc_write_bool_entry(rcfile, "show_label", pd->cfg_show_label);
    xfce_rc_write_bool_entry(rcfile, "show_icons", pd->cfg_show_icons);
    xfce_rc_write_entry(rcfile, "label", pd->cfg_label);

    xfce_rc_close(rcfile);

    DBG("configuration has been saved");
}

static void
places_view_configure_plugin_show_changed(GtkComboBox *combo, PlacesData *pd)
{ //TODO header
    gint option;
    gboolean show_image, show_label;
    
    option = gtk_combo_box_get_active(combo);
    show_image = (option == 0 || option == 2);
    show_label = (option == 1 || option == 2);

    if(show_image && !pd->cfg_show_image){
        pd->cfg_show_image = TRUE;

        if(pd->view_button_image == NULL){
            pd->view_button_image = g_object_ref(gtk_image_new());
            gtk_widget_show(pd->view_button_image);
            gtk_box_pack_start(GTK_BOX(pd->view_button_box), pd->view_button_image, TRUE, TRUE, 0);
        }

    }else if(!show_image && pd->cfg_show_image){
        pd->cfg_show_image = FALSE;

        if(pd->view_button_image != NULL){
            g_object_unref(pd->view_button_image);
            gtk_widget_destroy(pd->view_button_image);
            pd->view_button_image = NULL;
        }

    }

    if(show_label && !pd->cfg_show_label){
        pd->cfg_show_label = TRUE;

        if(pd->view_button_label == NULL){
            pd->view_button_label = g_object_ref(gtk_label_new(pd->cfg_label));
            gtk_widget_show(pd->view_button_label);
            gtk_box_pack_end(GTK_BOX(pd->view_button_box), pd->view_button_label, TRUE, TRUE, 0);
        }

    }else if(!show_label && pd->cfg_show_label){
        pd->cfg_show_label = FALSE;
        
        if(pd->view_button_label != NULL){
            g_object_unref(pd->view_button_label);
            gtk_widget_destroy(pd->view_button_label);
            pd->view_button_label = NULL;
        }

    }
    
    places_view_button_update(pd, -1);
}

static gboolean
places_view_configure_plugin_label_changed(GtkWidget *label_entry, GdkEventFocus *event, PlacesData *pd)
{ // TODO header
    if(pd->cfg_label != NULL)
        g_free(pd->cfg_label);
    
    pd->cfg_label = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry))));
    if(*(pd->cfg_label) == '\0'){
        g_free(pd->cfg_label);
        pd->cfg_label = g_strdup(_("Places"));
        gtk_entry_set_text(GTK_ENTRY(label_entry), pd->cfg_label);
    }

    if(pd->cfg_show_label){
        gtk_label_set_text(GTK_LABEL(pd->view_button_label), pd->cfg_label);
        gtk_tooltips_set_tip(pd->view_tooltips, pd->view_button, pd->cfg_label, NULL);
        places_view_button_update(pd, -1);
    }

    return FALSE;
}

static void
places_view_configure_plugin_icons_changed(GtkToggleButton *icons_check, PlacesData *pd)
{
    pd->cfg_show_icons = gtk_toggle_button_get_active(icons_check);
    places_view_destroy_menu(pd);
}

static void
places_view_configure_plugin_done(GtkDialog *dialog, gint response, PlacesData *pd)
{ //TODO header
    gtk_widget_destroy(GTK_WIDGET(dialog));
    xfce_panel_plugin_unblock_menu(pd->plugin);
    places_view_save_config(pd);
}

static void
places_view_configure_plugin(PlacesData *pd)
{
    DBG("configure plugin");
    GtkWidget *dlg, *combo, *label_entry, *icons_check;
    gint active;
    
    xfce_panel_plugin_block_menu(pd->plugin);

    dlg = xfce_titled_dialog_new_with_buttons(_("Places"),
              GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(pd->plugin))),
              GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
              GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT, NULL);

    g_signal_connect(G_OBJECT(dlg), "response",
                     G_CALLBACK(places_view_configure_plugin_done), pd);


    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _("Image Only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _("Label Only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _("Image and Label"));

    if(pd->cfg_show_label)
        if(pd->cfg_show_image)
            active = 2;
        else
            active = 1;
    else
        active = 0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
    
    g_signal_connect(G_OBJECT(combo), "changed",
                     G_CALLBACK(places_view_configure_plugin_show_changed), pd);

    gtk_widget_show(combo);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), combo, TRUE, TRUE, 0);


    // Label entry
    label_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(label_entry), pd->cfg_label);

    g_signal_connect(G_OBJECT(label_entry), "focus-out-event",
                     G_CALLBACK(places_view_configure_plugin_label_changed), pd);

    gtk_widget_show(label_entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), label_entry, TRUE, TRUE, 0);


    // Show Icons
    icons_check = gtk_check_button_new_with_mnemonic(_("Show _icons in menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(icons_check), pd->cfg_show_icons);

    g_signal_connect(G_OBJECT(icons_check), "toggled",
                     G_CALLBACK(places_view_configure_plugin_icons_changed), pd);

    gtk_widget_show(icons_check);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), icons_check, TRUE, TRUE, 0);

    gtk_widget_show(dlg);


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
    gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent_menu), pd->cfg_show_icons);
    g_signal_connect(recent_menu, "item-activated", 
                     G_CALLBACK(places_view_cb_recent_item_open), pd);

    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu),
                          gtk_separator_menu_item_new());

    if(pd->cfg_show_icons){
        clear_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
    }else{
        GtkStockItem clear_stock_item;
        gtk_stock_lookup(GTK_STOCK_CLEAR, &clear_stock_item);
        clear_item = gtk_menu_item_new_with_mnemonic(clear_stock_item.label);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(recent_menu), clear_item);
    g_signal_connect(clear_item, "activate",
                     G_CALLBACK(places_view_cb_recent_items_clear), NULL);
    
    recent_item = gtk_image_menu_item_new_with_label(_("Recent Documents"));
    if(pd->cfg_show_icons){
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(recent_item), 
                                      gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(recent_item), recent_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu), recent_item);
#endif

    /* connect deactivate signal */
    g_signal_connect_swapped(pd->view_menu, "deactivate",
                             G_CALLBACK(places_view_cb_menu_deact), pd);

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

    // Register this menu (for focus, transparency, auto-hide, etc)
    xfce_panel_plugin_register_menu(pd->plugin, GTK_MENU(pd->view_menu));

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
places_view_button_update(PlacesData *pd, gint wsize)
{
    GdkPixbuf *icon;
    gint size, width, height;
    gint pix_w = 0, pix_h = 0;
    GtkOrientation orientation = xfce_panel_plugin_get_orientation(pd->plugin);
    
    if(wsize <= 0)
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
        places_view_button_update(pd, wsize);

    return TRUE;
}

static gboolean
places_view_cb_theme_changed(GSignalInvocationHint *ihint,
                             guint n_param_values, const GValue *param_values,
                             PlacesData *pd)
{
    // update the button
    if(GTK_WIDGET_REALIZED(pd->view_button))
        places_view_button_update(pd, -1);
    
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

    places_view_button_update(pd, -1);
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
places_view_cb_button_pressed(PlacesData *pd, GdkEventButton *evt)
{
    // (it's the way xfdesktop menu does it...)
    if(evt->button != 1 || ((evt->state & GDK_CONTROL_MASK)
                             && !(evt->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK))))
        return FALSE;
    
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

    if(pd->cfg_show_icons && icon != NULL){
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

    pd->view_just_separated = FALSE;
}

static void
places_view_add_menu_sep(gpointer _pd)
{
    g_assert(_pd != NULL);
    PlacesData *pd = (PlacesData*) _pd;

    if(!pd->view_just_separated){
        gtk_menu_shell_append(GTK_MENU_SHELL(pd->view_menu),
                              gtk_separator_menu_item_new());
        pd->view_just_separated = TRUE;
    }
}

// vim: ai et tabstop=4
