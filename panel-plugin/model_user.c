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
#include <glib/gstdio.h>
#include "unescape_uri.c"

typedef struct
{
  GPtrArray *bookmarks;
  gchar     *filename;
  time_t     loaded;
} BookmarksUser;


// internal

static void
places_bookmarks_user_reinit(BookmarksUser *b)
{
    DBG("initializing");
    BookmarkInfo *bi;

    gchar *contents;
    gchar **split;
    gchar **lines;
    int i;
    
    if (!g_file_get_contents(b->filename, &contents, NULL, NULL)) {
        DBG("Error opening gtk bookmarks file");
    }else{
    
        lines = g_strsplit (contents, "\n", -1);
        g_free(contents);
  
        for (i = 0; lines[i]; i++) {
            if(!lines[i][0])
                continue;
    
            bi = g_new0(BookmarkInfo, 1);
            bi->icon = "gnome-fs-directory";

            // See if the line is in the form "file:///path" or "file:///path friendly-name"
            split = g_strsplit(lines[i], " ", 2);
            if(split[1]){
                bi->label = g_strdup(split[1]);
                bi->uri = g_strdup(split[0]);
            }else{
                bi->label = places_unescape_uri_string(g_strrstr(lines[i], "/") + sizeof(gchar));
                bi->uri = g_strdup(lines[i]);
            }

            g_ptr_array_add(b->bookmarks, bi);
            g_free(split);
        }

        g_strfreev(lines);
    }
}

static time_t
places_bookmarks_user_get_mtime(BookmarksUser *b)
{
    struct stat buf;
    if(g_stat(b->filename, &buf) == 0)
        return buf.st_mtime;
    return 0;
}

// external

static BookmarksUser*
places_bookmarks_user_init()
{ 
    BookmarksUser *b = g_new0(BookmarksUser, 1);

    b->filename = g_build_filename(xfce_get_homedir(), ".gtk-bookmarks", NULL);
    b->bookmarks = g_ptr_array_new();
    b->loaded = places_bookmarks_user_get_mtime(b);
    
    places_bookmarks_user_reinit(b);
    return b;
}

static gboolean
places_bookmarks_user_changed(BookmarksUser *b)
{
    time_t mtime = places_bookmarks_user_get_mtime(b);
    
    if(mtime > b->loaded){
        g_ptr_array_free(b->bookmarks, TRUE);
        b->bookmarks = g_ptr_array_new();
        b->loaded = mtime;
        places_bookmarks_user_reinit(b);
        return TRUE;
    }

    return FALSE;
}

static void
places_bookmarks_user_visit(BookmarksUser *b,
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
places_bookmarks_user_finalize(BookmarksUser *b)
{
    g_ptr_array_free(b->bookmarks, TRUE);
    g_free(b->filename);
    g_free(b);
}


// vim: ai et tabstop=4
