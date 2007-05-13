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

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define PLUGIN_NAME "places"

typedef struct
{
  // plugin
  XfcePanelPlugin   *plugin;

  // configuration
  gboolean           cfg_show_image;
  gboolean           cfg_show_label;
  gboolean           cfg_show_icons;
  gchar             *cfg_label;

  // view
  GtkWidget         *view_button;
  GtkWidget         *view_button_box;
  GtkWidget         *view_button_image;
  GtkWidget         *view_button_label;
  GtkWidget         *view_menu;
  GtkTooltips       *view_tooltips;
  gulong             view_theme_timeout_id;
  gboolean           view_just_separated;

  // model
  gpointer           bookmarks;

} PlacesData;

void places_load_thunar(const gchar*);

#endif
// vim: ai et tabstop=4
