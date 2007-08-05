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

#include "model.h"
#include "model_volumes.h"

#define EXO_API_SUBJECT_TO_CHANGE
#include <thunar-vfs/thunar-vfs.h>

#include <libxfce4util/libxfce4util.h>

#include <string.h>

struct _BookmarksVolumes
{
    GPtrArray *bookmarks;
    gboolean   changed;
    ThunarVfsVolumeManager *volume_manager;
};

typedef struct {
    BookmarksVolumes    *b;
    ThunarVfsVolume     *volume;
} BookmarksVolumes_Volume;

static gboolean 
places_bookmarks_volumes_show_volume(ThunarVfsVolume *volume);

static void
places_bookmarks_volumes_add(BookmarksVolumes *b, const GList *volumes);


/********** ThunarVFS Callbacks **********/

static void
places_bookmarks_volumes_cb_changed(ThunarVfsVolume *volume, 
                                    BookmarksVolumes *b)
{
    DBG("volume changed"); 
    // unfortunately there tends to be like 3 of these in a row

    BookmarkInfo *bi;
    GList *volumes;
    guint k;
    
    b->changed = TRUE;

    if(places_bookmarks_volumes_show_volume(volume)){

        // make sure it's in the array
        for(k = 0; k < b->bookmarks->len; k++){
            bi = g_ptr_array_index(b->bookmarks, k);
            if(THUNAR_VFS_VOLUME(bi->data) == volume)
                break;
        }

        if(k == b->bookmarks->len){ // it's not there
            DBG("adding volume to array");

            volumes = g_list_prepend(NULL, volume);
            places_bookmarks_volumes_add(b, volumes);
            g_list_free(volumes);
        }else{
            DBG("volume already in array");
        }

    }else{
        // make sure it's not in the array
        for(k = 0; k < b->bookmarks->len; k++){
            bi = g_ptr_array_index(b->bookmarks, k);
            if(THUNAR_VFS_VOLUME(bi->data) == volume){ // it is there
                DBG("dropping volume from array");
                
                bi = g_ptr_array_remove_index(b->bookmarks, k);
                g_object_unref(bi->data); // unref the volume
                bi->data = NULL;
                places_bookmark_info_free(bi);
            }
        }
    }
}

static void
places_bookmarks_volumes_cb_added(ThunarVfsVolumeManager *volume_manager,
                                  const GList *volumes, 
                                  BookmarksVolumes *b)
{
    DBG("volumes added");
    places_bookmarks_volumes_add(b, volumes);
    b->changed = TRUE;
}

static void
places_bookmarks_volumes_cb_removed(ThunarVfsVolumeManager *volume_manager, 
                                    const GList *volumes, 
                                    BookmarksVolumes *b)
{
    DBG("volumes removed");

    BookmarkInfo *bi;
    GList *vol_iter;
    guint k;

    // step through existing bookmarks
    for(k = 0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);

        // step through removals
        vol_iter = (GList*) volumes;
        while(vol_iter){
            if(bi->data == vol_iter->data){ // it is there
                
                // delete the bookmark
                bi = g_ptr_array_remove_index(b->bookmarks, k);
                DBG("Removing bookmark %s", bi->label);
                
                if(bi->data != NULL){ // unref the volume
                    g_object_unref(bi->data);
                    bi->data = NULL;
                }
                places_bookmark_info_free(bi);
                
                b->changed = TRUE;
                break;
            }

            vol_iter = vol_iter->next;
        }
    }
}

/********** Actions Callbacks **********/

static void
places_bookmarks_volumes_unmount(BookmarkAction *act)
{
    DBG("Unmount");
    BookmarksVolumes_Volume *priv = (BookmarksVolumes_Volume*) act->priv;
    ThunarVfsVolume *volume = priv->volume;

    if(thunar_vfs_volume_is_mounted(volume)){
        if(thunar_vfs_volume_is_ejectable(volume))
            thunar_vfs_volume_eject(volume, NULL, NULL);
        else
            thunar_vfs_volume_unmount(volume, NULL, NULL);
    }
}

