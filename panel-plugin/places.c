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

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/libxfcegui4.h>
#include <exo/exo.h>

#include "places.h"
#include "view.h"

#include "string.h" // for strncmp

static void places_construct(XfcePanelPlugin*);
static void places_finalize(XfcePanelPlugin*, PlacesData*);

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(places_construct);

/**
 * Initializes the plugin:
 *
 * 1. Sets up i18n
 * 2. Creates the PlacesData struct
 * 3. Asks the view to initialize
 * 4. Connects the finalize callback.
 */
static void 
places_construct(XfcePanelPlugin *plugin)
{
    DBG("Construct: %s", PLUGIN_NAME);
   
    // Set up i18n
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8"); 

    // Create the PlacesData struct
    PlacesData *pd = panel_slice_new0(PlacesData);
    pd->plugin = plugin;

    // Initialize view
    places_view_init(pd);

    // Connect the finalize callback
    g_signal_connect(pd->plugin, "free-data", 
                     G_CALLBACK(places_finalize), pd);

}

/**
 * Cleans up resources.
 *
 * 1. Asks the view to finalize
 * 2. Frees the PlacesData struct
 */
static void 
places_finalize(XfcePanelPlugin *plugin, PlacesData *pd)
{
    DBG("Free data: %s", PLUGIN_NAME);
    g_assert(pd != NULL);
   
    // finalize the view
    places_view_finalize(pd);
    
    // free the PlacesData struct
    panel_slice_free(PlacesData, pd);
}

/**
 * Opens Thunar at the location given by path.
 * If path is NULL or empty, it will open Thunar at the default location (home).
 * This function is also abused to open files.
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

void
places_load_file(const gchar *path)
{
    exo_url_show(path, NULL, NULL);
}

void
places_gui_exec(const gchar *cmd)
{
    if(cmd != NULL)
        xfce_exec(cmd, FALSE, TRUE, NULL);
}

// vim: ai et tabstop=4
