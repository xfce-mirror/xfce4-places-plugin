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
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfcegui4/libxfcegui4.h>

#include "view.h"
#include "places.h"
#include "model.h"

// Init
static void     places_cfg_init_defaults(PlacesConfig *cfg);

// Configuration File
static void     places_cfg_load(PlacesData*);
static void     places_cfg_save(PlacesData*);

// Configuration Dialog
static void     places_cfg_button_show_cb(GtkComboBox*, PlacesData*);
static gboolean places_cfg_button_label_cb(GtkWidget *entry, GdkEventFocus*, PlacesData*);
#if USE_RECENT_DOCUMENTS
static void     places_cfg_recent_num_cb(GtkAdjustment*, PlacesData*);
#endif
static void     places_cfg_menu_cb(GtkToggleButton*, PlacesData*);
static void     places_cfg_model_cb(GtkToggleButton*, PlacesData*);
static void     places_cfg_dialog_cb(GtkDialog*, gint response, PlacesData*);
static void     places_cfg_launch_dialog(PlacesData*);

/********** Initialization & Finalization **********/
PlacesConfig*
places_cfg_new(PlacesData *pd)
{
    pd->cfg = g_new0(PlacesConfig, 1);
    places_cfg_load(pd);
    return pd->cfg;
}

static void
places_cfg_init_defaults(PlacesConfig *cfg)
{

    cfg->show_button_icon   = TRUE;
    cfg->show_button_label  = FALSE;
    cfg->show_icons         = TRUE;
    cfg->show_volumes       = TRUE;
    cfg->show_bookmarks     = TRUE;
#if USE_RECENT_DOCUMENTS
    cfg->show_recent        = TRUE;
    cfg->show_recent_clear  = TRUE;
    cfg->show_recent_number = 10;
#endif
    cfg->search_cmd         = NULL;

    if(cfg->label != NULL)
        g_free(cfg->label);
    cfg->label = g_strdup(_("Places"));

}

void
places_cfg_init_signals(PlacesData *pd)
{
    g_signal_connect_swapped(G_OBJECT(pd->plugin), "configure-plugin",
                             G_CALLBACK(places_cfg_launch_dialog), pd);

    g_signal_connect_swapped(G_OBJECT(pd->plugin), "save",
                             G_CALLBACK(places_cfg_save), pd);
    
    xfce_panel_plugin_menu_show_configure(pd->plugin);
}

void 
places_cfg_finalize(PlacesData *pd)
{
    if(pd->cfg->label != NULL)
        g_free(pd->cfg->label);
    if(pd->cfg->search_cmd != NULL)
        g_free(pd->cfg->search_cmd);
}

/********** Configuration File **********/
static void
places_cfg_load(PlacesData *pd)
{
    PlacesConfig *cfg = pd->cfg;

    XfceRc *rcfile;
    gchar *rcpath;

    rcpath = xfce_panel_plugin_lookup_rc_file(pd->plugin);
    if(rcpath == NULL){
        places_cfg_init_defaults(cfg);
        return;
    }

    rcfile = xfce_rc_simple_open(rcpath, TRUE);
    g_free(rcpath);
    if(rcfile == NULL){
        places_cfg_init_defaults(cfg);
        return;
    }

    cfg->show_button_label = xfce_rc_read_bool_entry(rcfile, "show_button_label", FALSE);

    if(!cfg->show_button_label)
        cfg->show_button_icon = TRUE;
    else
        cfg->show_button_icon = xfce_rc_read_bool_entry(rcfile, "show_button_icon", TRUE);

    cfg->show_icons = xfce_rc_read_bool_entry(rcfile, "show_icons", TRUE);

    cfg->show_volumes   = xfce_rc_read_bool_entry(rcfile, "show_volumes", TRUE);
    cfg->show_bookmarks = xfce_rc_read_bool_entry(rcfile, "show_bookmarks", TRUE);

    if(cfg->label != NULL)
        g_free(cfg->label);

    cfg->label = (gchar*) xfce_rc_read_entry(rcfile, "label", NULL);
    if(cfg->label == NULL || *(cfg->label) == '\0')
        cfg->label = _("Places");
    cfg->label = g_strdup(cfg->label);

    cfg->search_cmd = g_strdup(xfce_rc_read_entry(rcfile, "search_cmd", NULL));

#if USE_RECENT_DOCUMENTS
    cfg->show_recent    = xfce_rc_read_bool_entry(rcfile, "show_recent", TRUE);
    cfg->show_recent_clear = xfce_rc_read_bool_entry(rcfile, "show_recent_clear", TRUE);

    cfg->show_recent_number = xfce_rc_read_int_entry(rcfile, "show_recent_number", 10);
    if(cfg->show_recent_number < 1 || cfg->show_recent_number > 25)
        cfg->show_recent_number = 10;
#endif

    xfce_rc_close(rcfile);
}

