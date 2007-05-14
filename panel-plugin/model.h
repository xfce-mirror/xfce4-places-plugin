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

typedef struct
{
    gchar           *label;
    gchar           *uri;
    gchar           *icon;
    gboolean         show;
    gpointer        *data;
} BookmarkInfo;

typedef struct
{
    gpointer   pass_thru;
    void       (*item)        (gpointer, const gchar*, const gchar*, const gchar*);
    void       (*separator)   (gpointer);
} BookmarksVisitor;

typedef struct _Bookmarks Bookmarks;

Bookmarks*
places_bookmarks_init();

#define PLACES_BOOKMARKS_ENABLE_NONE    (0)
#define PLACES_BOOKMARKS_ENABLE_VOLUMES (1)
#define PLACES_BOOKMARKS_ENABLE_USER    (1 << 1)
#define PLACES_BOOKMARKS_ENABLE_ALL     (PLACES_BOOKMARKS_ENABLE_VOLUMES & PLACES_BOOKMARKS_ENABLE_USER)
void
places_bookmarks_enable(Bookmarks *b, gint enable);

void
places_bookmarks_visit(Bookmarks *b, BookmarksVisitor *visitor);

gboolean
places_bookmarks_changed(Bookmarks *b);

void
places_bookmarks_finalize(Bookmarks *b);


#endif
// vim: ai et tabstop=4
