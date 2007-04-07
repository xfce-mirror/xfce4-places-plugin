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
#include "model_volumes.h"
#include <thunar-vfs/thunar-vfs.h>
#include <libxfce4util/libxfce4util.h>

struct _BookmarksVolumes
{
    GPtrArray *bookmarks;
    gboolean   changed;
    ThunarVfsVolumeManager *volume_manager;
};

static gboolean 
places_bookmarks_volumes_show_volume(ThunarVfsVolume *volume);

static void
places_bookmarks_volumes_add(BookmarksVolumes *b, const GList *volumes);


/********** ThunarVFS Callbacks **********/

void
places_bookmarks_volumes_cb_changed(ThunarVfsVolume *volume, 
                                    BookmarksVolumes *b)
{
    DBG("volume changed"); 
    // unfortunately there tends to be like 3 of these in a row

    guint k;

    if(places_bookmarks_volumes_show_volume(volume)){

        // make sure it's in the array
        for(k = 0; k < b->bookmarks->len; k++){
            BookmarkInfo *bi = g_ptr_array_index(b->bookmarks, k);
            if(THUNAR_VFS_VOLUME(bi->data) == volume)
                break;
        }

        if(k == b->bookmarks->len){ // it's not there
            DBG("adding volume to array");

            places_bookmarks_volumes_add(b, g_list_prepend(NULL, volume));
            b->changed = TRUE;
        }else{
            DBG("volume already in array");
        }

    }else{
        // make sure it's not in the array
        for(k = 0; k < b->bookmarks->len; k++){
            BookmarkInfo *bi = g_ptr_array_index(b->bookmarks, k);
            if(THUNAR_VFS_VOLUME(bi->data) == volume){ // it is there
                DBG("dropping volume from array");
                
                bi = g_ptr_array_remove_index(b->bookmarks, k);
                g_object_unref(bi->data);
                g_free(bi);
                
                b->changed = TRUE;
            }
        }
    }
}

void
places_bookmarks_volumes_cb_added(ThunarVfsVolumeManager *volume_manager,
                                  const GList *volumes, 
                                  BookmarksVolumes *b)
{
    DBG("volumes added");
    places_bookmarks_volumes_add(b, volumes);
    b->changed = TRUE;
}

void
places_bookmarks_volumes_cb_removed(ThunarVfsVolumeManager *volume_manager, 
                                    const GList *volumes, 
                                    BookmarksVolumes *b)
{
    DBG("volumes removed");

    GList *vol_iter;
    guint k;

    // step through existing bookmarks
    for(k = 0; k < b->bookmarks->len; k++){
        BookmarkInfo *bi = g_ptr_array_index(b->bookmarks, k);

        // step through removals
        vol_iter = (GList*) volumes;
        while(vol_iter){
            if(bi->data == vol_iter->data){ // it is there
                
                // delete the bookmark
                bi = g_ptr_array_remove_index(b->bookmarks, k);
                DBG("Removing bookmark %s", bi->label);
                
                if(bi->data)
                    g_object_unref(bi->data);
                g_free(bi);
                
                b->changed = TRUE;
            }

            vol_iter = vol_iter->next;
        }
    }
}

// internal
static gboolean
places_bookmarks_volumes_show_volume(ThunarVfsVolume *volume){
    
    DBG("Volume: %s [mounted=%x removable=%x present=%x]", thunar_vfs_volume_get_name(volume), 
                                                           thunar_vfs_volume_is_mounted(volume),
                                                           thunar_vfs_volume_is_removable(volume), 
                                                           thunar_vfs_volume_is_present(volume));

    return thunar_vfs_volume_is_mounted(volume) && 
           thunar_vfs_volume_is_removable(volume) && 
           thunar_vfs_volume_is_present(volume);
}


static void
places_bookmarks_volumes_add(BookmarksVolumes *b, const GList *volumes)
{
    ThunarVfsVolume *volume;
    BookmarkInfo *bi;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();

    while(volumes){
        volume = THUNAR_VFS_VOLUME(volumes->data);
        
        g_signal_connect (volume, "changed",
                      G_CALLBACK(places_bookmarks_volumes_cb_changed), b);

        if(places_bookmarks_volumes_show_volume(volume)){

            g_object_ref(volume);

            bi          = g_new0(BookmarkInfo, 1);
            bi->label   = g_strdup( thunar_vfs_volume_get_name(volume) );
            bi->uri     = thunar_vfs_path_dup_uri( thunar_vfs_volume_get_mount_point(volume) );
            bi->icon    = g_strdup( thunar_vfs_volume_lookup_icon_name(volume, icon_theme) );
            bi->data    = (gpointer) volume;

            g_ptr_array_add(b->bookmarks, bi);
        }

        volumes = volumes->next;
    }
}

// external

BookmarksVolumes*
places_bookmarks_volumes_init()
{
    DBG("init");
    BookmarksVolumes *b = g_new0(BookmarksVolumes, 1);

    thunar_vfs_init();
    
    b->bookmarks = g_ptr_array_new();
    b->changed = FALSE;
    b->volume_manager = thunar_vfs_volume_manager_get_default();
    
    places_bookmarks_volumes_add(b, thunar_vfs_volume_manager_get_volumes(b->volume_manager));

    g_signal_connect (b->volume_manager, "volumes-added",
                      G_CALLBACK (places_bookmarks_volumes_cb_added), b);

    g_signal_connect (b->volume_manager, "volumes-removed",
                      G_CALLBACK (places_bookmarks_volumes_cb_removed), b);

    DBG("done");

    return b;
}

void
places_bookmarks_volumes_finalize(BookmarksVolumes *b)
{
    guint k;
    
    thunar_vfs_shutdown();
    g_object_unref(b->volume_manager);
    
    for(k = 0; k < b->bookmarks->len; k++){
        BookmarkInfo *bi = g_ptr_array_remove_index(b->bookmarks, k);
        if(bi->data)
            g_object_unref(bi->data);
    }

    g_ptr_array_free(b->bookmarks, TRUE);
    g_free(b);
}

gboolean
places_bookmarks_volumes_changed(BookmarksVolumes *b)
{
    if(b->changed){
        b->changed = FALSE;
        return TRUE;
    }else{
        return FALSE;
    }
}

void
places_bookmarks_volumes_visit(BookmarksVolumes *b,
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


// vim: ai et tabstop=4
