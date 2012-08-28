/*  xfce4-places-plugin
 *
 *  This file provides wrappers to open external applications.
 *
 *  Copyright (c) 2007 Diego Ongaro <ongardie@gmail.com>
 *
 *  Error dialog code adapted from thunar's thunar-dialogs.c:
 *      Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#include <glib.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#define EXO_API_SUBJECT_TO_CHANGE
#include <exo/exo.h>

#include "string.h"

#include "support.h"
#include "model.h"

/**
 * Opens file browser at the location given by path.
 * If path is NULL or empty, it will open the file browser at the default
 * location (home).
 * The caller is in charge of freeing path.
 */
void
places_load_file_browser(const gchar *path)
{
    GError *error = NULL;

    if(path != NULL && *path != '\0'){

        DBG("Open file manager at %s", path);
        exo_execute_preferred_application("FileManager", path, NULL, NULL, &error);

    }else{

        gchar *home;
        home = g_strconcat("file://", xfce_get_homedir(), NULL);
        places_load_file_browser(home);
        g_free(home);

    }
}

/**
 * Opens the terminal at the location given by path.
 * If path is NULL or empty, it will open the terminal at the default location (home).
 * The caller is in charge of freeing path.
 */
void
places_load_terminal(const gchar *const_path)
{
    gchar *path = NULL;
    gboolean path_owner = FALSE; /* whether this function "owns" path */

    if(const_path != NULL){
        if(strncmp(const_path, "trash://", 8) == 0){
            DBG("Can't load terminal at trash:// URI's");
            return;

        }else if(strncmp(const_path, "file://", 7) == 0){
            path = g_filename_from_uri(const_path, NULL, NULL);
            path_owner = TRUE;

        }else{
            path = (gchar*) const_path;
            /* (path_owner is FALSE) */
            
        }
    }

    DBG("Open terminal emulator at %s", path);
    exo_execute_preferred_application("TerminalEmulator", NULL, path, NULL, NULL);

    if(path_owner && path != NULL)
        g_free(path);
}

/**
 * Loads the file given by path.
 * If path is NULL or empty, it will do nothing.
 * The caller is in charge of freeing path.
 */
void
places_load_file(const gchar *path)
{
    GError *error = NULL;
    if(path != NULL && *path != '\0')
        gtk_show_uri(NULL , path, 0, &error);
}

/**
 * Runs the graphical command given by cmd
 * If cmd is NULL or empty, it will do nothing.
 * The caller is in charge of freeing cmd.
 */
void
places_gui_exec(const gchar *cmd)
{
    if(cmd != NULL && *cmd != '\0')
        xfce_spawn_command_line_on_screen(gdk_screen_get_default(),
                        cmd, FALSE, TRUE, NULL);
}

static void
psupport_load_file_browser_wrapper(PlacesBookmarkAction *act)
{
    g_assert(act != NULL);

    /* we stored the path in priv */
    places_load_file_browser((gchar*) act->priv);
}

static void
psupport_load_terminal_wrapper(PlacesBookmarkAction *act)
{
    g_assert(act != NULL);

    /* we stored the path in priv */
    places_load_terminal((gchar*) act->priv);
}


PlacesBookmarkAction*
places_create_open_action(const PlacesBookmark *bookmark)
{
    PlacesBookmarkAction *action;

    g_assert(bookmark != NULL);
    g_assert(bookmark->uri != NULL);

    action                = places_bookmark_action_create(_("Open"));
    action->priv          = bookmark->uri;
    action->action        = psupport_load_file_browser_wrapper;

    return action;
}

PlacesBookmarkAction*
places_create_open_terminal_action(const PlacesBookmark *bookmark)
{
    PlacesBookmarkAction *action;

    g_assert(bookmark != NULL);
    g_assert(bookmark->uri != NULL);

    action            = places_bookmark_action_create(_("Open Terminal Here"));
    action->priv      = bookmark->uri;
    action->action    = psupport_load_terminal_wrapper;

    return action;
}


void
places_show_error_dialog (const GError *error,
                          const gchar  *format,
                          ...)
{
    GtkWidget *dialog;
    va_list    args;
    gchar     *primary_text;

    /* determine the primary error text */
    va_start (args, format);
    primary_text = g_strdup_vprintf (format, args);
    va_end (args);

    /* allocate the error dialog */
    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s.", primary_text);

    /* set secondary text if an error is provided */
    if (G_LIKELY (error != NULL)){
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), 
                                                  "%s.", error->message);
    }

    /* display the dialog */
    gtk_dialog_run (GTK_DIALOG (dialog));

    /* cleanup */
    gtk_widget_destroy (dialog);
    g_free (primary_text);

}

/* vim: set ai et tabstop=4: */
