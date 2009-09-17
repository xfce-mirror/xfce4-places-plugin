/* $Id$
 *
 *  Places - panel plugin for Xfce Desktop Environment
 *           popup command
 *  Adapted from xfce4-panel's windowlist plugin
 *
 *  Copyright (C) 2002-2006  Olivier Fourdan
 *                2007       Mike Massonnet <mmassonnet@xfce.com>
 *                2007       Diego Ongaro <ongardie@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "xfce4-popup-places.h"



static gboolean
plugin_check_is_running (GtkWidget *widget,
                         Window *xid)
{
    GdkScreen          *gscreen;
    gchar               selection_name[256];
    Atom                selection_atom;

    gscreen = gtk_widget_get_screen (widget);
    
    g_snprintf (selection_name, 256,
                XFCE_PLACES_SELECTION"%d",
                gdk_screen_get_number (gscreen));
    selection_atom = XInternAtom (GDK_DISPLAY (), selection_name, FALSE);

    if ((*xid = XGetSelectionOwner (GDK_DISPLAY (), selection_atom)))
        return TRUE;
    else
        return FALSE;
}

gint
main (gint argc, gchar *argv[])
{
    GdkEventClient        gev;
    GtkWidget            *win;
    Window                id;

    gtk_init (&argc, &argv);

    win = gtk_invisible_new ();
    gtk_widget_realize (win);

    gev.type              = GDK_CLIENT_EVENT;
    gev.window            = win->window;
    gev.send_event        = TRUE;
    gev.message_type      = gdk_atom_intern ("STRING", FALSE);
    gev.data_format       = 8;
    g_snprintf (gev.data.b, sizeof (gev.data.b), PLACES_MSG_MENU);

    if (plugin_check_is_running (win, &id))
        gdk_event_send_client_message ((GdkEvent *)&gev, (GdkNativeWindow)id);
    else
        g_warning ("Can't find the xfce4-places-plugin.\n");
    gdk_flush ();

    gtk_widget_destroy (win);

    return FALSE;
}

/* vim: set ai et tabstop=4: */
