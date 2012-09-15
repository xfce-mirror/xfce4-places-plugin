/*  xfce4-places-plugin
 *
 *  Defines the interface by which cfg can communicate with view.
 *  Headers for interface by which places.c creates/destroys view.
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

#ifndef _XFCE_PANEL_PLACES_VIEW_H
#define _XFCE_PANEL_PLACES_VIEW_H

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

#define USE_RECENT_DOCUMENTS    GTK_CHECK_VERSION(2,10,0)

typedef struct _PlacesView PlacesView;

/* Init & Finalize */
PlacesView*
places_view_init(XfcePanelPlugin*);

void
places_view_finalize(PlacesView*);

#endif
/* vim: set ai et tabstop=4: */
