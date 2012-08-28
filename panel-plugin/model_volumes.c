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
#include "support.h"

#include <gio/gio.h>
#ifdef HAVE_GIO_UNIX
#include <gio/gunixmounts.h>
#endif
#include <gtk/gtk.h>
 
#ifdef HAVE_LIBNOTIFY
#include "model_volumes_notify.h"
#endif

#include <libxfce4util/libxfce4util.h>

#include <string.h>

#define pbg_priv(pbg) ((PBVolData*) pbg->priv)

typedef struct
{
    GVolumeMonitor *volume_monitor;
    gboolean   changed;
    gboolean   mount_and_open_by_default;
} PBVolData;


/********** Actions Callbacks **********/
static void
pbvol_eject_finish(GObject *object,
                GAsyncResult *result,
                gpointer user_data)
{
    GVolume *volume = G_VOLUME(object);
    GError *error = NULL;

    g_return_if_fail(G_IS_VOLUME(object));
    g_return_if_fail(G_IS_ASYNC_RESULT(result));

    if (!g_volume_eject_with_operation_finish(volume, result, &error)) {
         /* ignore GIO errors handled internally */
         if(error->domain != G_IO_ERROR || error->code != G_IO_ERROR_FAILED_HANDLED) {
             gchar *volume_name = g_volume_get_name(volume);
             places_show_error_dialog(error,
                                 _("Failed to eject \"%s\""),
                                 volume_name);
             g_free(volume_name);
         }
         g_error_free(error);
    }

#ifdef HAVE_LIBNOTIFY
    pbvol_notify_eject_finish(volume);
#endif
}

static void
pbvol_eject(PlacesBookmarkAction *action)
{
    GVolume *volume;

    DBG("Eject");

    g_return_if_fail(G_IS_VOLUME(action->priv));
    volume = G_VOLUME(action->priv);

    if (g_volume_can_eject(volume)) {
#ifdef HAVE_LIBNOTIFY
        pbvol_notify_eject(volume);
#endif
        g_volume_eject_with_operation(volume, G_MOUNT_UNMOUNT_NONE, NULL,
                           NULL,
                           pbvol_eject_finish,
                           g_object_ref(volume));
    }
}

static void
pbvol_unmount_finish(GObject *object,
                GAsyncResult *result,
                gpointer user_data)
{
    GMount *mount = G_MOUNT(object);
    GError *error = NULL;

    g_return_if_fail(G_IS_MOUNT(object));
    g_return_if_fail(G_IS_ASYNC_RESULT(result));

    if (!g_mount_unmount_with_operation_finish(mount, result, &error)) {
         /* ignore GIO errors handled internally */
         if (error->domain != G_IO_ERROR || error->code != G_IO_ERROR_FAILED_HANDLED) {
             gchar *mount_name = g_mount_get_name(mount);
             places_show_error_dialog(error,
                                     _("Failed to unmount \"%s\""),
                                     mount_name);
             g_free(mount_name);
         }
         g_error_free (error);
    }

#ifdef HAVE_LIBNOTIFY
    pbvol_notify_unmount_finish(mount);
#endif
}

static void
pbvol_unmount(PlacesBookmarkAction *action)
{
    GVolume *volume;
    GMount *mount;

    DBG("Unmount");

    g_return_if_fail(G_IS_VOLUME(action->priv));
    volume = G_VOLUME(action->priv);
    mount = g_volume_get_mount(volume);

    if (mount) {
#ifdef HAVE_LIBNOTIFY
        pbvol_notify_unmount(mount);
#endif
        g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, NULL,
                            NULL,
                            pbvol_unmount_finish, 
                            g_object_ref(volume));
    }
}

static void
pbvol_mount_finish(GObject *object,
                GAsyncResult *result,
                gpointer user_data)
{
    GVolume *volume = G_VOLUME(object);
    GError *error = NULL;

    DBG("Mount finish");

    if (!g_volume_mount_finish(volume, result, &error)) {
         /* ignore GIO errors handled internally */
         if (error->domain != G_IO_ERROR || error->code != G_IO_ERROR_FAILED_HANDLED) {
             gchar *volume_name = g_volume_get_name(volume);
             places_show_error_dialog(error,
                                     _("Failed to mount \"%s\""),
                                     volume_name);
             g_free(volume_name);
         }
         g_error_free (error);
    }
}

