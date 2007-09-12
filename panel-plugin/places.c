/*  xfce4-places-plugin
 *
 *  This is the main plugin file. It starts the init and finalize processes.
 *  Also, this file provides wrappers to open external applications.
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

#include <glib.h>

#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/libxfcegui4.h>
#include <exo/exo.h>

#include "string.h"

#include "places.h"
#include "view.h"


/**
 * Cleans up resources.
 */
static void 
places_finalize(XfcePanelPlugin *plugin, PlacesView *view)
{
    DBG("Finalize: %s", PLUGIN_NAME);
    g_assert(plugin != NULL);
    g_assert(view != NULL);
   
    /* Finalize view */
    places_view_finalize(view);
}

/**
 * Initializes the plugin.
 */
static void 
places_construct(XfcePanelPlugin *plugin)
{
    PlacesView *view;

    DBG("Construct: %s", PLUGIN_NAME);
   
    /* Set up i18n */
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8"); 

    /* Initialize view */
    view = places_view_init(plugin);

    /* Connect the finalize callback */
    g_signal_connect(plugin, "free-data", 
                     G_CALLBACK(places_finalize), view);

    DBG("done");
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(places_construct);



/**
 * Opens Thunar at the location given by path.
 * If path is NULL or empty, it will open Thunar at the default location (home).
 * The caller is in charge of freeing path.
 */
void
places_load_thunar(const gchar *path)
{
    if(path != NULL && *path != '\0'){

        gchar *cmd = g_strconcat("thunar \"", path, "\"", NULL);
        DBG("exec: %s", cmd);
        places_gui_exec(cmd);
        g_free(cmd);

    }else{
        DBG("exec: thunar");
        places_gui_exec("thunar");
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
    if(path != NULL && strlen(path))
        exo_url_show(path, NULL, NULL);
}

/**
 * Runs the graphical command given by cmd
 * If cmd is NULL or empty, it will do nothing.
 * The caller is in charge of freeing cmd.
 */
void
places_gui_exec(const gchar *cmd)
{
    if(cmd != NULL && strlen(cmd))
        xfce_exec(cmd, FALSE, TRUE, NULL);
}

/* vim: set ai et tabstop=4: */
