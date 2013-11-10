/*  xfce4-places-plugin
 *
 *  Model: user bookmarks (defined in ~/.gtk-bookmarks)
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

/* User bookmarks come from the file ~/.gtk-bookmarks.
 *
 * When changed() is first called, it returns TRUE.
 *
 * When get_bookmarks() is first called, they will be built using
 * pbuser_build_bookmarks(). It stores the bookmarks in PBUserData.bookmarks
 * and the file's mtime in PBUserData.loaded. get_bookmarks() then clones the
 * bookmarks from PBUserData.bookmarks.
 *
 * Once that's done, a call to changed() checks the file's mtime and a couple
 * other things.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "model_user.h"
#include "model.h"
#include "support.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <libxfce4util/libxfce4util.h>
#include <glib.h>
#include <glib/gstdio.h>

#define pbg_priv(pbg) ((PBUserData*) pbg->priv)
#define show_bookmark(b) (GPOINTER_TO_INT(b->priv))

typedef struct
{
  GList     *bookmarks;
  gchar     *filename;
  time_t     loaded; /* 0  indicates loading the file hasn't been attempted
                        1  indicates the file does not exist
                        2+ are actual timestamps */

} PBUserData;

static inline time_t
pbuser_get_mtime(const gchar *filename)
{
    struct stat buf;
    if(g_stat(filename, &buf) == 0)
        return MAX(buf.st_mtime, 2);
    else
        return 1;
}

static inline gboolean
pbuser_dir_exists(const gchar *path)
{
    return g_file_test(path, G_FILE_TEST_IS_DIR);
}

static void
pbuser_finalize_bookmark(PlacesBookmark *bookmark)
{
    g_assert(bookmark != NULL);

    if(bookmark->uri != NULL){
        g_free(bookmark->uri);
        bookmark->uri = NULL;
    }
    if(bookmark->label != NULL){
        g_free(bookmark->label);
        bookmark->label = NULL;
    }
}

static void
pbuser_destroy_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    GList *bookmarks = pbg_priv(bookmark_group)->bookmarks;

    if(bookmarks == NULL)
        return;
    
    DBG("destroy internal bookmarks");

    while(bookmarks != NULL){
        places_bookmark_destroy((PlacesBookmark*) bookmarks->data);
        bookmarks = bookmarks->next;
    }
    g_list_free(bookmarks);
    pbg_priv(bookmark_group)->bookmarks = NULL;

    pbg_priv(bookmark_group)->loaded = 0;
}

static void
pbuser_build_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    /* As of 2013-11-09, this is pretty much taken from/analogous to Thunar */

    GList  *bookmarks = NULL;
    PlacesBookmark *bookmark;
    places_uri_scheme p_uri;
    gchar  *name;
    gchar  *space;
    gchar  *uri;
    gchar   line[2048];
    FILE   *fp;
    GFile  *file;
    GFileInfo *fileinfo;
    GIcon  *icon;
 
    pbuser_destroy_bookmarks(bookmark_group);

    fp = fopen(pbg_priv(bookmark_group)->filename, "r");

    if(G_UNLIKELY(fp == NULL)){
        DBG("Error opening gtk bookmarks file");
        pbg_priv(bookmark_group)->loaded = 1;
        return;
    }

    while( fgets(line, sizeof(line), fp) != NULL )
    {
        /* remove trailing spaces */
        g_strchomp (line);

        /* skip over empty lines */
        if (*line == '\0' || *line == ' ')
            continue;

        /* check if there is a custom name in the line */
        name = NULL;
        space = strchr (line, ' ');
        if (space != NULL){
            /* break line */
            *space++ = '\0';

            /* get the custom name */
            if (G_LIKELY (*space != '\0'))
                name = g_strdup(space);
        }

        file = g_file_new_for_uri (line);

        if(g_file_is_native(file)){

            uri = g_filename_from_uri(line, NULL, NULL);

            fileinfo = g_file_query_info(file,
                                  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
                                  G_FILE_ATTRIBUTE_STANDARD_ICON,
                                  0, NULL, NULL);

            icon = g_file_info_get_icon(fileinfo);
            if(icon == NULL)
                icon = g_themed_icon_new ("folder");
            g_object_ref(icon);
            p_uri = PLACES_URI_SCHEME_FILE;

            if(name == NULL) {
                name = g_strdup(g_file_info_get_attribute_string(fileinfo,
                                  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME));
                if(name == NULL)
                    name = g_strdup(g_filename_display_basename(uri));
            }

            g_object_unref (G_OBJECT (fileinfo));

        }else{

            uri = g_strdup(line);
            icon = g_themed_icon_new ("folder-remote");
            g_object_ref(icon);
            p_uri = PLACES_URI_SCHEME_REMOTE;

            if(name == NULL) {
                /* I tried to make this into a separate function
                   but it crashes evrytim... */
                gchar       *scheme;
                gchar       *parse_name;
                const gchar *p;
                const gchar *path;
                gchar       *hostname;
                gchar       *display_name = NULL;
                const gchar *skip;
                const gchar *firstdot;
                const gchar  skip_chars[] = ":@";
                guint        n;

                scheme = g_file_get_uri_scheme (file);
                parse_name = g_file_get_parse_name (file);

                if (g_str_has_prefix (parse_name, scheme)) {
                    /* extract the hostname */
                    p = parse_name + strlen (scheme);
                    while (*p == ':' || *p == '/')
                        ++p;

                    /* goto path part */
                    path = strchr (p, '/');
                    firstdot = strchr (p, '.');

                    if (firstdot != NULL){
                        /* skip password or login names in the hostname */
                        for (n = 0; n < G_N_ELEMENTS (skip_chars) - 1; n++){
                            skip = strchr (p, skip_chars[n]);
                            if (skip != NULL
                                   && (path == NULL || skip < path)
                                   && (skip < firstdot))
                                        p = skip + 1;
                        }
                    }

                    /* extract the path and hostname from the string */
                    if (G_LIKELY (path != NULL)){
                        hostname = g_strndup (p, path - p);
                    }else{
                        hostname = g_strdup (p);
                        path = "/";
                    }

                    /* TRANSLATORS: this will result in "<path> on <hostname>" */
                    name = g_strdup_printf (_("%s on %s"), path, hostname);

                    g_free (hostname);
                }

                g_free (scheme);
                g_free (parse_name);

            }
        }

        /* create the BookmarkInfo container */
        bookmark        = places_bookmark_create(name);
        bookmark->uri   = uri;
        bookmark->uri_scheme   = p_uri;
        bookmark->icon  = icon;
        bookmark->priv  = GINT_TO_POINTER((p_uri==PLACES_URI_SCHEME_REMOTE)||pbuser_dir_exists(bookmark->uri));
        bookmark->finalize = pbuser_finalize_bookmark;

        bookmarks = g_list_prepend(bookmarks, bookmark);

        g_object_unref (G_OBJECT (file));
    }

    fclose(fp);

    pbg_priv(bookmark_group)->bookmarks = g_list_reverse(bookmarks);
    pbg_priv(bookmark_group)->loaded    = pbuser_get_mtime(pbg_priv(bookmark_group)->filename);
}