static void
places_cfg_save(PlacesData *pd)
{
    PlacesConfig *cfg = pd->cfg;
    XfceRc *rcfile;
    gchar *rcpath;
    
    rcpath = xfce_panel_plugin_save_location(pd->plugin, TRUE);
    if(rcpath == NULL)
        return;

    rcfile = xfce_rc_simple_open(rcpath, FALSE);
    g_free(rcpath);
    if(rcfile == NULL)
        return;

    // BUTTON
    xfce_rc_write_bool_entry(rcfile, "show_button_icon", cfg->show_button_icon);
    xfce_rc_write_bool_entry(rcfile, "show_button_label", cfg->show_button_label);
    xfce_rc_write_entry(rcfile, "label",           cfg->label);

    // MENU
    xfce_rc_write_bool_entry(rcfile, "show_icons",     cfg->show_icons);
    xfce_rc_write_bool_entry(rcfile, "show_volumes",   cfg->show_volumes);
    xfce_rc_write_bool_entry(rcfile, "show_bookmarks", cfg->show_bookmarks);
#if USE_RECENT_DOCUMENTS
    xfce_rc_write_bool_entry(rcfile, "show_recent",    cfg->show_recent);

    // RECENT DOCUMENTS
    xfce_rc_write_bool_entry(rcfile, "show_recent_clear", cfg->show_recent_clear);
    xfce_rc_write_int_entry(rcfile, "show_recent_number", cfg->show_recent_number);
#endif

    xfce_rc_write_entry(rcfile, "search_cmd", cfg->search_cmd);

    xfce_rc_close(rcfile);

    DBG("configuration saved");
}


/********** Dialog **********/

static void
places_cfg_button_show_cb(GtkComboBox *combo, PlacesData *pd)
{
    PlacesConfig *cfg = pd->cfg;
    gint option;
    gboolean show_icon, show_label;
    
    option = gtk_combo_box_get_active(combo);
    show_icon  = (option == 0 || option == 2);
    show_label = (option == 1 || option == 2);

    if(show_icon && !cfg->show_button_icon){
        cfg->show_button_icon = TRUE;

        if(pd->view_button_image == NULL){
            pd->view_button_image = g_object_ref(gtk_image_new());
            gtk_widget_show(pd->view_button_image);
            gtk_box_pack_start(GTK_BOX(pd->view_button_box), pd->view_button_image, TRUE, TRUE, 0);
        }

    }else if(!show_icon && cfg->show_button_icon){
        cfg->show_button_icon = FALSE;

        if(pd->view_button_image != NULL){
            g_object_unref(pd->view_button_image);
            gtk_widget_destroy(pd->view_button_image);
            pd->view_button_image = NULL;
        }

    }

    if(show_label && !cfg->show_button_label){
        cfg->show_button_label = TRUE;

        if(pd->view_button_label == NULL){
            pd->view_button_label = g_object_ref(gtk_label_new(cfg->label));
            gtk_widget_show(pd->view_button_label);
            gtk_box_pack_end(GTK_BOX(pd->view_button_box), pd->view_button_label, TRUE, TRUE, 0);
        }

    }else if(!show_label && cfg->show_button_label){
        cfg->show_button_label = FALSE;
        
        if(pd->view_button_label != NULL){
            g_object_unref(pd->view_button_label);
            gtk_widget_destroy(pd->view_button_label);
            pd->view_button_label = NULL;
        }

    }
    
    places_view_button_update(pd);
}

