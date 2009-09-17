/*  xfce4-places-plugin
 *
 *  Provides the widget that sits on the panel
 *
 *  Note that, while this extends GtkToggleButton, much of the gtk_button_*()
 *  functions shouldn't be used.
 *
 *  Copyright (c) 2007-2009 Diego Ongaro <ongardie@gmail.com>
 *  
 *  Some code adapted from libxfce4panel for the togglebutton configuration
 *    (xfce-panel-convenience.c)
 *    Copyright (c) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 *  Some code adapted from gtk+ for the properties
 *    (gtkbutton.c)
 *    Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *    Modified by the GTK+ Team and others 1997-2001
 *
 *  May also contain code adapted from:
 *   - notes plugin
 *     panel-plugin.c - (xfce4-panel plugin for temporary notes)
 *     Copyright (c) 2006 Mike Massonnet <mmassonnet@gmail.com>
 *
 *   - xfdesktop menu plugin
 *     desktop-menu-plugin.c - xfce4-panel plugin that displays the desktop menu
 *     Copyright (C) 2004 Brian Tarricone, <bjt23@cornell.edu>
 *  
 *   - launcher plugin
 *     launcher.c - (xfce4-panel plugin that opens programs)
 *     Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *     Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
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


public class Places.Button : Gtk.ToggleButton {

    public static delegate Gdk.Pixbuf pixbuf_factory_delegate (int size);
    private static const int BOX_SPACING = 4;

    private string? _label_text;
    private pixbuf_factory_delegate? _pixbuf_factory;
    private Xfce.PanelPlugin _panel_plugin;
    private Xfce.HVBox _box;
    private int _plugin_size;
    private Gtk.Image? _image;
    private Gtk.Label? _label;

    private void resize_image (int new_size, out int width, out int height) {
        width = 0;
        height = 0;

        if (this._pixbuf_factory == null) {
            if (this._image != null) {
                this._image.destroy ();
                this._image = null;
            }
            return;
        }

        Gdk.Pixbuf icon = this._pixbuf_factory (new_size);
        if (icon == null) {
            GLib.debug ("Could not load icon for button");
            if (this._image != null) {
                this._image.destroy ();
                this._image = null;
            }
            return;
        }

        width  = icon.width;
        height = icon.height;

        if (this._image == null) {
            this._image = new Gtk.Image.from_pixbuf (icon);
            this._box.pack_start (this._image, true, true, 0);
            this._image.show ();
        }
        else {
            this._image.set_from_pixbuf (icon);
        }
    }

    private void resize_label (out int width, out int height) {
        width = 0;
        height = 0;

        if (this._label_text == null) {
            if (this._label != null) {
                this._label.destroy ();
                this._label = null;
            }
            return;
        }

        if (this._label == null) {
            this._label = new Gtk.Label (this._label_text);
            this._box.pack_end (this._label, true, true, 0);
            this._label.show ();
        }
        else {
            this._label.set_text (this._label_text);
        }

        Gtk.Requisition req;
        this._label.size_request (out req);
        width  = req.width;
        height = req.height;
    }

    private void resize () {

        int new_size = this._panel_plugin.size;
        this._plugin_size = new_size;
        GLib.debug ("Panel size: %d", new_size);

        Gtk.Orientation orientation = this._panel_plugin.get_orientation ();

        bool show_image = (this._pixbuf_factory != null);
        bool show_label = (this._label_text != null);

        int total_width = 0;
        int total_height = 0;

        /* these will be added into totals later */
        int button_width  = 2 + 2 * this.style.xthickness;
        int button_height = 2 + 2 * this.style.ythickness;

        /* image */
        int image_size = new_size - int.max (button_width, button_height);
        /* TODO: could check if anything changed
         * (though it's hard to know if the icon theme changed) */
        int image_width;
        int image_height;
        this.resize_image (image_size, out image_width, out image_height);
        show_image = (this._image != null);
        if (show_image) {
            image_width  = int.max (image_width, image_size);
            image_height = int.max (image_height, image_size);
            total_width  += image_width;
            total_height += image_height;
        }

        /* label */
        /* TODO: could check if anything changed */
        int label_width;
        int label_height;
        this.resize_label (out label_width, out label_height);
        show_label = (this._label != null);
        if (show_label) {
            if (orientation == Gtk.Orientation.HORIZONTAL) {
                total_width  += label_width;
                total_height  = int.max (total_height, label_height);
            }
            else {
                total_width   = int.max (total_width, label_width);
                total_height += label_height;
            }
        }

        /* at this point, total width and height reflect just image and label */
        /* now, add on the button and box overhead */
        total_width  += button_width;
        total_height += button_height;

        int box_width  = 0;
        int box_height = 0;

        if (show_image && show_label) {

            if (orientation == Gtk.Orientation.HORIZONTAL)
                box_width  = this.BOX_SPACING;
            else
                box_height = this.BOX_SPACING;

            total_width  += box_width;
            total_height += box_height;
        }

        if (orientation == Gtk.Orientation.HORIZONTAL)
            total_height = int.max (total_height, new_size);
        else
            total_width  = int.max (total_width,  new_size);

        GLib.debug ("width=%d, height=%d", total_width, total_height);
        this.set_size_request (total_width, total_height);

    }

    private void orientation_changed (Gtk.Orientation orientation) {
        GLib.debug ("orientation changed");
        this._box.set_orientation (orientation);
        this.resize ();
    }

    private bool size_changed (int size) {
        if (this._plugin_size == size)
            return true;

        GLib.debug ("size changed");
        this.resize ();
        return true;
    }

    private void theme_changed () {
        GLib.debug ("theme changed");
        this.resize ();
    }

    [Description(nick="Label", blurb="Button text")]
    public new string? label {
        get {
            GLib.debug ("returning %s", this._label_text);
            return this._label_text;
        }

        set {
            if (value == this._label_text)
                return;

            GLib.debug ("new label text: %s", value);

            this._label_text = value;
            this.resize ();
        }
    }

    [Description(nick="Pixbuf factory", blurb="Factory to create icons for image to appear next to button text")]
    public pixbuf_factory_delegate? pixbuf_factory {
        get {
            //GLib.debug ("returning %p", this._pixbuf_factory);
            return this._pixbuf_factory;
        }

        set {
            if (value == this._pixbuf_factory)
                return;

            //GLib.debug ("new pixbuf factory: %p", value);

            this._pixbuf_factory = value;
            this.resize ();
        }
    }

    public Button (Xfce.PanelPlugin panel_plugin) {
        this._panel_plugin = panel_plugin;

        /* style like xfce_create_panel_button */
        this.can_default = false;
        this.can_focus = false;
        this.focus_on_click = false;
        this.set_relief (Gtk.ReliefStyle.NONE);

        var orientation = panel_plugin.get_orientation ();

        this._box = new Xfce.HVBox (orientation, false, this.BOX_SPACING);
        this._box.border_width = 0;
        this.add (this._box);
        this._box.show ();

        this.resize ();

        panel_plugin.orientation_changed.connect (this.orientation_changed);
        panel_plugin.size_changed.connect (this.size_changed);
        this.style_set.connect (this.theme_changed);
        this.screen_changed.connect (this.theme_changed);
    }

}

/* vim: set et ts=4 sw=4: */