static GList*
pbuser_get_bookmarks(PlacesBookmarkGroup *bookmark_group)
{
    const GList *orig_ls      = pbg_priv(bookmark_group)->bookmarks;
    const PlacesBookmark *orig;
    
    GList *clone_ls           = NULL;
    PlacesBookmark *clone;

    PlacesBookmarkAction *open, *terminal;


    if(orig_ls == NULL){
        pbuser_build_bookmarks(bookmark_group);
        orig_ls = pbg_priv(bookmark_group)->bookmarks;

        if(orig_ls == NULL)
            return NULL;
    }

    orig_ls = g_list_last((GList*) orig_ls);

    while(orig_ls != NULL){

        orig = (PlacesBookmark*) orig_ls->data;

        if(show_bookmark(orig)){

            clone                 = places_bookmark_create(g_strdup(orig->label));
            clone->uri            = g_strdup(orig->uri);
            clone->uri_scheme     = orig->uri_scheme;
            clone->icon           = g_object_ref(orig->icon);
            clone->finalize       = pbuser_finalize_bookmark;
    
            if(orig->uri_scheme == PLACES_URI_SCHEME_FILE) {
                /* terminal action only works on native files. */
                terminal              = places_create_open_terminal_action(clone);
                clone->actions        = g_list_prepend(clone->actions, terminal);
            }
            open                  = places_create_open_action(clone);
            clone->actions        = g_list_prepend(clone->actions, open);
            clone->primary_action = open;
    
            clone_ls = g_list_prepend(clone_ls, clone);
        }
        orig_ls  = orig_ls->prev;
    }

    return clone_ls;

}

static gboolean
pbuser_changed(PlacesBookmarkGroup *bookmark_group)
{
    PlacesBookmark *bookmark;
    GList * bookmarks;
    time_t mtime;
    gboolean ret;

    /* If we haven't even tried, we should load the bookmarks */
    if(pbg_priv(bookmark_group)->loaded == 0)
        goto pbuser_did_change;

    /* see if the file has changed (mtime or existence) */
    mtime = pbuser_get_mtime(pbg_priv(bookmark_group)->filename);
    if(mtime != pbg_priv(bookmark_group)->loaded)
        goto pbuser_did_change;
    
    /* see if any directories have been created or removed */
    bookmarks = pbg_priv(bookmark_group)->bookmarks;
    ret = FALSE;

    while(bookmarks != NULL){
        bookmark = bookmarks->data;
        if(bookmark->uri_scheme != PLACES_URI_SCHEME_REMOTE && show_bookmark(bookmark) != pbuser_dir_exists(bookmark->uri)){
            bookmark->priv = GINT_TO_POINTER(!show_bookmark(bookmark));
            ret = TRUE;
        }
        bookmarks = bookmarks->next;
    }
    if(ret)
        return TRUE;

    /* if we're still here, assume nothing changed */
    return FALSE;

  pbuser_did_change:
    pbuser_destroy_bookmarks(bookmark_group);
    return TRUE;
}

static void
pbuser_finalize(PlacesBookmarkGroup *bookmark_group)
{
    pbuser_destroy_bookmarks(bookmark_group);

    g_free(pbg_priv(bookmark_group)->filename);
    pbg_priv(bookmark_group)->filename = NULL;

    g_free(pbg_priv(bookmark_group));
    bookmark_group->priv = NULL;
}


/* external interface */

PlacesBookmarkGroup*
places_bookmarks_user_create(void)
{ 
    PlacesBookmarkGroup *bookmark_group;
    
    bookmark_group = places_bookmark_group_create();
    bookmark_group->get_bookmarks       = pbuser_get_bookmarks;
    bookmark_group->changed             = pbuser_changed;
    bookmark_group->finalize            = pbuser_finalize;
    bookmark_group->priv                = g_new0(PBUserData, 1);

    pbg_priv(bookmark_group)->filename = xfce_get_homefile(".gtk-bookmarks", NULL);
    
    return bookmark_group;
}

/* vim: set ai et tabstop=4: */
