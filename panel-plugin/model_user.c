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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "model_system.h"
#include "model_user.h"
#include "model.h"

#include <libxfce4util/libxfce4util.h>
#include <glib/gstdio.h>


#define bookmarks_user_dir_exists(path)    g_file_test(path, G_FILE_TEST_IS_DIR)

struct _BookmarksUser
{
  GPtrArray             *bookmarks;
  gchar                 *filename;
  time_t                 loaded;

  const BookmarksSystem *system;

};

// internal use

static void
places_bookmarks_user_reinit(BookmarksUser *b)
{
    DBG("initializing");
    // As of 2007-04-06, this is pretty much taken from/analogous to Thunar

    BookmarkInfo *bi;
    gchar  *name;
    gchar  *path;
    gchar   line[2048];
    FILE   *fp;
 
    fp = fopen(b->filename, "r");

    if(G_UNLIKELY(fp == NULL)){
        DBG("Error opening gtk bookmarks file");
        return;
    }

    while( fgets(line, sizeof(line), fp) != NULL )
    {
        /* strip leading/trailing whitespace */
        g_strstrip(line);
        
        /* skip over the URI */
        for (name = line; *name != '\0' && !g_ascii_isspace (*name); ++name)
            /* pass */;
        
        /* zero-terminate the URI */
        *name++ = '\0';
        
        /* check if we have a name */
        for (; g_ascii_isspace (*name); ++name)
            /* pass */;
        
        /* parse the URI */ // TODO: trash:// URI's
        path = g_filename_from_uri(line, NULL, NULL);
        if (G_UNLIKELY(path == NULL || *path == '\0'))
            continue;

        /* if we don't have a name, find it in the path */
        if(*name == '\0'){
            name = g_filename_display_basename(path);
            if(*name == '\0'){
                g_free(path);
                continue;
            }
        }else{
            name = g_strdup(name);
        }

        /* create the BookmarkInfo container */
        bi = g_new0(BookmarkInfo, 1);
        bi->uri = path;
        bi->label = name;
        bi->show = bookmarks_user_dir_exists(bi->uri);
        bi->icon = g_strdup("gnome-fs-directory");

        places_bookmarks_system_bi_system_mod((BookmarksSystem*) b->system, bi);

        g_ptr_array_add(b->bookmarks, bi);
    }

    fclose(fp);
}

static time_t
places_bookmarks_user_get_mtime(BookmarksUser *b)
{
    struct stat buf;
    if(g_stat(b->filename, &buf) == 0)
        return buf.st_mtime;
    return 0;
}

// external interface

BookmarksUser*
places_bookmarks_user_init(const BookmarksSystem *system)
{ 
    BookmarksUser *b = g_new0(BookmarksUser, 1);
    
    g_assert(system != NULL);
    b->system = system;

    b->filename = xfce_get_homefile(".gtk-bookmarks", NULL);
    b->bookmarks = g_ptr_array_new();
    b->loaded = places_bookmarks_user_get_mtime(b);
    
    places_bookmarks_user_reinit(b);
    return b;
}

gboolean
places_bookmarks_user_changed(BookmarksUser *b)
{
    // see if the file has changed
    time_t mtime = places_bookmarks_user_get_mtime(b);
    
    if(mtime > b->loaded){
        g_ptr_array_foreach(b->bookmarks, (GFunc) places_bookmark_info_free, NULL);
        g_ptr_array_free(b->bookmarks, TRUE);
        b->bookmarks = g_ptr_array_new();
        b->loaded = mtime;
        places_bookmarks_user_reinit(b);
        return TRUE;
    }

    // see if any directories have been created or removed
    guint k;
    BookmarkInfo *bi;
    gboolean ret = FALSE;

    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        if(bi->show != bookmarks_user_dir_exists(bi->uri)){
            bi->show = !bi->show;
            ret = TRUE;
        }
    }

    return ret;
}

void
places_bookmarks_user_visit(BookmarksUser *b,  BookmarksVisitor *visitor)
{
    guint k;
    BookmarkInfo *bi;
    
    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        if(bi->show)
            visitor->item(visitor->pass_thru, bi->label, bi->uri, bi->icon, NULL);
    }
}

void
places_bookmarks_user_finalize(BookmarksUser *b)
{
    g_ptr_array_foreach(b->bookmarks, (GFunc) places_bookmark_info_free, NULL);
    g_ptr_array_free(b->bookmarks, TRUE);
    b->bookmarks = NULL;

    g_free(b->filename);
    b->filename = NULL;

    b->system = NULL;

    g_free(b);
}

// vim: ai et tabstop=4
