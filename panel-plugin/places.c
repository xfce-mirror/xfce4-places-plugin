/*  xfce4-places-plugin
 *
 *  This is the main plugin file. It starts the init and finalize processes.
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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

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

XFCE_PANEL_PLUGIN_REGISTER(places_construct);

/* vim: set ai et tabstop=4: */
