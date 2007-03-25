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

  // bookmarks
  GPtrArray *bookmarks_system;
  GPtrArray *bookmarks_user;
  gchar     *bookmarks_user_filename;
  time_t     bookmarks_user_loaded;

} PlacesData;


typedef struct
{
    gchar *label;
    gchar *uri;
    gchar *icon;
} BookmarkInfo;
static BookmarkInfo* places_construct_BookmarkInfo(gchar* label, gchar* uri, gchar* icon);

// Init
static void places_construct(XfcePanelPlugin*);
 static void places_init_bookmarks(PlacesData*);
  static void places_init_bookmarks_system(PlacesData*);
  static void places_init_bookmarks_user(PlacesData*);
 static void places_init_ui(PlacesData*);
  static void places_init_panel_menu(PlacesData*);
   static void places_init_panel_menu_system(PlacesData*);
   static void places_init_panel_menu_user(PlacesData*);


// Library
static void places_load_thunar(const gchar*);
static time_t places_get_bookmarks_user_mtime(PlacesData*);

// UI Helpers
static void places_close_menu(PlacesData*);
static GtkWidget* places_bookmark_info_to_gtk_menu_item(BookmarkInfo*);
static void places_ui_redraw(PlacesData*);

// GTK Callbacks
static gboolean places_cb_size_changed(XfcePanelPlugin*, int, PlacesData*);
static void places_cb_menu_position(GtkMenu*, int*, int*, gboolean*, PlacesData*);
static void places_cb_menu_close(GtkMenuShell*, PlacesData*);
static gboolean places_cb_menu_open(GtkButton*, GdkEventButton*, PlacesData*);
static gboolean places_cb_menu_item_act(GtkWidget*, GdkEventButton*, BookmarkInfo*);
static gboolean places_cb_button_act(GtkWidget*, GdkEventButton*, gpointer);
static void places_cb_free_data(XfcePanelPlugin*, PlacesData*);


#endif
// vim: ai et tabstop=4
