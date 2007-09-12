/*  xfce4-places-plugin
 *
 *  This file provides a means of configuring the plugin.
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

#include <string.h>

#include <gtk/gtk.h>

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfcegui4/libxfcegui4.h>

#include "cfg.h"
#include "view.h"
#include "places.h"
#include "model.h"

/* Init */
static void     pcfg_init_defaults(PlacesCfg *cfg);

/* Configuration File */
static void     pcfg_load(PlacesCfg*);
static void     pcfg_save(PlacesCfg*);

/* Configuration Dialog */
static void     pcfg_button_show_cb(GtkComboBox*, PlacesCfg*);
static gboolean pcfg_button_label_cb(GtkWidget *entry, GdkEventFocus*, PlacesCfg*);
#if USE_RECENT_DOCUMENTS
static void     pcfg_recent_num_cb(GtkAdjustment*, PlacesCfg*);
#endif
static void     pcfg_menu_cb(GtkToggleButton*, PlacesCfg*);
static void     pcfg_model_cb(GtkToggleButton*, PlacesCfg*);
//static void     pcfg_dialog_cb(GtkDialog*, gint response, PlacesCfg*);
static void     pcfg_open_dialog(PlacesCfg*);


/********** PlacesCfgViewIface helpers **********/

inline PlacesCfg*
places_cfg_view_iface_get_cfg(PlacesCfgViewIface *iface)
{
    return iface->cfg;
}

inline void
places_cfg_view_iface_open_dialog(PlacesCfgViewIface *iface)
{
    iface->open_dialog(iface->cfg);
}

inline void
places_cfg_view_iface_load(PlacesCfgViewIface *iface)
{
    iface->load(iface->cfg);
}

inline void
places_cfg_view_iface_save(PlacesCfgViewIface *iface)
{
    iface->save(iface->cfg);
}

inline void
places_cfg_view_iface_finalize(PlacesCfgViewIface *iface)
{
    iface->finalize(iface);
}

/********** Initialization & Finalization **********/

static void 
pcfg_finalize(PlacesCfgViewIface *cfg_iface)
{
    PlacesCfg *cfg;

    g_assert(cfg_iface != NULL);

    cfg = cfg_iface->cfg;
    g_assert(cfg != NULL);

    if(cfg->label != NULL)
        g_free(cfg->label);
    if(cfg->search_cmd != NULL)
        g_free(cfg->search_cmd);

    if(cfg->read_path != NULL)
        g_free(cfg->read_path);
    if(cfg->write_path != NULL)
        g_free(cfg->write_path);

    g_free(cfg);
    g_free(cfg_iface);
}

PlacesCfgViewIface*
places_cfg_new(PlacesViewCfgIface *view_iface, gchar *read_path, gchar *write_path)
{
    PlacesCfgViewIface *cfg_iface;
    PlacesCfg *cfg;

    g_assert(view_iface != NULL);

    cfg                     = g_new0(PlacesCfg, 1);
    cfg->read_path          = read_path;
    cfg->write_path         = write_path;
    cfg->view_iface         = view_iface;

    pcfg_load(cfg);
    
    cfg_iface               = g_new0(PlacesCfgViewIface, 1);
    cfg_iface->cfg          = cfg;
    cfg_iface->open_dialog  = pcfg_open_dialog;
    cfg_iface->save         = pcfg_save;
    cfg_iface->finalize     = pcfg_finalize;

    return cfg_iface;
}

/********** Configuration File **********/

static void
pcfg_init_defaults(PlacesCfg *cfg)
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

    if(cfg->search_cmd != NULL)
        g_free(cfg->search_cmd);
    cfg->search_cmd         = g_strdup("");

    if(cfg->label != NULL)
        g_free(cfg->label);
    cfg->label = g_strdup(_("Places"));

}

