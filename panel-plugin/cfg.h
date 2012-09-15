/*  xfce4-places-plugin
 *
 *  Defines the struct holding configuration data.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
 *  Copyright (c) 2012 Andrzej <ndrwrdck@gmail.com>
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

typedef struct _PlacesCfgClass PlacesCfgClass;
typedef struct _PlacesCfg      PlacesCfg;

#define XFCE_TYPE_PLACES_CFG            (places_cfg_get_type ())
#define XFCE_PLACES_CFG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PLACES_CFG, PlacesCfg))
#define XFCE_PLACES_CFG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PLACES_CFG, PlacesCfgClass))
#define XFCE_IS_PLACES_CFG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PLACES_CFG))
#define XFCE_IS_PLACES_CFG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_PLACES_CFG))
#define XFCE_PLACES_CFG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_PLACES_CFG, PlacesCfgClass))

GType places_cfg_get_type      (void) G_GNUC_CONST;

struct _PlacesCfg
{
    GObject             __parent__;

    /* "private" */
    XfcePanelPlugin     *plugin;

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

};

struct _PlacesCfgClass
{
  GObjectClass __parent__;
};

void
places_cfg_open_dialog(PlacesCfg*);

void
places_cfg_load(PlacesCfg*);

void
places_cfg_save(PlacesCfg*);

PlacesCfg*
places_cfg_new(XfcePanelPlugin*);

G_END_DECLS

#endif
/* vim: set ai et tabstop=4: */