static void
pbvol_mount_finish_and_open(GObject *object,
                            GAsyncResult *result,
                            gpointer user_data)
{
    GVolume *volume = G_VOLUME(object);
    GError *error = NULL;

    DBG("Mount finish and open");

    if (!g_volume_mount_finish(volume, result, &error)) {
         /* ignore GIO errors handled internally */
         if (error->domain != G_IO_ERROR || error->code != G_IO_ERROR_FAILED_HANDLED) {
             gchar *volume_name = g_volume_get_name(volume);
             places_show_error_dialog(error,
                                     _("Failed to mount \"%s\""),
                                     volume_name);
             g_free(volume_name);
         }
         g_error_free (error);
    } else {
        GMount *mount;
        gchar *uri;
        mount = g_volume_get_mount(volume);

        if (mount) {
            GFile *file = g_mount_get_root(mount);
            uri = g_file_get_uri(file);
            places_load_file_browser(uri);
            g_free(uri);
            g_object_unref(file);
            g_object_unref(mount);
        }
    }
}

static void
pbvol_mount(PlacesBookmarkAction *action)
{
    GVolume *volume;
    GMount *mount;

    DBG("Mount");

    g_return_if_fail(G_IS_VOLUME(action->priv));
    volume = G_VOLUME(action->priv);
    mount = g_volume_get_mount(volume);

    if (!mount) {
        GMountOperation *operation = gtk_mount_operation_new(NULL);

        g_volume_mount(volume, G_MOUNT_MOUNT_NONE, operation, NULL,
                       pbvol_mount_finish, 
                       g_object_ref(volume));

        g_object_unref(operation);
    }
}

static void
pbvol_mount_and_open(PlacesBookmarkAction *action)
{
    GVolume *volume;
    GMount *mount;

    DBG("Mount and open");
    
    g_return_if_fail(G_IS_VOLUME(action->priv));
    volume = G_VOLUME(action->priv);
    mount = g_volume_get_mount(volume);

    if (!mount) {
        GMountOperation *operation = gtk_mount_operation_new(NULL);

        g_volume_mount(volume, G_MOUNT_MOUNT_NONE, operation, NULL,
                       pbvol_mount_finish_and_open,
                       g_object_ref(volume));

        g_object_unref(operation);
    }
}

#ifdef HAVE_GIO_UNIX
static gboolean
pbvol_mount_is_internal (GMount *mount)
{
    const gchar *point_mount_path;
    gboolean is_internal = FALSE;
    GFile *root;
    GList *lp;
    GList *mount_points;
    gchar *mount_path;

    g_return_val_if_fail(G_IS_MOUNT(mount), FALSE);

    /* determine the mount path */
    root = g_mount_get_root(mount);
    mount_path = g_file_get_path(root);
    g_object_unref(root);

    /* assume non-internal if we cannot determine the path */
    if (!mount_path)
        return FALSE;

    if (g_unix_is_mount_path_system_internal(mount_path)) {
        /* mark as internal */
        is_internal = TRUE;
    } else {
        /* get a list of all mount points */
        mount_points = g_unix_mount_points_get(NULL);

        /* search for the mount point associated with the mount entry */
        for (lp = mount_points; !is_internal && lp != NULL; lp = lp->next) {
            point_mount_path = g_unix_mount_point_get_mount_path(lp->data);

            /* check if this is the mount point we are looking for */
            if (g_strcmp0(mount_path, point_mount_path) == 0) {
                /* mark as internal if the user cannot mount this device */
                if (!g_unix_mount_point_is_user_mountable(lp->data))
                    is_internal = TRUE;
            }
                
            /* free the mount point, we no longer need it */
            g_unix_mount_point_free(lp->data);
        }

        /* free the mount point list */
        g_list_free(mount_points);
    }

    g_free(mount_path);


    return is_internal;
}
#endif


