/*  xfce4-places-plugin
 *
 *  Defines the struct holding configuration data.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
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
#include <libxfce4panel/libxfce4panel.h>
#include "view.h"

typedef struct
{
    /* "private" */
    XfcePanelPlugin     *plugin;
    PlacesViewCfgIface  *view_iface;
    gchar               *read_path;
    gchar               *write_path;

    /* "public" for view's access */
    gboolean            show_button_icon;
    gboolean            show_button_label;
    gboolean            show_icons;
    gboolean            show_volumes;
    gboolean            mount_open_volumes;
    gboolean            show_bookmarks;
#if USE_RECENT_DOCUMENTS
    gboolean            show_recent;
    gboolean            show_recent_clear;
    gint                show_recent_number;
#endif
    gchar               *label;
    gchar               *search_cmd;

} PlacesCfg;

void
places_cfg_open_dialog(PlacesCfg*);

void
places_cfg_load(PlacesCfg*);

void
places_cfg_save(PlacesCfg*);

void
places_cfg_finalize(PlacesCfg*);

PlacesCfg*
places_cfg_new(XfcePanelPlugin*, PlacesViewCfgIface*);

#endif
/* vim: set ai et tabstop=4: */
