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

#include "model_user.h"
#include "model.h"

#include <libxfce4util/libxfce4util.h>
#include <glib.h>
#include <glib/gstdio.h>

#define pbg_priv(pbg) ((PBUserData*) pbg->priv)
#define show_bookmark(b) ((gboolean) b->priv)


typedef struct
{
  GList     *bookmarks;
  gchar     *filename;
  time_t     loaded;

} PBUserData;

static inline time_t
pbuser_get_mtime(const gchar *filename)
{
    struct stat buf;
    if(g_stat(filename, &buf) == 0)
        return buf.st_mtime;
    return 0;
}

static inline gboolean
pbuser_dir_exists(const gchar *path)
{
    return g_file_test(path, G_FILE_TEST_IS_DIR);
}

static void
pbuser_free_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);
    g_free(bookmark->uri);
    g_free(bookmark->label);
    g_free(bookmark);
}

static void
pbuser_destroy_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = pbg_priv(bookmark_group)->bookmarks;

    if(bookmarks == NULL)
        return;
    
    while(bookmarks != NULL){
        pbuser_free_bookmark((PlacesBookmark*) bookmarks->data);
        bookmarks = bookmarks->next;
    }
    g_list_free(bookmarks);
    pbg_priv(bookmark_group)->bookmarks = NULL;
}

static void
pbuser_build_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    /* As of 2007-04-06, this is pretty much taken from/analogous to Thunar */

    GList  *bookmarks = NULL;
    PlacesBookmark *bookmark;
    gchar  *name;
    gchar  *path;
    gchar   line[2048];
    FILE   *fp;
 
    pbuser_destroy_bookmarks(bookmark_group);

    fp = fopen(pbg_priv(bookmark_group)->filename, "r");

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
        
        /* parse the URI */ /* TODO: trash:// URI's */
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
        bookmark        = g_new0(PlacesBookmark, 1);
        bookmark->uri   = path;                                   /* needs to be freed */
        bookmark->label = name;                                   /* needs to be freed */
        bookmark->icon  = "gnome-fs-directory";
        bookmark->priv  = (gpointer) pbuser_dir_exists(path);
        bookmark->free  = pbuser_free_bookmark;

        bookmarks = g_list_prepend(bookmarks, bookmark);
    }

    fclose(fp);

    pbg_priv(bookmark_group)->bookmarks = g_list_reverse(bookmarks);
    pbg_priv(bookmark_group)->loaded    = pbuser_get_mtime(pbg_priv(bookmark_group)->filename);
}


static GList*
pbuser_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks    = pbg_priv(bookmark_group)->bookmarks;
    GList *clone        = NULL;
    PlacesBookmark *bookmark;

    if(bookmarks == NULL){
        pbuser_build_bookmarks(bookmark_group);
        bookmarks = pbg_priv(bookmark_group)->bookmarks;
    }

    bookmarks = g_list_last(bookmarks);

    while(bookmarks != NULL){
        bookmark        = g_memdup(bookmarks->data, sizeof(PlacesBookmark));
        bookmark->uri   = g_strdup(bookmark->uri);
        bookmark->label = g_strdup(bookmark->label);

        clone = g_list_prepend(clone, bookmark);
        bookmarks = bookmarks->prev;
    }

    return clone;

}

static gboolean
pbuser_changed(PlacesBookmarkGroup *bookmark_group)
{
    if(pbg_priv(bookmark_group)->loaded == 0)
        goto pbuser_did_change;

    /* see if the file has changed */
    time_t mtime = pbuser_get_mtime(pbg_priv(bookmark_group)->filename);
    if(mtime > pbg_priv(bookmark_group)->loaded)
        goto pbuser_did_change;
    
    /* see if any directories have been created or removed */
    GList *bookmarks = pbg_priv(bookmark_group)->bookmarks;
    PlacesBookmark *bookmark;
    gboolean ret = FALSE;

    while(bookmarks != NULL){
        bookmark = bookmarks->data;
        if(show_bookmark(bookmark) != pbuser_dir_exists(bookmark->uri)){
            bookmark->priv = (gpointer) !show_bookmark(bookmark);
            ret = TRUE;
        }
        bookmarks = bookmarks->next;
    }
    return ret;

  pbuser_did_change:
    pbuser_destroy_bookmarks(bookmark_group);
    return TRUE;
}

static void
pbuser_finalize(PlacesBookmarkGroup *bookmark_group)
{
    pbuser_destroy_bookmarks(bookmark_group);

    g_free(pbg_priv(bookmark_group)->filename);
    pbg_priv(bookmark_group)->filename = NULL;

    g_free(pbg_priv(bookmark_group));
    g_free(bookmark_group);
}


/* external interface */

PlacesBookmarkGroup*
places_bookmarks_user_create()
{ 
    PlacesBookmarkGroup *bookmark_group = g_new0(PlacesBookmarkGroup, 1);
    bookmark_group->get_bookmarks       = pbuser_get_bookmarks;
    bookmark_group->changed             = pbuser_changed;
    bookmark_group->finalize            = pbuser_finalize;
    bookmark_group->priv                = g_new0(PBUserData, 1);

    pbg_priv(bookmark_group)->filename = xfce_get_homefile(".gtk-bookmarks", NULL);
    
    return bookmark_group;
}

/* vim: set ai et tabstop=4: */