static gboolean
pbvol_is_removable(GVolume *volume)
{
    gboolean can_eject = FALSE;
    gboolean can_mount = FALSE;
    gboolean can_unmount = FALSE;
    gboolean is_removable = FALSE;
    gboolean is_internal = FALSE;
    GDrive *drive;
    GMount *mount;

    g_return_val_if_fail(G_IS_VOLUME(volume), FALSE);

    /* check if the volume can be ejected */
    can_eject = g_volume_can_eject(volume);

    /* determine the drive for the volume */
    drive = g_volume_get_drive(volume);
    if (drive) {
        /*check if the drive media can be removed */
        is_removable = g_drive_is_media_removable(drive);

        /* release the drive */
        g_object_unref(drive);
    }
    /* determine the mount for the volume (if it is mounted at all) */
    mount = g_volume_get_mount(volume);
    if (mount) {
#ifdef HAVE_GIO_UNIX
        is_internal = pbvol_mount_is_internal (mount);
#endif

        /* check if the volume can be unmounted */
        can_unmount = g_mount_can_unmount(mount);

        /* release the mount */
        g_object_unref(mount);
    }

    /* determine whether the device can be mounted */
    can_mount = g_volume_can_mount(volume);

    return (!is_internal) && (can_eject || can_unmount || is_removable || can_mount);
}

static gboolean
pbvol_is_present(GVolume *volume)
{
    gboolean has_media = FALSE;
    gboolean is_shadowed = FALSE;
    GDrive *drive;
    GMount *mount;

    g_return_val_if_fail(G_IS_VOLUME(volume), FALSE);

    drive = g_volume_get_drive (volume);
    if(drive) {
        has_media = g_drive_has_media(drive);
        g_object_unref(drive);
    }

    mount = g_volume_get_mount(volume);
    if(mount) {
        is_shadowed = g_mount_is_shadowed(mount);
        g_object_unref(mount);
    }

    return has_media && !is_shadowed;
}

static inline gboolean
pbvol_show_volume(GVolume *volume){
    GMount *mount = g_volume_get_mount(volume);
    DBG("Volume: %s [mounted=%x removable=%x present=%x]", g_volume_get_name(volume), 
                                                           (guint) mount,
                                                           pbvol_is_removable(volume), 
                                                           pbvol_is_present(volume));
    if (mount)
       g_object_unref(mount);

    return pbvol_is_removable(volume) && 
           pbvol_is_present(volume);
}

static void
pbvol_set_changed(PlacesBookmarkGroup *bookmark_group)
{
    DBG("-");
    pbg_priv(bookmark_group)->changed = TRUE;
}


static void
pbvol_volume_added(GVolumeMonitor *monitor, GVolume *volume, PlacesBookmarkGroup *bookmark_group)
{
    DBG("-");

    pbg_priv(bookmark_group)->changed = TRUE;
    g_signal_connect_swapped(G_VOLUME(volume), "changed",
                             G_CALLBACK(pbvol_set_changed), bookmark_group);
}

static void
pbvol_volume_removed(GVolumeMonitor *monitor, GVolume *volume, PlacesBookmarkGroup *bookmark_group)
{
    DBG("-");

    pbg_priv(bookmark_group)->changed = TRUE;
    g_signal_handlers_disconnect_by_func(G_VOLUME(volume),
                                         G_CALLBACK(pbvol_set_changed), bookmark_group);
}

static void
pbvol_bookmark_finalize(PlacesBookmark *bookmark)
{
    if(bookmark->uri != NULL){
        g_free(bookmark->uri);
        bookmark->uri = NULL;
    }
}

static void
pbvol_bookmark_action_finalize(PlacesBookmarkAction *action){
    GVolume *volume;

    g_assert(action != NULL && action->priv != NULL);

    volume = G_VOLUME(action->priv);
    g_object_unref(volume);
    action->priv = NULL;
}

