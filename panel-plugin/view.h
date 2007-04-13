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

#ifndef _XFCE_PANEL_PLACES_VIEW_H
#define _XFCE_PANEL_PLACES_VIEW_H

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include "places.h"

// Init & Finalize
void            places_view_init(PlacesData*);
static void     places_view_init_menu(PlacesData*);
void            places_view_finalize(PlacesData*);

// UI Helpers
static void     places_view_close_menu(PlacesData*);
static void     places_view_redraw(PlacesData*);

// GTK Callbacks

//  - Panel
static gboolean places_view_cb_size_changed(XfcePanelPlugin*, int, PlacesData*);

//  - Menu
static void     places_view_cb_menu_position(GtkMenu*, int*, int*, gboolean*, PlacesData*);
static void     places_view_cb_menu_close(GtkMenuShell*, PlacesData*);
static void     places_view_cb_menu_open(GtkButton*, PlacesData*);
static void     places_view_cb_menu_item_open(GtkWidget*, const gchar*);

//  - Button
static void     places_view_cb_button_clicked(GtkWidget*);

//  - Recent Documents
static void     places_view_cb_recent_item_open(GtkRecentChooser*, PlacesData*);
static void     places_view_cb_recent_items_clear(GtkWidget*, GdkEventButton*);

// Model Visitor Callbacks
void            places_view_add_menu_item(gpointer _pd, 
                                       const gchar *label, const gchar *uri, const gchar *icon);
void            places_view_add_menu_sep(gpointer _pd);

#endif
// vim: ai et tabstop=4
