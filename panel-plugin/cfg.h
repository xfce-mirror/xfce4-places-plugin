/*  xfce4-places-plugin
 *
 *  Defines the struct holding configuration data.
 *  Defines the interface by which view communicates with cfg.
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
#include "view.h"

typedef struct
{
    /* "private" */
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

typedef struct _PlacesCfgViewIface PlacesCfgViewIface;
struct _PlacesCfgViewIface {
    
    PlacesCfg           *cfg;

    void                (*open_dialog)         (PlacesCfg*);
    void                (*load)                (PlacesCfg*);
    void                (*save)                (PlacesCfg*);
    void                (*finalize)            (PlacesCfgViewIface*);
    
};

inline PlacesCfg*
places_cfg_view_iface_get_cfg(PlacesCfgViewIface*);

inline void
places_cfg_view_iface_open_dialog(PlacesCfgViewIface*);

inline void
places_cfg_view_iface_load(PlacesCfgViewIface*);

inline void
places_cfg_view_iface_save(PlacesCfgViewIface*);

inline void
places_cfg_view_iface_finalize(PlacesCfgViewIface*);


/* PlacesCfg will take ownership of the paths */
PlacesCfgViewIface*
places_cfg_new(PlacesViewCfgIface*,
               gchar *read_path, gchar *write_path);

#endif
/* vim: set ai et tabstop=4: */
