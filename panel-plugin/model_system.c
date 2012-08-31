/*  xfce4-places-plugin
 *
 *  Model: system bookmarks (e.g., home folder, desktop)
 *
 *  Copyright (c) 2007-2009 Diego Ongaro <ongardie@gmail.com>
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
#include <gio/gio.h>

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>

#define TRASH          1
#define XDG_USER_DIRS  GLIB_CHECK_VERSION(2,14,0)

#define pbg_priv(pbg) ((PBSysData*) pbg->priv)

typedef struct
{

    /* These are the things that might "change" */
    gboolean       check_changed;   /* starts off false to indicate the following are meaningless */
    gchar         *desktop_dir;     /* NULL => no desktop or desktop is same as home */
#if TRASH
    gboolean       trash_is_empty;
    GFile *trash_path;
#endif

} PBSysData;
 
static void
pbsys_finalize_desktop_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);

    if(bookmark->uri != NULL){
        g_free(bookmark->uri);
        bookmark->uri = NULL;
    }
}

#if TRASH
static void
pbsys_finalize_trash_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);

    if(bookmark->icon != NULL){
        g_object_unref(bookmark->icon);
        bookmark->icon = NULL;
    }
}
#endif

#if TRASH
static gboolean
pbsys_trash_is_empty(GFileInfo *trash_info)
{
    guint item_count = g_file_info_get_attribute_uint32(trash_info,
                                                        G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);
    if(item_count == 0)
        return TRUE;
    else
        return FALSE;
}
#endif


static gchar*
pbsys_desktop_dir(void)
{
    const gchar *home_dir = xfce_get_homedir();
    gchar *desktop_dir = NULL;

#if XDG_USER_DIRS
    /* get the xdg desktop directory, or possibly NULL */
    desktop_dir = g_strdup(g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP));

    /* if xdg desktop is the same as home, pretend it's not there */
    if(exo_str_is_equal(desktop_dir, home_dir)){
        g_free(desktop_dir);
        return NULL;
    }
#endif

    /* fall back to ~/Desktop */
    if(desktop_dir == NULL)
        desktop_dir = g_build_filename(home_dir, "Desktop", NULL);

    /* make sure the directory is there */
    if(!g_file_test(desktop_dir, G_FILE_TEST_IS_DIR)){
        g_free(desktop_dir);
        return NULL;
    }

    return desktop_dir;
}

static GList*
pbsys_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = NULL;           /* we'll return this */
    PlacesBookmark *bookmark;
    PlacesBookmarkAction *open, *terminal;
#if TRASH
    GFileInfo *trash_info;
#endif
    const gchar *home_dir = xfce_get_homedir();
    gchar *desktop_dir;

    pbg_priv(bookmark_group)->check_changed = TRUE;

    /* These icon names are consistent with Thunar. */

    /* Home */
    bookmark                = places_bookmark_create((gchar*) g_get_user_name());
    bookmark->uri           = (gchar*) home_dir;
    bookmark->icon          = g_themed_icon_new("user-home");

    terminal                 = places_create_open_terminal_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);

#if TRASH
    /* Trash */
    bookmark                = places_bookmark_create(_("Trash"));
    bookmark->uri           = "trash:///";
    bookmark->uri_scheme    = PLACES_URI_SCHEME_TRASH;
    bookmark->finalize      = pbsys_finalize_trash_bookmark;;

    /* Try for an icon to indicate whether trash is empty or not */
    
    trash_info = g_file_query_info(pbg_priv(bookmark_group)->trash_path,
                    "trash::*",
                    G_FILE_QUERY_INFO_NONE,
                    NULL, NULL);
    pbg_priv(bookmark_group)->trash_is_empty = pbsys_trash_is_empty(trash_info);
    if (bookmark->icon != NULL)
        g_object_unref(bookmark->icon);
    if (pbg_priv(bookmark_group)->trash_is_empty)
        bookmark->icon = g_themed_icon_new("user-trash");
    else
        bookmark->icon = g_themed_icon_new("user-trash-full");
    g_object_unref(trash_info);

    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);
#endif

    /* Desktop */
    desktop_dir = pbsys_desktop_dir();

    g_free(pbg_priv(bookmark_group)->desktop_dir);
    pbg_priv(bookmark_group)->desktop_dir = g_strdup(desktop_dir);

    if(desktop_dir != NULL){
        bookmark                = places_bookmark_create(_("Desktop"));
        bookmark->uri           = desktop_dir;
        bookmark->icon          = g_themed_icon_new("user-desktop");
        bookmark->finalize      = pbsys_finalize_desktop_bookmark;


        terminal                 = places_create_open_terminal_action(bookmark);
        bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
        open                     = places_create_open_action(bookmark);
        bookmark->actions        = g_list_prepend(bookmark->actions, open);
        bookmark->primary_action = open;

        bookmarks = g_list_append(bookmarks, bookmark);
    }

    /* File System (/) */
    bookmark                = places_bookmark_create(_("File System"));
    bookmark->uri           = "/";
    bookmark->icon          = g_themed_icon_new("drive-harddisk");

    terminal                 = places_create_open_terminal_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
    open                     = places_create_open_action(bookmark);
    bookmark->actions        = g_list_prepend(bookmark->actions, open);
    bookmark->primary_action = open;

    bookmarks = g_list_append(bookmarks, bookmark);

    return bookmarks;
};

static gboolean
pbsys_changed(PlacesBookmarkGroup *bookmark_group)
{
    gchar *desktop_dir;
#if TRASH
    gboolean trash_is_empty;
    GFileInfo *trash_info;
#endif

    if(!pbg_priv(bookmark_group)->check_changed)
        return FALSE;
    
    /* Check if desktop now exists and didn't before */
    desktop_dir = pbsys_desktop_dir();
    if(!exo_str_is_equal(desktop_dir, pbg_priv(bookmark_group)->desktop_dir)){
        g_free(desktop_dir);
        return TRUE;
    }else
        g_free(desktop_dir);

#if TRASH
    /* see if trash gets a different icon (e.g., was empty, now full) */
    trash_info = g_file_query_info(pbg_priv(bookmark_group)->trash_path,
                    "trash::*",
                    G_FILE_QUERY_INFO_NONE,
                    NULL, NULL);
    trash_is_empty = pbsys_trash_is_empty(trash_info);
    g_object_unref(trash_info);
    
    if(trash_is_empty != pbg_priv(bookmark_group)->trash_is_empty)
        return TRUE;
#endif

    return FALSE;
}

static void
pbsys_finalize(PlacesBookmarkGroup *bookmark_group)
{
#if TRASH
    g_object_unref(pbg_priv(bookmark_group)->trash_path);
#endif

    g_free(pbg_priv(bookmark_group)->desktop_dir);

    g_free(pbg_priv(bookmark_group));

}

PlacesBookmarkGroup*
places_bookmarks_system_create(void)
{
    PlacesBookmarkGroup *bookmark_group;
    
    bookmark_group = places_bookmark_group_create();
    bookmark_group->get_bookmarks = pbsys_get_bookmarks;
    bookmark_group->changed       = pbsys_changed;
    bookmark_group->finalize      = pbsys_finalize;
    bookmark_group->priv          = g_new0(PBSysData, 1);
    
#if TRASH
    pbg_priv(bookmark_group)->trash_path = g_file_new_for_uri("trash:///");
#endif

    return bookmark_group;
}
   

/* vim: set ai et tabstop=4: */
