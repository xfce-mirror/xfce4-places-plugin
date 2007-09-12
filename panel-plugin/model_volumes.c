/*  xfce4-places-plugin
 *
 *  Model: volumes bookmarks (i.e., removable media)
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

#define pbg_priv(pbg) ((PBVolData*) pbg->priv)

typedef struct
{

    ThunarVfsVolumeManager *volume_manager;
    gboolean   changed;

} PBVolData;


/********** Actions Callbacks **********/

static void
pbvol_eject(PlacesBookmarkAction *action)
{
    DBG("Eject");

    ThunarVfsVolume *volume = THUNAR_VFS_VOLUME(action->priv);

    thunar_vfs_volume_eject(volume, NULL, NULL);
}

static void
pbvol_unmount(PlacesBookmarkAction *action)
{
    DBG("Unmount");

    ThunarVfsVolume *volume = THUNAR_VFS_VOLUME(action->priv);

    if(thunar_vfs_volume_is_mounted(volume))
        thunar_vfs_volume_unmount(volume, NULL, NULL);
}

static void
pbvol_mount(PlacesBookmarkAction *action)
{
    DBG("Mount");

    ThunarVfsVolume *volume = THUNAR_VFS_VOLUME(action->priv);

    if(!thunar_vfs_volume_is_mounted(volume))
        thunar_vfs_volume_mount(volume, NULL, NULL);
}

static inline gboolean
pbvol_show_volume(ThunarVfsVolume *volume){
    
    DBG("Volume: %s [mounted=%x removable=%x present=%x]", thunar_vfs_volume_get_name(volume), 
                                                           thunar_vfs_volume_is_mounted(volume),
                                                           thunar_vfs_volume_is_removable(volume), 
                                                           thunar_vfs_volume_is_present(volume));

    return thunar_vfs_volume_is_removable(volume) && 
           thunar_vfs_volume_is_present(volume);
}

static void
pbvol_set_changed(PlacesBookmarkGroup *bookmark_group)
{
    pbg_priv(bookmark_group)->changed = TRUE;
}


static void
pbvol_volumes_added(ThunarVfsVolumeManager *volman, GList *volumes, PlacesBookmarkGroup *bookmark_group)
{
    pbg_priv(bookmark_group)->changed = TRUE;
    while(volumes != NULL){
        g_signal_connect_swapped(THUNAR_VFS_VOLUME(volumes->data), "changed",
                                 G_CALLBACK(pbvol_set_changed), bookmark_group);
        volumes = volumes->next;
    }
}

static void
pbvol_volumes_removed(ThunarVfsVolumeManager *volman, GList *volumes, PlacesBookmarkGroup *bookmark_group)
{
    pbg_priv(bookmark_group)->changed = TRUE;
    while(volumes != NULL){
        g_signal_handlers_disconnect_by_func(THUNAR_VFS_VOLUME(volumes->data),
                                             G_CALLBACK(pbvol_set_changed), bookmark_group);
        volumes = volumes->next;
    }
}

static void
pbvol_bookmark_free(PlacesBookmark *bookmark)
{
    if(bookmark->uri != NULL)
        g_free(bookmark->uri);
    g_free(bookmark);
}

static void
pbvol_bookmark_action_free(PlacesBookmarkAction *action){
    g_assert(action != NULL && action->priv != NULL);

    ThunarVfsVolume *volume = THUNAR_VFS_VOLUME(action->priv);
    g_object_unref(volume);
    action->priv = NULL;

    g_free(action);
}

