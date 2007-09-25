/*  xfce4-places-plugin
 *
 *  Model: system bookmarks (e.g., home folder, desktop)
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
#include "model.h"
#include "support.h"

#include <string.h>

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#define EXO_API_SUBJECT_TO_CHANGE
#include <thunar-vfs/thunar-vfs.h>


#define pbg_priv(pbg) ((PBSysData*) pbg->priv)

typedef struct
{
    ThunarVfsPath *trash_path;

    /* These are the things that might "change" */
    gboolean       check_changed;   /* starts off false to indicate the following are meaningless */
    gboolean       desktop_exists;
    gboolean       trash_is_empty;

} PBSysData;
 
static void
pbsys_free_desktop_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);

    if(bookmark->uri != NULL)
        g_free(bookmark->uri);

    g_free(bookmark);
}

static void
pbsys_free_trash_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);

    if(bookmark->icon != NULL)
        g_free(bookmark->icon);

    g_free(bookmark);
}

static GList*
pbsys_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = NULL;           /* we'll return this */
    PlacesBookmark *bookmark;
    PlacesBookmarkAction *open, *terminal;
    ThunarVfsInfo *trash_info;
    const gchar *home_dir = xfce_get_homedir();

    pbg_priv(bookmark_group)->check_changed = TRUE;

    /* These icon names are consistent with Thunar. */

    /* Home */
    bookmark                = g_new0(PlacesBookmark, 1);
    bookmark->label         = (gchar*) g_get_user_name();
    bookmark->uri           = (gchar*) home_dir;
    bookmark->icon          = "gnome-fs-home";

    terminal                 = places_create_open_terminal_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);

    /* Trash */
    bookmark                = g_new0(PlacesBookmark, 1);
    bookmark->label         = _("Trash");
    bookmark->uri           = "trash:///";
    bookmark->uri_scheme    = PLACES_URI_SCHEME_TRASH;
    bookmark->free          = pbsys_free_trash_bookmark;;

    /* Try for an icon from ThunarVFS to indicate whether trash is empty or not */
    
    trash_info = thunar_vfs_info_new_for_path(pbg_priv(bookmark_group)->trash_path, NULL);
    if(trash_info->custom_icon != NULL){
        bookmark->icon = g_strdup(trash_info->custom_icon);
        pbg_priv(bookmark_group)->trash_is_empty = (strcmp("gnome-fs-trash-full", bookmark->icon) != 0);
    }else{
        bookmark->icon = g_strdup("gnome-fs-trash-full");
        pbg_priv(bookmark_group)->trash_is_empty = FALSE;
    }
    thunar_vfs_info_unref(trash_info);

    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);

    /* Desktop */
    bookmark                = g_new0(PlacesBookmark, 1);
    bookmark->label         = _("Desktop");
    bookmark->uri           = g_build_filename(home_dir, "Desktop", NULL);
    bookmark->icon          = "gnome-fs-desktop";
    bookmark->free          = pbsys_free_desktop_bookmark;

    if(g_file_test(bookmark->uri, G_FILE_TEST_IS_DIR)){
    
        terminal                 = places_create_open_terminal_action(bookmark);
        bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
        open                     = places_create_open_action(bookmark);
        bookmark->actions        = g_list_prepend(bookmark->actions, open);
        bookmark->primary_action = open;

        pbg_priv(bookmark_group)->desktop_exists = TRUE;
        bookmarks = g_list_append(bookmarks, bookmark);

    }else{

        pbg_priv(bookmark_group)->desktop_exists = FALSE;
        places_bookmark_free(bookmark);

    }
    
    /* File System (/) */
    bookmark                = g_new0(PlacesBookmark, 1);
    bookmark->label         = _("File System");
    bookmark->uri           = "/";
    bookmark->icon          = "gnome-dev-harddisk";

    terminal                 = places_create_open_terminal_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);

    return bookmarks;
};

gboolean
pbsys_changed(PlacesBookmarkGroup *bookmark_group)
{
    gchar *uri;
    gboolean trash_is_empty = FALSE;
    ThunarVfsInfo *trash_info;
    
    if(!pbg_priv(bookmark_group)->check_changed)
        return FALSE;
    
    /* Check if desktop now exists and didn't before */
    uri = g_build_filename(xfce_get_homedir(), "Desktop", NULL);
    if(g_file_test(uri, G_FILE_TEST_IS_DIR) != pbg_priv(bookmark_group)->desktop_exists){
        g_free(uri);
        return TRUE;
    }else
        g_free(uri);

    /* see if trash gets a different icon (e.g., was empty, now full) */
    trash_info = thunar_vfs_info_new_for_path(pbg_priv(bookmark_group)->trash_path, NULL);
    if(trash_info->custom_icon != NULL)
        trash_is_empty = (strcmp("gnome-fs-trash-full", trash_info->custom_icon) != 0);
    thunar_vfs_info_unref(trash_info);
    
    if(trash_is_empty != pbg_priv(bookmark_group)->trash_is_empty)
        return TRUE;

    return FALSE;
}

static void
pbsys_finalize(PlacesBookmarkGroup *bookmark_group)
{
    thunar_vfs_path_unref(pbg_priv(bookmark_group)->trash_path);
    thunar_vfs_shutdown();
    
    g_free(pbg_priv(bookmark_group));

    g_free(bookmark_group);
}

PlacesBookmarkGroup*
places_bookmarks_system_create()
{
    PlacesBookmarkGroup *bookmark_group = g_new0(PlacesBookmarkGroup, 1);
    bookmark_group->get_bookmarks = pbsys_get_bookmarks;
    bookmark_group->changed       = pbsys_changed;
    bookmark_group->finalize      = pbsys_finalize;
    bookmark_group->priv          = g_new0(PBSysData, 1);
    
    thunar_vfs_init();
    pbg_priv(bookmark_group)->trash_path = thunar_vfs_path_get_for_trash();

    return bookmark_group;
}
   

/* vim: set ai et tabstop=4: */
