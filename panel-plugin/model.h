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

#ifndef _XFCE_PANEL_PLACES_MODEL_H
#define _XFCE_PANEL_PLACES_MODEL_H

#include <glib.h>

#define BOOKMARK_ITEM_FUNC(symbol)      void (*symbol) (gpointer, const gchar*, const gchar*, const gchar*)
#define BOOKMARK_SEPARATOR_FUNC(symbol) void (*symbol) (gpointer)

typedef struct
{
    gchar           *label;
    gchar           *uri;
    gchar           *icon;
    gpointer        *data;
} BookmarkInfo;

#endif
// vim: ai et tabstop=4
