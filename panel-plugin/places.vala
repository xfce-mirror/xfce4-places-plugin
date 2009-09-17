/*  xfce4-places-plugin
 *
 *  This is the main plugin file. It starts the init and finalize processes.
 *
 *  Copyright (c) 2007, 2009 Diego Ongaro <ongardie@gmail.com>
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

public class PlacesPlugin : GLib.Object {

    private Places.View view;

    /**
     * Initializes the plugin.
     */
    public PlacesPlugin (Xfce.PanelPlugin panel_plugin) {

        GLib.debug ("Construct: %s", Config.PACKAGE);

        /* Set up i18n */
        Xfce.textdomain (Config.GETTEXT_PACKAGE,
                         Config.PACKAGE_LOCALE_DIR,
                         "UTF-8");

        /* Initialize view */
        view = new Places.View (panel_plugin);

        /* Clean up resources */
        panel_plugin.free_data.connect ((s) => {

            GLib.debug ("Finalize: %s", Config.PACKAGE);
            view.finalize ();
        });

        GLib.debug ("done");
    }

    static PlacesPlugin plugin;

    public static void register (Xfce.PanelPlugin panel_plugin) {
        plugin = new PlacesPlugin (panel_plugin);
    }

    public static int main (string[] args) {
        return Xfce.PanelPluginRegisterExternal (ref args,
                                                 PlacesPlugin.register);
    }
}

/* vim: set et ts=4 sw=4: */
