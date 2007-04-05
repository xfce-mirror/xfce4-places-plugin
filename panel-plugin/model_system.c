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

#include "model.h"
#include <libxfce4util/libxfce4util.h>

typedef struct
{
  GPtrArray *bookmarks;
} BookmarksSystem;

static BookmarksSystem*
places_bookmarks_system_init()
{
    BookmarksSystem *b = g_new0(BookmarksSystem, 1);

    BookmarkInfo *bookmark;
    b->bookmarks = g_ptr_array_sized_new(4);
    
    const gchar *home_dir = xfce_get_homedir();

    // These icon names are consistent with Thunar.

    // Home
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = g_strdup(g_get_user_name());
    bookmark->uri = g_strdup(home_dir);
    bookmark->icon = "gnome-fs-home";
    g_ptr_array_add(b->bookmarks, bookmark);

    // Trash
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = _("Trash");
    bookmark->uri = "trash:///";
    bookmark->icon = "gnome-fs-trash-full";
    g_ptr_array_add(b->bookmarks, bookmark);

    // Desktop
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = _("Desktop");
    bookmark->uri = g_build_filename(home_dir, "Desktop", NULL);
    bookmark->icon = "gnome-fs-desktop";
    g_ptr_array_add(b->bookmarks, bookmark);
    
    // File System (/)
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = _("File System");
    bookmark->uri = "/";
    bookmark->icon = "gnome-dev-harddisk";
    g_ptr_array_add(b->bookmarks, bookmark);

    return b;
}

static gboolean
places_bookmarks_system_changed(BookmarksSystem *b)
{
    return FALSE;
}

static void
places_bookmarks_system_visit(BookmarksSystem *b,
                              gpointer pass_thru, 
                              BOOKMARK_ITEM_FUNC(item_func),
                              BOOKMARK_SEPARATOR_FUNC(separator_func))
{
    guint k;
    BookmarkInfo *bi;
    
    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        item_func(pass_thru, bi->label, bi->uri, bi->icon);
    }
}

static void
places_bookmarks_system_finalize(BookmarksSystem *b)
{
    g_ptr_array_free(b->bookmarks, TRUE);
    g_free(b);
}

// vim: ai et tabstop=4
