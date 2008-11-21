/*  xfce4-places-plugin
 *
 *  Headers for wrappers to open external applications.
 *
 *  Copyright (c) 2007 Diego Ongaro <ongardie@gmail.com>
 *
 *  Error dialog code adapted from thunar's thunar-dialogs.h:
 *      Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#ifndef _XFCE_PANEL_PLACES_SUPPORT_H
#define _XFCE_PANEL_PLACES_SUPPORT_H

#include <glib.h>
#include "model.h"

void
places_load_file_browser(const gchar *path);

void
places_load_terminal(const gchar *path);

void
places_load_file(const gchar *path);

void
places_gui_exec(const gchar *cmd);

PlacesBookmarkAction*
places_create_open_action(const PlacesBookmark *bookmark);

PlacesBookmarkAction*
places_create_open_terminal_action(const PlacesBookmark *bookmark);

void
places_show_error_dialog (const GError *error,
                          const gchar  *format,
                          ...) G_GNUC_PRINTF (2, 3);


#endif
/* vim: set ai et tabstop=4: */