static void
pcfg_load(PlacesCfg *cfg)
{
    XfceRc *rcfile;

    if(cfg->read_path == NULL){
        pcfg_init_defaults(cfg);
        return;
    }

    rcfile = xfce_rc_simple_open(cfg->read_path, TRUE);
    if(rcfile == NULL){
        pcfg_init_defaults(cfg);
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
    if(cfg->label == NULL || strlen(cfg->label) == 0)
        cfg->label = _("Places");
    cfg->label = g_strdup(cfg->label);

    cfg->search_cmd = (gchar*) xfce_rc_read_entry(rcfile, "search_cmd", NULL);
    if(cfg->search_cmd == NULL)
        cfg->search_cmd = "";
    cfg->search_cmd = g_strdup(cfg->search_cmd);

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
pcfg_save(PlacesCfg *cfg)
{
    XfceRc *rcfile;
    
    if(cfg->write_path == NULL)
        return;

    rcfile = xfce_rc_simple_open(cfg->write_path, FALSE);
    if(rcfile == NULL)
        return;

    /* BUTTON */
    xfce_rc_write_bool_entry(rcfile, "show_button_icon", cfg->show_button_icon);
    xfce_rc_write_bool_entry(rcfile, "show_button_label", cfg->show_button_label);
    xfce_rc_write_entry(rcfile, "label",           cfg->label);

    /* MENU */
    xfce_rc_write_bool_entry(rcfile, "show_icons",     cfg->show_icons);
    xfce_rc_write_bool_entry(rcfile, "show_volumes",   cfg->show_volumes);
    xfce_rc_write_bool_entry(rcfile, "show_bookmarks", cfg->show_bookmarks);
#if USE_RECENT_DOCUMENTS
    xfce_rc_write_bool_entry(rcfile, "show_recent",    cfg->show_recent);

    /* RECENT DOCUMENTS */
    xfce_rc_write_bool_entry(rcfile, "show_recent_clear", cfg->show_recent_clear);
    xfce_rc_write_int_entry(rcfile, "show_recent_number", cfg->show_recent_number);
#endif

    xfce_rc_write_entry(rcfile, "search_cmd", cfg->search_cmd);

    xfce_rc_close(rcfile);

    DBG("configuration saved");
}


/********** Dialog **********/

static void
pcfg_button_show_cb(GtkComboBox *combo, PlacesCfg *cfg)
{
    gint option;
    
    option = gtk_combo_box_get_active(combo);
    cfg->show_button_icon  = (option == 0 || option == 2);
    cfg->show_button_label = (option == 1 || option == 2);

    cfg->view_iface->update_button(cfg->view_iface->places_view);
}

static gboolean
pcfg_button_label_cb(GtkWidget *label_entry, GdkEventFocus *event, PlacesCfg *cfg)
{
    if(cfg->label != NULL)
        g_free(cfg->label);
    
    cfg->label = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry))));
    if(strlen(cfg->label) == 0){
        g_free(cfg->label);
        cfg->label = g_strdup(_("Places"));
        gtk_entry_set_text(GTK_ENTRY(label_entry), cfg->label);
    }

    cfg->view_iface->update_button(cfg->view_iface->places_view);

    return FALSE;
}

static gboolean
pcfg_search_cmd_cb(GtkWidget *label_entry, GdkEventFocus *event, PlacesCfg *cfg)
{
    if(cfg->search_cmd != NULL)
        g_free(cfg->search_cmd);
    
    cfg->search_cmd = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry))));

    cfg->view_iface->update_menu(cfg->view_iface->places_view);

    return FALSE;
}

#if USE_RECENT_DOCUMENTS
static void
pcfg_recent_num_cb(GtkAdjustment *adj, PlacesCfg *cfg)
{
    cfg->show_recent_number = (gint) gtk_adjustment_get_value(adj);
    DBG("Show %d recent documents", cfg->show_recent_number);
    cfg->view_iface->update_menu(cfg->view_iface->places_view);
}
#endif

