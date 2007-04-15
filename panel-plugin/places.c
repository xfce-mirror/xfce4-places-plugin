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

#include "places.h"
#include "model.h"
#include "view.h"

static void places_construct(XfcePanelPlugin*);
static void places_finalize(XfcePanelPlugin*, PlacesData*);

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(places_construct);

/**
 * Initializes the plugin:
 *
 * 1. Sets up i18n
 * 2. Creates the PlacesData struct
 * 3. Asks the model to initialize
 * 4. Asks the view to initialize
 * 5. Connects the finalize callback.
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

    // Initialize model
    pd->bookmarks = places_bookmarks_init();

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
 * 2. Asks the model to finalize
 * 3. Frees the PlacesData struct
 */
static void 
places_finalize(XfcePanelPlugin *plugin, PlacesData *pd)
{
    DBG("Free data: %s", PLUGIN_NAME);
    g_assert(pd != NULL);
   
    // finalize the view
    places_view_finalize(pd);
    
    // finalize the model
    places_bookmarks_finalize(pd->bookmarks);

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
        xfce_exec(cmd, FALSE, TRUE, NULL);
        g_free(cmd);

    }else{
        DBG("exec: thunar");
        xfce_exec("thunar", FALSE, TRUE, NULL);
    }
}

// vim: ai et tabstop=4