static GList*
pbvol_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = NULL;
    PlacesBookmark *bookmark;
    PlacesBookmarkAction *action, *terminal, *open;
    const GList *volumes;
    GVolume *volume;
    GMount *mount;

    volumes = g_volume_monitor_get_volumes(pbg_priv(bookmark_group)->volume_monitor);
    while (volumes != NULL) {
        volume = volumes->data;
        mount = g_volume_get_mount(volume);

        if(pbvol_show_volume(volume)){

            bookmark            = places_bookmark_create((gchar*) g_volume_get_name(volume));
            if (mount) {
                GFile *file = g_mount_get_root(mount);
                bookmark->uri   = g_file_get_uri(file);
                g_object_unref(file);
            } else
                bookmark->uri   = NULL;
            bookmark->icon      = g_volume_get_icon(volume);
            bookmark->finalize  = pbvol_bookmark_finalize;

            if (!mount) {

                g_object_ref(volume);
                action              = places_bookmark_action_create(_("Mount and Open"));
                action->may_block   = TRUE;
                action->priv        = volume;
                action->action      = pbvol_mount_and_open;
                action->finalize    = pbvol_bookmark_action_finalize;
                bookmark->actions   = g_list_append(bookmark->actions, action);

                if(pbg_priv(bookmark_group)->mount_and_open_by_default){
                    bookmark->primary_action = action;
                    bookmark->force_gray = TRUE;
                }

                g_object_ref(volume);
                action              = places_bookmark_action_create(_("Mount"));
                action->may_block   = TRUE;
                action->priv        = volume;
                action->action      = pbvol_mount;
                action->finalize    = pbvol_bookmark_action_finalize;
                bookmark->actions   = g_list_append(bookmark->actions, action);

            }else{

                terminal                 = places_create_open_terminal_action(bookmark);
                bookmark->actions        = g_list_prepend(bookmark->actions, terminal);
                open                     = places_create_open_action(bookmark);
                bookmark->actions        = g_list_prepend(bookmark->actions, open);
                bookmark->primary_action = open;

            }

            if (g_volume_can_eject(volume)) {

                g_object_ref(volume);
                action              = places_bookmark_action_create(_("Eject"));
                action->may_block   = TRUE;
                action->priv        = volume;
                action->action      = pbvol_eject;
                action->finalize    = pbvol_bookmark_action_finalize;
                bookmark->actions   = g_list_append(bookmark->actions, action);

            }
            if (mount) {
                g_object_ref(volume);
                action              = places_bookmark_action_create(_("Unmount"));
                action->may_block   = TRUE;
                action->priv        = volume;
                action->action      = pbvol_unmount;
                action->finalize    = pbvol_bookmark_action_finalize;
                bookmark->actions   = g_list_append(bookmark->actions, action);
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
    
    volumes = g_volume_monitor_get_volumes(pbg_priv(bookmark_group)->volume_monitor);
    while(volumes != NULL){
        g_signal_handlers_disconnect_by_func(G_VOLUME(volumes->data),
                                             G_CALLBACK(pbvol_set_changed), bookmark_group);
        volumes = volumes->next;
    }

    g_signal_handlers_disconnect_by_func(pbg_priv(bookmark_group)->volume_monitor,
                                         G_CALLBACK(pbvol_volume_added), bookmark_group);
    g_signal_handlers_disconnect_by_func(pbg_priv(bookmark_group)->volume_monitor,
                                         G_CALLBACK(pbvol_volume_removed), bookmark_group);

    g_object_unref(pbg_priv(bookmark_group)->volume_monitor);
    pbg_priv(bookmark_group)->volume_monitor = NULL;
    
    g_free(pbg_priv(bookmark_group));
    bookmark_group->priv = NULL;
}

PlacesBookmarkGroup*
places_bookmarks_volumes_create(gboolean mount_and_open_by_default)
{
    GList *volumes;
    PlacesBookmarkGroup *bookmark_group;

    bookmark_group                      = places_bookmark_group_create();
    bookmark_group->get_bookmarks       = pbvol_get_bookmarks;
    bookmark_group->changed             = pbvol_changed;
    bookmark_group->finalize            = pbvol_finalize;
    bookmark_group->priv                = g_new0(PBVolData, 1);

    pbg_priv(bookmark_group)->volume_monitor = g_volume_monitor_get();
    pbg_priv(bookmark_group)->changed        = TRUE;
    pbg_priv(bookmark_group)->mount_and_open_by_default = mount_and_open_by_default;
    
    volumes = g_volume_monitor_get_volumes(pbg_priv(bookmark_group)->volume_monitor);
    while(volumes != NULL) {
        g_signal_connect_swapped(G_OBJECT(volumes->data), "changed",
                                 G_CALLBACK(pbvol_set_changed), bookmark_group);
        g_object_unref(volumes->data);
        volumes = volumes->next;
    }
    g_list_free(volumes);

    g_signal_connect(pbg_priv(bookmark_group)->volume_monitor, "volume-added",
                     G_CALLBACK(pbvol_volume_added), bookmark_group);

    g_signal_connect(pbg_priv(bookmark_group)->volume_monitor, "volume-removed",
                     G_CALLBACK(pbvol_volume_removed), bookmark_group);

    return bookmark_group;
}

/* vim: set ai et tabstop=4: */