static gboolean
places_cfg_button_label_cb(GtkWidget *label_entry, GdkEventFocus *event, PlacesData *pd)
{
    if(pd->cfg->label != NULL)
        g_free(pd->cfg->label);
    
    pd->cfg->label = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry))));
    if(*(pd->cfg->label) == '\0'){
        g_free(pd->cfg->label);
        pd->cfg->label = g_strdup(_("Places"));
        gtk_entry_set_text(GTK_ENTRY(label_entry), pd->cfg->label);
    }

    if(pd->cfg->show_button_label){
        gtk_label_set_text(GTK_LABEL(pd->view_button_label), pd->cfg->label);
        gtk_tooltips_set_tip(pd->view_tooltips, pd->view_button, pd->cfg->label, NULL);
        places_view_button_update(pd);
    }

    return FALSE;
}

static gboolean
places_cfg_search_cmd_cb(GtkWidget *label_entry, GdkEventFocus *event, PlacesData *pd)
{
    if(pd->cfg->search_cmd != NULL)
        g_free(pd->cfg->search_cmd);
    
    pd->cfg->search_cmd = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry))));
    if(*(pd->cfg->search_cmd) == '\0'){
        g_free(pd->cfg->search_cmd);
        pd->cfg->search_cmd = NULL;
    }

    places_view_destroy_menu(pd);

    return FALSE;
}

#if USE_RECENT_DOCUMENTS
static void
places_cfg_recent_num_cb(GtkAdjustment *adj, PlacesData *pd)
{
    pd->cfg->show_recent_number = (gint) gtk_adjustment_get_value(adj);
    DBG("Show %d recent documents", pd->cfg->show_recent_number);
    places_view_destroy_menu(pd);
}
#endif

static void
places_cfg_menu_cb(GtkToggleButton *toggle, PlacesData *pd)
{
    gboolean *cfg = g_object_get_data(G_OBJECT(toggle), "cfg_opt");
    g_assert(cfg != NULL);
    *cfg = gtk_toggle_button_get_active(toggle);

    places_view_destroy_menu(pd);
}

static void
places_cfg_model_cb(GtkToggleButton *toggle, PlacesData *pd)
{
    gboolean *cfg = g_object_get_data(G_OBJECT(toggle), "cfg_opt");
    g_assert(cfg != NULL);
    *cfg = gtk_toggle_button_get_active(toggle);

    places_view_reconfigure_model(pd);
    places_view_destroy_menu(pd);
}

static void
places_cfg_dialog_cb(GtkDialog *dialog, gint response, PlacesData *pd)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
    xfce_panel_plugin_unblock_menu(pd->plugin);
    places_cfg_save(pd);
}