static void
places_bookmarks_volumes_mount(BookmarkAction *act)
{
    DBG("Mount");
    BookmarksVolumes_Volume *priv = (BookmarksVolumes_Volume*) act->priv;
    ThunarVfsVolume *volume = priv->volume;
    BookmarksVolumes *b = priv->b;
    BookmarkInfo *bi;
    guint k;

    if(!thunar_vfs_volume_is_mounted(volume)){

        thunar_vfs_volume_mount(volume, NULL, NULL);
    
        /* it sometimes wouldn't get the mount point right otherwise */
        for(k = 0; k < b->bookmarks->len; k++){
            bi = g_ptr_array_index(b->bookmarks, k);
            if(volume == bi->data){

                if(bi->uri != NULL)
                    g_free(bi->uri);

                bi->uri = thunar_vfs_path_dup_uri(thunar_vfs_volume_get_mount_point(volume));

                b->changed = TRUE;

                break;
            }
        }
    }
}

/********** Internal **********/
static gboolean
places_bookmarks_volumes_show_volume(ThunarVfsVolume *volume){
    
    DBG("Volume: %s [mounted=%x removable=%x present=%x]", thunar_vfs_volume_get_name(volume), 
                                                           thunar_vfs_volume_is_mounted(volume),
                                                           thunar_vfs_volume_is_removable(volume), 
                                                           thunar_vfs_volume_is_present(volume));

    return thunar_vfs_volume_is_removable(volume) && 
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

/********** External **********/

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
    BookmarkInfo *bi;
    guint k;

    for(k = 0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_remove_index(b->bookmarks, k);
        if(bi->data != NULL){ // unref the volume
            g_object_unref(bi->data);
            bi->data = NULL;
        }
        places_bookmark_info_free(bi);
    }

    g_object_unref(b->volume_manager);
    b->volume_manager = NULL;
    thunar_vfs_shutdown();
    
    g_ptr_array_foreach(b->bookmarks, (GFunc) places_bookmark_info_free, NULL);
    g_ptr_array_free(b->bookmarks, TRUE);
    b->bookmarks = NULL;

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

static void
free_toggle_mount_action(BookmarkAction *act)
{
    g_assert(act != NULL);
    g_assert(act->priv != NULL);

    g_free(act->priv);
    g_free(act);
}

void
places_bookmarks_volumes_visit(BookmarksVolumes *b, BookmarksVisitor *visitor)
{
    guint k;
    BookmarkInfo *bi;
    GSList *actions;
    ThunarVfsVolume *volume;
    gchar *uri;
    BookmarkAction *toggle_mount;
    BookmarksVolumes_Volume *toggle_mount_priv;

    for(k=0; k < b->bookmarks->len; k++){
        bi = g_ptr_array_index(b->bookmarks, k);
        volume = THUNAR_VFS_VOLUME(bi->data);

        toggle_mount_priv = g_new0(BookmarksVolumes_Volume, 1);
        toggle_mount_priv->b = b;
        toggle_mount_priv->volume = volume;

        toggle_mount = g_new0(BookmarkAction, 1); /* visitor will free */
        toggle_mount->priv = toggle_mount_priv;
        toggle_mount->free = free_toggle_mount_action;
        
        actions = g_slist_prepend(NULL, toggle_mount);
    
        if(thunar_vfs_volume_is_mounted(volume)){

            if(thunar_vfs_volume_is_ejectable(volume))
                toggle_mount->label = _("Eject Volume");
            else
                toggle_mount->label = _("Unmount Volume");
            toggle_mount->action = places_bookmarks_volumes_unmount;

            uri = bi->uri;

        }else{

            toggle_mount->label = _("Mount Volume");
            toggle_mount->action = places_bookmarks_volumes_mount;

            uri = NULL;
        }

        visitor->item(visitor->pass_thru, bi->label, uri, bi->icon, actions);
    }
}


// vim: ai et tabstop=4
