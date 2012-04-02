/*  xfce4-places-plugin
 *
 *  This file defines the interfaces to the model.
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

#ifndef _XFCE_PANEL_PLACES_MODEL_H
#define _XFCE_PANEL_PLACES_MODEL_H

#include <glib.h>
#include <gio/gio.h>

/* Places Bookmark Action */
typedef struct _PlacesBookmarkAction PlacesBookmarkAction;
struct _PlacesBookmarkAction
{
    gchar       *label;     /* must not be NULL */
    gboolean    may_block;
    gpointer    priv;
    void        (*action)   (PlacesBookmarkAction *self);
    void        (*finalize) (PlacesBookmarkAction *self);
};

inline PlacesBookmarkAction*
places_bookmark_action_create(gchar *label);

inline void
places_bookmark_action_destroy(PlacesBookmarkAction*);

inline void
places_bookmark_action_call(PlacesBookmarkAction*);

/* Places Bookmark */

typedef enum
{
    PLACES_URI_SCHEME_NONE=0,
    PLACES_URI_SCHEME_FILE, 
    PLACES_URI_SCHEME_TRASH
} places_uri_scheme;

typedef struct _PlacesBookmark PlacesBookmark;
struct _PlacesBookmark
{
    gchar                 *label;          /* must not be NULL */
    gchar                 *uri;            /* may be NULL */
    places_uri_scheme      uri_scheme;    
    GIcon                 *icon;           /* may be NULL */
    PlacesBookmarkAction  *primary_action; /* may be NULL */
    gboolean               force_gray;
    GList                 *actions;        /* may be NULL (empty) */

    gpointer               priv;           /* private data */
    void                 (*finalize) (PlacesBookmark *self);
};

inline PlacesBookmark*
places_bookmark_create(gchar *label);

inline void
places_bookmark_destroy(PlacesBookmark *bookmark);

/* Places Bookmark Group */
typedef struct _PlacesBookmarkGroup PlacesBookmarkGroup;
struct _PlacesBookmarkGroup
{
    GList*      (*get_bookmarks) (PlacesBookmarkGroup *self);
    gboolean    (*changed)       (PlacesBookmarkGroup *self);
    void        (*finalize)      (PlacesBookmarkGroup *self);
    gpointer    priv;
};

inline GList*
places_bookmark_group_get_bookmarks(PlacesBookmarkGroup*);

inline gboolean
places_bookmark_group_changed(PlacesBookmarkGroup*);

inline PlacesBookmarkGroup*
places_bookmark_group_create();

inline void
places_bookmark_group_destroy(PlacesBookmarkGroup*);

#endif
/* vim: set ai et tabstop=4: */