static void
places_cfg_launch_dialog(PlacesData *pd)
{
    DBG("configure plugin");
    PlacesConfig *cfg = pd->cfg;

    GtkWidget *dlg;
    GtkWidget *frame_button, *vbox_button;
    GtkWidget *frame_menu,   *vbox_menu;
#if USE_RECENT_DOCUMENTS
    GtkWidget *frame_recent, *vbox_recent;
#endif
    GtkWidget *frame_search, *vbox_search;

    GtkWidget *tmp_box, *tmp_label, *tmp_widget;
    gint active;
    
    xfce_panel_plugin_block_menu(pd->plugin);

    dlg = xfce_titled_dialog_new_with_buttons(_("Places"),
              GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(pd->plugin))),
              GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
              GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT, NULL);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "xfce4-settings");

    g_signal_connect(G_OBJECT(dlg), "response",
                     G_CALLBACK(places_cfg_dialog_cb), pd);


    // BUTTON: frame, vbox
    vbox_button = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_button);
    
    frame_button = xfce_create_framebox_with_content(_("Button"), vbox_button);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_button, FALSE, FALSE, 0);


    // BUTTON: Show Icon/Label
    tmp_box = gtk_hbox_new(FALSE, 15);
    gtk_widget_show(tmp_box);
    gtk_box_pack_start(GTK_BOX(vbox_button), tmp_box, FALSE, FALSE, 0);
    
    tmp_label = gtk_label_new_with_mnemonic(_("_Show"));
    gtk_widget_show(tmp_label);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_label, FALSE, FALSE, 0);

    tmp_widget = gtk_combo_box_new_text();
    gtk_label_set_mnemonic_widget(GTK_LABEL(tmp_label), tmp_widget);
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp_widget), _("Icon Only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp_widget), _("Label Only"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(tmp_widget), _("Icon and Label"));

    if(cfg->show_button_label)
        if(cfg->show_button_icon)
            active = 2;
        else
            active = 1;
    else
        active = 0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(tmp_widget), active);
    
    g_signal_connect(G_OBJECT(tmp_widget), "changed",
                     G_CALLBACK(places_cfg_button_show_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    // BUTTON: Label text entry
    tmp_box = gtk_hbox_new(FALSE, 15);
    gtk_widget_show(tmp_box);
    gtk_box_pack_start(GTK_BOX(vbox_button), tmp_box, FALSE, FALSE, 0);
    
    tmp_label = gtk_label_new_with_mnemonic(_("_Label"));
    gtk_widget_show(tmp_label);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_label, FALSE, FALSE, 0);

    tmp_widget = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(tmp_label), tmp_widget);
    gtk_entry_set_text(GTK_ENTRY(tmp_widget), cfg->label);

    g_signal_connect(G_OBJECT(tmp_widget), "focus-out-event",
                     G_CALLBACK(places_cfg_button_label_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    // MENU: frame, vbox
    vbox_menu = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_menu);
    
    frame_menu = xfce_create_framebox_with_content(_("Menu"), vbox_menu);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_menu, FALSE, FALSE, 0);

    // MENU: Show Icons
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _icons in menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_icons);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_icons));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(places_cfg_menu_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);


    // MENU: Show Removable Media
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _removable media"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_volumes);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_volumes));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(places_cfg_model_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);   

    // MENU: Show GTK Bookmarks
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show GTK _bookmarks"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_bookmarks);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_bookmarks));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(places_cfg_model_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);


#if USE_RECENT_DOCUMENTS
    // MENU: Show Recent Documents
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show recent _documents"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_recent);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_recent));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(places_cfg_menu_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);

    // RECENT DOCUMENTS: frame, vbox
    vbox_recent = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_recent);
    
    frame_recent = xfce_create_framebox_with_content(_("Recent Documents"), vbox_recent);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_recent, FALSE, FALSE, 0);

    // RECENT DOCUMENTS: Show clear option
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show cl_ear option"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_recent_clear);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_recent_clear));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(places_cfg_menu_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_recent), tmp_widget, FALSE, FALSE, 0);

    // RECENT DOCUMENTS: Number to display
    tmp_box = gtk_hbox_new(FALSE, 15);
    gtk_widget_show(tmp_box);
    gtk_box_pack_start(GTK_BOX(vbox_recent), tmp_box, FALSE, FALSE, 0);
    
    tmp_label = gtk_label_new_with_mnemonic(_("_Number to display"));
    gtk_widget_show(tmp_label);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_label, FALSE, FALSE, 0);

    GtkObject *adj = gtk_adjustment_new(cfg->show_recent_number, 1, 25, 1, 5, 5);

    tmp_widget = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
    gtk_label_set_mnemonic_widget(GTK_LABEL(tmp_label), tmp_widget);

    g_signal_connect(G_OBJECT(adj), "value-changed",
                     G_CALLBACK(places_cfg_recent_num_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);
#endif

    // Search: frame, vbox
    vbox_search = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_search);
    
    frame_search = xfce_create_framebox_with_content(_("Search"), vbox_search);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_search, FALSE, FALSE, 0);

    // Search: command
    tmp_box = gtk_hbox_new(FALSE, 15);
    gtk_widget_show(tmp_box);
    gtk_box_pack_start(GTK_BOX(vbox_search), tmp_box, FALSE, FALSE, 0);
    
    tmp_label = gtk_label_new_with_mnemonic(_("Co_mmand"));
    gtk_widget_show(tmp_label);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_label, FALSE, FALSE, 0);

    tmp_widget = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(tmp_label), tmp_widget);
    gtk_entry_set_text(GTK_ENTRY(tmp_widget), cfg->search_cmd);

    g_signal_connect(G_OBJECT(tmp_widget), "focus-out-event",
                     G_CALLBACK(places_cfg_search_cmd_cb), pd);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    gtk_widget_show(dlg);
}

// vim: ai et tabstop=4
