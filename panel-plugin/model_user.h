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

#ifndef _XFCE_PANEL_PLACES_MODEL_USER_H
#define _XFCE_PANEL_PLACES_MODEL_USER_H

#include "model.h"
#include "model_system.h"
#include <glib.h>

typedef struct _BookmarksUser BookmarksUser;

BookmarksUser*
places_bookmarks_user_init(const BookmarksSystem *system);

void
places_bookmarks_user_visit(BookmarksUser *b,
                            gpointer pass_thru, 
                            BOOKMARK_ITEM_FUNC(item_func),
                            BOOKMARK_SEPARATOR_FUNC(separator_func));

gboolean
places_bookmarks_user_changed(BookmarksUser *b);

void
places_bookmarks_user_finalize(BookmarksUser *b);

#endif
// vim: ai et tabstop=4
