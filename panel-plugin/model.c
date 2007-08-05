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
#include "model_system.h"
#include "model_volumes.h"
#include "model_user.h"

#include <libxfce4util/libxfce4util.h>

struct _Bookmarks
{
    BookmarksSystem  *system;
    BookmarksVolumes *volumes;
    BookmarksUser    *user;
};

Bookmarks*
places_bookmarks_init()
{
    DBG("initializing model");

    Bookmarks *b = g_new0(Bookmarks, 1);
    
    b->system = places_bookmarks_system_init();
    return b;
}

void
places_bookmarks_enable(Bookmarks *b, gint enable_mask)
{
    g_assert(b != NULL);

    gboolean enable_volumes, enable_user;
    
    // Enable/disable volumes
    enable_volumes = (enable_mask & PLACES_BOOKMARKS_ENABLE_VOLUMES);

    if(b->volumes == NULL && enable_volumes){
        b->volumes = places_bookmarks_volumes_init();
    }else if(b->volumes != NULL && !enable_volumes) {
        places_bookmarks_volumes_finalize(b->volumes);
        b->volumes = NULL;
    }

    // Enable/disable GTK bookmarks
    enable_user = (enable_mask & PLACES_BOOKMARKS_ENABLE_USER);

    if(b->user == NULL && enable_user){
        b->user = places_bookmarks_user_init(b->system); // user depends on system for 
                                                         // places_bookmarks_system_bi_system_mod().
                                                         // It's a sucky aspect of this design...
    }else if(b->user != NULL && !enable_user){
        places_bookmarks_user_finalize(b->user);
        b->user = NULL;
    }

}

void
places_bookmarks_visit(Bookmarks *b,
                       BookmarksVisitor *visitor)
{
    g_assert(b != NULL);
    g_assert(visitor != NULL);

    places_bookmarks_system_visit  (b->system,  visitor);

    if(b->volumes != NULL)
        places_bookmarks_volumes_visit (b->volumes, visitor);

    if(b->user != NULL){
        visitor->separator             (visitor->pass_thru);
        places_bookmarks_user_visit    (b->user,    visitor);
    }
}

gboolean
places_bookmarks_changed(Bookmarks *b)
{
    g_assert(b != NULL);

    // try to avoid short-circuit of || since changed() has side-effects
    gboolean changed = FALSE;

    changed = places_bookmarks_system_changed(b->system)        || changed;

    if(b->volumes != NULL)
        changed = places_bookmarks_volumes_changed(b->volumes)  || changed;

    if(b->user != NULL)
        changed = places_bookmarks_user_changed(b->user)        || changed;

    return changed;
}

void
places_bookmarks_finalize(Bookmarks *b)
{
    g_assert(b != NULL);

    places_bookmarks_system_finalize(b->system);
    b->system = NULL;
    
    places_bookmarks_enable(b, PLACES_BOOKMARKS_ENABLE_NONE);

    g_free(b);
}

void
places_bookmark_info_free(BookmarkInfo *bi)
{
    g_assert(bi != NULL);

    if(bi->label != NULL){
        g_free(bi->label);
        bi->label = NULL;
    }
    if(bi->uri != NULL){
        g_free(bi->uri);
        bi->uri = NULL;
    }
    if(bi->icon != NULL){
        g_free(bi->icon);
        bi->icon = NULL;
    }
    if(bi->data != NULL){
        DBG("WARNING: data != NULL. Caller is in charge of freeing data");
        bi->data = NULL;
    }

    g_free(bi);
}


void places_bookmark_action_call(BookmarkAction *act)
{
    g_assert(act != NULL);

    if(act->action != NULL)
        act->action(act);
}

void places_bookmark_action_free(BookmarkAction *act)
{
    g_assert(act != NULL);

    if(act->free != NULL)
        act->free(act);
    else
        g_free(act);
}

void
places_bookmark_actions_list_destroy(GSList *actions)
{
    g_assert(actions != NULL);

    g_slist_foreach(actions, (GFunc) places_bookmark_action_free, NULL);
    g_slist_free(actions);
}

// vim: ai et tabstop=4