static GList*
pbvol_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = NULL;
    PlacesBookmark *bookmark;
    PlacesBookmarkAction *action;
    const GList *volumes;
    ThunarVfsVolume *volume;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();

    volumes = thunar_vfs_volume_manager_get_volumes(pbg_priv(bookmark_group)->volume_manager);
    while(volumes != NULL){
        volume = THUNAR_VFS_VOLUME(volumes->data);

        if(pbvol_show_volume(volume)){

            bookmark        = g_new0(PlacesBookmark, 1);
            bookmark->label = (gchar*) thunar_vfs_volume_get_name(volume);
            if(thunar_vfs_volume_is_mounted(volume))
                bookmark->uri   = thunar_vfs_path_dup_uri(thunar_vfs_volume_get_mount_point(volume));
            else
                bookmark->uri   = NULL;
            bookmark->icon  = (gchar*) thunar_vfs_volume_lookup_icon_name(volume, icon_theme);
            bookmark->free  = pbvol_bookmark_free;

            if(!thunar_vfs_volume_is_mounted(volume)){
                g_object_ref(volume);
                action          = g_new0(PlacesBookmarkAction, 1);
                action->label   = _("Mount");
                action->priv    = volume;
                action->action  = pbvol_mount;
                action->free    = pbvol_bookmark_action_free;
                bookmark->actions = g_list_append(bookmark->actions, action);
            }

            if(thunar_vfs_volume_is_disc(volume)){
                if(thunar_vfs_volume_is_ejectable(volume)){
                    g_object_ref(volume);
                    action          = g_new0(PlacesBookmarkAction, 1);
                    action->label   = _("Eject");
                    action->priv    = volume;
                    action->action  = pbvol_eject;
                    action->free    = pbvol_bookmark_action_free;
                    bookmark->actions = g_list_append(bookmark->actions, action);
                }
            }else{
                if(thunar_vfs_volume_is_mounted(volume)){
                    g_object_ref(volume);
                    action          = g_new0(PlacesBookmarkAction, 1);
                    action->label   = _("Unmount");
                    action->priv    = volume;
                    action->action  = pbvol_unmount;
                    action->free    = pbvol_bookmark_action_free;
                    bookmark->actions = g_list_append(bookmark->actions, action);
                }
            }

            bookmarks = g_list_prepend(bookmarks, bookmark);
        }

        volumes = volumes->next;
    }
    
    pbg_priv(bookmark_group)->changed = FALSE;

    return g_list_reverse(bookmarks);

}

static gboolean
pbvol_changed(PlacesBookmarkGroup *bookmark_group)
{
    return pbg_priv(bookmark_group)->changed;
}

static void
pbvol_finalize(PlacesBookmarkGroup *bookmark_group)
{
    const GList *volumes;
    
    volumes = thunar_vfs_volume_manager_get_volumes(pbg_priv(bookmark_group)->volume_manager);
    while(volumes != NULL){
        g_signal_handlers_disconnect_by_func(THUNAR_VFS_VOLUME(volumes->data),
                                             G_CALLBACK(pbvol_set_changed), bookmark_group);
        volumes = volumes->next;
    }

    g_signal_handlers_disconnect_by_func(pbg_priv(bookmark_group)->volume_manager,
                                         G_CALLBACK(pbvol_volumes_added), bookmark_group);
    g_signal_handlers_disconnect_by_func(pbg_priv(bookmark_group)->volume_manager,
                                         G_CALLBACK(pbvol_volumes_removed), bookmark_group);

    g_object_unref(pbg_priv(bookmark_group)->volume_manager);
    pbg_priv(bookmark_group)->volume_manager = NULL;
    thunar_vfs_shutdown();
    
    g_free(bookmark_group->priv);
    g_free(bookmark_group);
}

PlacesBookmarkGroup*
places_bookmarks_volumes_create()
{
    const GList *volumes;
    PlacesBookmarkGroup *bookmark_group;

    bookmark_group                      = g_new0(PlacesBookmarkGroup, 1);
    bookmark_group->get_bookmarks       = pbvol_get_bookmarks;
    bookmark_group->changed             = pbvol_changed;
    bookmark_group->finalize            = pbvol_finalize;
    bookmark_group->priv                = g_new0(PBVolData, 1);
    
    thunar_vfs_init();
    pbg_priv(bookmark_group)->volume_manager = thunar_vfs_volume_manager_get_default();
    pbg_priv(bookmark_group)->changed        = TRUE;
    
    volumes = thunar_vfs_volume_manager_get_volumes(pbg_priv(bookmark_group)->volume_manager);
    while(volumes != NULL){
        g_signal_connect_swapped(THUNAR_VFS_VOLUME(volumes->data), "changed",
                                 G_CALLBACK(pbvol_set_changed), bookmark_group);
        volumes = volumes->next;
    }

    g_signal_connect(pbg_priv(bookmark_group)->volume_manager, "volumes-added",
                     G_CALLBACK(pbvol_volumes_added), bookmark_group);

    g_signal_connect(pbg_priv(bookmark_group)->volume_manager, "volumes-removed",
                     G_CALLBACK(pbvol_volumes_removed), bookmark_group);

    return bookmark_group;
}

/* vim: set ai et tabstop=4: */
