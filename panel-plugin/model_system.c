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
#include "model.h"
#include <libxfce4util/libxfce4util.h>
#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>
#include <thunar-vfs/thunar-vfs.h>

#define bookmarks_system_check_existence data

struct _BookmarksSystem
{
    GPtrArray *bookmarks;
    ThunarVfsPath *trash_path;
};


BookmarksSystem*
places_bookmarks_system_init()
{
    thunar_vfs_init();

    BookmarksSystem *b = g_new0(BookmarksSystem, 1);

    BookmarkInfo *bookmark;
    b->bookmarks = g_ptr_array_sized_new(4);
    
    const gchar *home_dir = xfce_get_homedir();

    // These icon names are consistent with Thunar.

    // Home
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = g_strdup(g_get_user_name());
    bookmark->uri = g_strdup(home_dir);
    bookmark->icon = g_strdup("gnome-fs-home");
    bookmark->show = TRUE;
    bookmark->bookmarks_system_check_existence = NULL;
    g_ptr_array_add(b->bookmarks, bookmark);

    // Trash
    bookmark = g_new0(BookmarkInfo, 1);

    bookmark->label = g_strdup(_("Trash"));
    bookmark->uri = g_strdup("trash:///");

    b->trash_path = thunar_vfs_path_get_for_trash();

    ThunarVfsInfo *trash_info = thunar_vfs_info_new_for_path(b->trash_path, NULL);
    bookmark->icon = g_strdup(trash_info->custom_icon);
    if(bookmark->icon == NULL)
        bookmark->icon = g_strdup("gnome-fs-trash-full");
    thunar_vfs_info_unref(trash_info);

    bookmark->show = TRUE;
    bookmark->bookmarks_system_check_existence = NULL;
    g_ptr_array_add(b->bookmarks, bookmark);

    // Desktop
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->uri = g_build_filename(home_dir, "Desktop", NULL);
    bookmark->label = g_strdup(_("Desktop"));
    bookmark->icon = g_strdup("gnome-fs-desktop");
    bookmark->show = g_file_test(bookmark->uri, G_FILE_TEST_IS_DIR);
    bookmark->bookmarks_system_check_existence = (gpointer) 1;
    g_ptr_array_add(b->bookmarks, bookmark);
    
    // File System (/)
    bookmark = g_new0(BookmarkInfo, 1);
    bookmark->label = g_strdup(_("File System"));
    bookmark->uri = g_strdup("/");
    bookmark->icon = g_strdup("gnome-dev-harddisk");
    bookmark->show = TRUE;
    bookmark->bookmarks_system_check_existence = NULL;
    g_ptr_array_add(b->bookmarks, bookmark);

    return b;
}

gboolean
places_bookmarks_system_changed(BookmarksSystem *b)
{
    guint k;
    BookmarkInfo *bi;
    gboolean ret = FALSE;
    
    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        if(bi->bookmarks_system_check_existence && bi->show != g_file_test(bi->uri, G_FILE_TEST_IS_DIR)){
            bi->show = !bi->show;
            ret = TRUE;
        }
    }

    // see if trash gets a different icon (e.g., was empty, now full)
    bi = g_ptr_array_index(b->bookmarks, 1);
    ThunarVfsInfo *trash_info = thunar_vfs_info_new_for_path(b->trash_path, NULL);
    if(trash_info->custom_icon != NULL && !exo_str_is_equal(trash_info->custom_icon, bi->icon)){
        g_free(bi->icon);
        bi->icon = g_strdup(trash_info->custom_icon);
        ret = TRUE;
    }
    thunar_vfs_info_unref(trash_info);

    return ret;
}

void
places_bookmarks_system_visit(BookmarksSystem *b,
                              BookmarksVisitor *visitor)
{
    guint k;
    BookmarkInfo *bi;
    
    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        if(bi->show)
            visitor->item(visitor->pass_thru, bi->label, bi->uri, bi->icon);
    }
}

void
places_bookmarks_system_finalize(BookmarksSystem *b)
{
    g_ptr_array_free(b->bookmarks, TRUE);
    thunar_vfs_path_unref(b->trash_path);
    g_free(b);
    thunar_vfs_shutdown();
}

/*
 * A bookmark with the same path as a system path should have the system icon.
 * Such a bookmark with its default label should also use the system label.
 */
void
places_bookmarks_system_bi_system_mod(BookmarksSystem *b, BookmarkInfo *other){
    g_assert(b != NULL);
    g_assert(other != NULL);
    g_assert(other->icon != NULL);
    g_assert(other->label != NULL);

    gboolean label_is_default;
    gchar   *default_label;
    
    default_label    = g_filename_display_basename(other->uri);
    label_is_default = exo_str_is_equal(default_label, other->label);
    g_free(default_label);

    BookmarkInfo *bi;
    guint k;
    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);

        if(G_UNLIKELY(exo_str_is_equal(other->uri, bi->uri))){
            g_free(other->icon);
            other->icon = g_strdup(bi->icon);

            if(label_is_default && !exo_str_is_equal(other->label, bi->label)){
                g_free(other->label);
                other->label = g_strdup(bi->label);
            }
            
            return;
        }
    }
}

// vim: ai et tabstop=4
