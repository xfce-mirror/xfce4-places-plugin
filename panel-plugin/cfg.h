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

#ifndef _XFCE_PANEL_PLACES_CFG_H
#define _XFCE_PANEL_PLACES_CFG_H

#include <glib.h>
#include "places.h"

typedef struct
{
  gboolean           show_button_icon;
  gboolean           show_button_label;
  gchar             *label;
  gboolean           show_icons;
  gboolean           show_volumes;
  gboolean           show_bookmarks;
  gboolean           show_recent;
  gboolean           show_recent_clear;
  gint               show_recent_number;
} PlacesConfig;

// Init & Finalize
PlacesConfig* places_cfg_new(PlacesData*);
void          places_cfg_init_signals(PlacesData*);
void          places_cfg_finalize(PlacesData*);

#endif
// vim: ai et tabstop=4
