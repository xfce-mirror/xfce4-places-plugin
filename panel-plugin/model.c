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
    b->volumes = places_bookmarks_volumes_init();
    b->user = places_bookmarks_user_init(b->system); // user depends on system for 
                                                     // places_bookmarks_system_bi_system_mod().
                                                     // It's a sucky aspect of this design...

    return b;
}

void
places_bookmarks_visit(Bookmarks *b,
                       gpointer pass_thru, 
                       BOOKMARK_ITEM_FUNC(item_func),
                       BOOKMARK_SEPARATOR_FUNC(separator_func))
{
    places_bookmarks_system_visit  (b->system,  pass_thru, item_func, separator_func);
    places_bookmarks_volumes_visit (b->volumes, pass_thru, item_func, separator_func);
    separator_func                 (pass_thru);
    places_bookmarks_user_visit    (b->user,    pass_thru, item_func, separator_func);
}

gboolean
places_bookmarks_changed(Bookmarks *b)
{
    // try to avoid short-circuit of || since changed() has side-effects
    gboolean changed = FALSE;
    changed = places_bookmarks_system_changed(b->system)    || changed;
    changed = places_bookmarks_volumes_changed(b->volumes)  || changed;
    changed = places_bookmarks_user_changed(b->user)        || changed;
    return changed || TRUE;
}

void
places_bookmarks_finalize(Bookmarks *b)
{
    places_bookmarks_system_finalize(b->system);
    places_bookmarks_volumes_finalize(b->volumes);
    places_bookmarks_user_finalize(b->user);
    g_free(b);
}

// vim: ai et tabstop=4