static void
pcfg_menu_cb(GtkToggleButton *toggle, PlacesCfg *cfg)
{
    gboolean *opt = g_object_get_data(G_OBJECT(toggle), "cfg_opt");
    g_assert(opt != NULL);
    *opt = gtk_toggle_button_get_active(toggle);

    cfg->view_iface->update_menu(cfg->view_iface->places_view);
}

static void
pcfg_model_cb(GtkToggleButton *toggle, PlacesCfg *cfg)
{
    gboolean *opt = g_object_get_data(G_OBJECT(toggle), "cfg_opt");
    g_assert(opt != NULL);
    *opt = gtk_toggle_button_get_active(toggle);

    cfg->view_iface->reconfigure_model(cfg->view_iface->places_view);
    cfg->view_iface->update_menu(cfg->view_iface->places_view);
}

static void
pcfg_open_dialog(PlacesCfg *cfg)
{
    DBG("configure plugin");

    GtkWidget *dlg;
    GtkWidget *frame_button, *vbox_button;
    GtkWidget *frame_menu,   *vbox_menu;
#if USE_RECENT_DOCUMENTS
    GtkWidget *frame_recent, *vbox_recent;
#endif
    GtkWidget *frame_search, *vbox_search;

    GtkWidget *tmp_box, *tmp_label, *tmp_widget;
    gint active;
    
    dlg = cfg->view_iface->make_empty_cfg_dialog(cfg->view_iface->places_view);

    /* BUTTON: frame, vbox */
    vbox_button = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_button);
    
    frame_button = xfce_create_framebox_with_content(_("Button"), vbox_button);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_button, FALSE, FALSE, 0);


    /* BUTTON: Show Icon/Label */
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
                     G_CALLBACK(pcfg_button_show_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    /* BUTTON: Label text entry */
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
                     G_CALLBACK(pcfg_button_label_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    /* MENU: frame, vbox */
    vbox_menu = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_menu);
    
    frame_menu = xfce_create_framebox_with_content(_("Menu"), vbox_menu);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_menu, FALSE, FALSE, 0);

    /* MENU: Show Icons */
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _icons in menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_icons);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_icons));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(pcfg_menu_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);


    /* MENU: Show Removable Media */
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show _removable media"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_volumes);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_volumes));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(pcfg_model_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);   

    /* MENU: Show GTK Bookmarks */
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show GTK _bookmarks"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_bookmarks);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_bookmarks));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(pcfg_model_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);


#if USE_RECENT_DOCUMENTS
    /* MENU: Show Recent Documents */
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show recent _documents"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_recent);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_recent));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(pcfg_menu_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_menu), tmp_widget, FALSE, FALSE, 0);

    /* RECENT DOCUMENTS: frame, vbox */
    vbox_recent = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_recent);
    
    frame_recent = xfce_create_framebox_with_content(_("Recent Documents"), vbox_recent);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_recent, FALSE, FALSE, 0);

    /* RECENT DOCUMENTS: Show clear option */
    tmp_widget = gtk_check_button_new_with_mnemonic(_("Show cl_ear option"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmp_widget), cfg->show_recent_clear);

    g_object_set_data(G_OBJECT(tmp_widget), "cfg_opt", &(cfg->show_recent_clear));
    g_signal_connect(G_OBJECT(tmp_widget), "toggled",
                     G_CALLBACK(pcfg_menu_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(vbox_recent), tmp_widget, FALSE, FALSE, 0);

    /* RECENT DOCUMENTS: Number to display */
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
                     G_CALLBACK(pcfg_recent_num_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);
#endif

    /* Search: frame, vbox */
    vbox_search = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox_search);
    
    frame_search = xfce_create_framebox_with_content(_("Search"), vbox_search);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame_search, FALSE, FALSE, 0);

    /* Search: command */
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
                     G_CALLBACK(pcfg_search_cmd_cb), cfg);

    gtk_widget_show(tmp_widget);
    gtk_box_pack_start(GTK_BOX(tmp_box), tmp_widget, FALSE, FALSE, 0);

    gtk_widget_show(dlg);
}

/* vim: set ai et tabstop=4: */
