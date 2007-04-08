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

#ifndef _XFCE_PANEL_PLACES_H
#define _XFCE_PANEL_PLACES_H

#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
  // plugin
  XfcePanelPlugin *plugin;

  // ui
  GtkWidget *panel_box;
  GtkWidget *panel_button;
  GtkWidget *panel_arrow;
  GtkWidget *panel_menu;
  gboolean   panel_menu_open;
  int        panel_size;

  gpointer   bookmarks;

} PlacesData;

// Init
static void places_construct(XfcePanelPlugin*);
static void places_init_ui(PlacesData*);
static void places_init_panel_menu(PlacesData*);

// Library
static void places_load_thunar(const gchar*);

// UI Helpers
static void places_close_menu(PlacesData*);
static void places_ui_redraw(PlacesData*);

// GTK Callbacks
static void places_cb_recent_item_activated(GtkRecentChooser*, PlacesData*);
static gboolean places_cb_size_changed(XfcePanelPlugin*, int, PlacesData*);
static void places_cb_menu_position(GtkMenu*, int*, int*, gboolean*, PlacesData*);
static void places_cb_menu_close(GtkMenuShell*, PlacesData*);
static gboolean places_cb_menu_open(GtkButton*, GdkEventButton*, PlacesData*);
static gboolean places_cb_menu_item_act(GtkWidget*, GdkEventButton*, const gchar*);
static gboolean places_cb_button_act(GtkWidget*, GdkEventButton*, gpointer);
static void places_cb_free_data(XfcePanelPlugin*, PlacesData*);

#endif
// vim: ai et tabstop=4
