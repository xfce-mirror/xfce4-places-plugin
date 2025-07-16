/*  xfce4-places-plugin
 *
 *  This file provides a means of configuring the plugin.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
 *  Copyright (c) 2012 Andrzej <ndrwrdck@gmail.com>
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

#include <string.h>

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "cfg.h"
#include "view.h"

static void             places_cfg_finalize              (GObject         *object);
static void             places_cfg_get_property          (GObject         *object,
                                                          guint            prop_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);
static void             places_cfg_set_property          (GObject         *object,
                                                          guint            prop_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);

enum
{
  PROP_0,
  PROP_SHOW_BUTTON_TYPE,
  PROP_BUTTON_LABEL,
  PROP_SHOW_ICONS,
  PROP_SHOW_VOLUMES,
  PROP_MOUNT_OPEN_VOLUMES,
  PROP_SHOW_BOOKMARKS,
  PROP_SHOW_RECENT,
  PROP_SHOW_RECENT_CLEAR,
  PROP_SHOW_RECENT_NUMBER,
  PROP_SEARCH_CMD
};

enum
{
  BUTTON_CHANGED,
  MENU_CHANGED,
  MODEL_CHANGED,
  LAST_SIGNAL
};

static guint places_cfg_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (PlacesCfg, places_cfg, G_TYPE_OBJECT)

static void
places_cfg_class_init (PlacesCfgClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = places_cfg_finalize;
  gobject_class->get_property = places_cfg_get_property;
  gobject_class->set_property = places_cfg_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_BUTTON_TYPE,
                                   g_param_spec_uint ("show-button-type",
                                                      NULL, NULL,
                                                      0,
                                                      2,
                                                      0,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BUTTON_LABEL,
                                   g_param_spec_string ("button-label",
                                                        NULL, NULL,
                                                        _("Places"),
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_ICONS,
                                   g_param_spec_boolean ("show-icons", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_VOLUMES,
                                   g_param_spec_boolean ("show-volumes", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_MOUNT_OPEN_VOLUMES,
                                   g_param_spec_boolean ("mount-open-volumes", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_BOOKMARKS,
                                   g_param_spec_boolean ("show-bookmarks", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_RECENT,
                                   g_param_spec_boolean ("show-recent", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_RECENT_CLEAR,
                                   g_param_spec_boolean ("show-recent-clear", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_RECENT_NUMBER,
                                   g_param_spec_uint ("show-recent-number",
                                                      NULL, NULL,
                                                      1,
                                                      25,
                                                      10,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SEARCH_CMD,
                                   g_param_spec_string ("search-cmd",
                                                        NULL, NULL,
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  places_cfg_signals[BUTTON_CHANGED] =
    g_signal_new (g_intern_static_string ("button-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  places_cfg_signals[MENU_CHANGED] =
    g_signal_new (g_intern_static_string ("menu-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  places_cfg_signals[MODEL_CHANGED] =
    g_signal_new (g_intern_static_string ("model-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
places_cfg_init (PlacesCfg *cfg)
{
  cfg->show_button_icon   = TRUE;
  cfg->show_button_label  = FALSE;
  cfg->show_icons         = TRUE;
  cfg->show_volumes       = TRUE;
  cfg->mount_open_volumes = FALSE;
  cfg->show_bookmarks     = TRUE;
  cfg->show_recent        = TRUE;
  cfg->show_recent_clear  = TRUE;
  cfg->show_recent_number = 10;
  cfg->search_cmd = g_strdup("");
  cfg->label = g_strdup(_("Places"));
}


static void
places_cfg_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  PlacesCfg     *cfg = XFCE_PLACES_CFG (object);
  gint           val;

  switch (prop_id)
    {
    case PROP_SHOW_BUTTON_TYPE:
      if      ( cfg->show_button_icon && !cfg->show_button_label) val = 0;
      else if ( cfg->show_button_icon &&  cfg->show_button_label) val = 2;
      else                                                        val = 1;

      g_value_set_uint (value, val);
      break;

    case PROP_BUTTON_LABEL:
      g_value_set_string (value, cfg->label);
      break;

    case PROP_SHOW_ICONS:
      g_value_set_boolean (value, cfg->show_icons);
      break;

    case PROP_SHOW_VOLUMES:
      g_value_set_boolean (value, cfg->show_volumes);
      break;

    case PROP_MOUNT_OPEN_VOLUMES:
      g_value_set_boolean (value, cfg->mount_open_volumes);
      break;

    case PROP_SHOW_BOOKMARKS:
      g_value_set_boolean (value, cfg->show_bookmarks);
      break;

    case PROP_SHOW_RECENT:
      g_value_set_boolean (value, cfg->show_recent);
      break;

    case PROP_SHOW_RECENT_CLEAR:
      g_value_set_boolean (value, cfg->show_recent_clear);
      break;

    case PROP_SHOW_RECENT_NUMBER:
      g_value_set_uint (value, cfg->show_recent_number);
      break;

    case PROP_SEARCH_CMD:
      g_value_set_string (value, cfg->search_cmd);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
places_cfg_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  PlacesCfg     *cfg = XFCE_PLACES_CFG (object);
  gint           val;
  const char    *text;

  DBG ("Property changed");
  switch (prop_id)
    {
    case PROP_SHOW_BUTTON_TYPE:
      val = g_value_get_uint (value);
      if (cfg->show_button_icon != (val == 0 || val == 2))
        {
          cfg->show_button_icon = (val == 0 || val == 2);
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[BUTTON_CHANGED], 0);
        }
      if (cfg->show_button_label != (val == 1 || val == 2))
        {
          cfg->show_button_label = (val == 1 || val == 2);
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[BUTTON_CHANGED], 0);
        }
      break;

    case PROP_BUTTON_LABEL:
      text = g_value_get_string (value);
      if (strcmp(cfg->label, text))
        {
          g_free (cfg->label);
          cfg->label = g_value_dup_string (value);
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[BUTTON_CHANGED], 0);
        }
      break;

    case PROP_SHOW_ICONS:
      val = g_value_get_boolean (value);
      if (cfg->show_icons != val)
        {
          cfg->show_icons = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MENU_CHANGED], 0);
        }
      break;

    case PROP_SHOW_VOLUMES:
      val = g_value_get_boolean (value);
      if (cfg->show_volumes != val)
        {
          cfg->show_volumes = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MODEL_CHANGED], 0);
        }
      break;

    case PROP_MOUNT_OPEN_VOLUMES:
      val = g_value_get_boolean (value);
      if (cfg->mount_open_volumes != val)
        {
          cfg->mount_open_volumes = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MODEL_CHANGED], 0);
        }
      break;

    case PROP_SHOW_BOOKMARKS:
      val = g_value_get_boolean (value);
      if (cfg->show_bookmarks != val)
        {
          cfg->show_bookmarks = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MODEL_CHANGED], 0);
        }
      break;

    case PROP_SHOW_RECENT:
      val = g_value_get_boolean (value);
      if (cfg->show_recent != val)
        {
          cfg->show_recent = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MENU_CHANGED], 0);
        }
      break;

    case PROP_SHOW_RECENT_CLEAR:
      val = g_value_get_boolean (value);
      if (cfg->show_recent_clear != val)
        {
          cfg->show_recent_clear = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MENU_CHANGED], 0);
        }
      break;

    case PROP_SHOW_RECENT_NUMBER:
      val = g_value_get_uint (value);
      if (cfg->show_recent_number != val)
        {
          cfg->show_recent_number = val;
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MENU_CHANGED], 0);
        }
      break;

    case PROP_SEARCH_CMD:
      text = g_value_get_string (value);
      if (strcmp(cfg->search_cmd, text))
        {
          g_free (cfg->search_cmd);
          cfg->search_cmd = g_value_dup_string (value);
          g_signal_emit (G_OBJECT (cfg), places_cfg_signals[MENU_CHANGED], 0);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/********** Dialog **********/

static void
pcfg_dialog_close_cb(GtkDialog *dialog, gint response, PlacesCfg *cfg)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
    xfce_panel_plugin_unblock_menu(cfg->plugin);
}

static GtkWidget*
pcfg_make_empty_dialog(PlacesCfg *cfg)
{
    GtkWidget *dlg; /* we'll return this */

    xfce_panel_plugin_block_menu(cfg->plugin);

    dlg = xfce_titled_dialog_new_with_mixed_buttons(_("Places"),
              NULL,
              GTK_DIALOG_DESTROY_WITH_PARENT,
              "window-close-symbolic", _("Close"), GTK_RESPONSE_ACCEPT, NULL);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "xfce4-settings");

    g_signal_connect(dlg, "response",
                     G_CALLBACK(pcfg_dialog_close_cb), cfg);

    return dlg;
}

static GtkWidget*
get_label (const gchar *str,
           gboolean     bold,
           gboolean     upper_margin)
{
    GtkWidget *label;

    if (bold) {
        const char *format = "<b>\%s</b>";
        char *markup;

        label = gtk_label_new (NULL);
        markup = g_markup_printf_escaped(format, str);
        gtk_label_set_markup(GTK_LABEL(label), markup);
        g_free(markup);
    } else {
        label = gtk_label_new_with_mnemonic(str);
    }

    gtk_label_set_xalign (GTK_LABEL (label), 0);

    if (upper_margin)
        gtk_widget_set_margin_top (label, 12);

    return label;
}

static void
attach_to_grid (GtkGrid   *grid,
                GtkWidget *child,
                gint       left,
                gint       top,
                gint       width,
                gint       height,
                gint       margin_start)
{
    gtk_grid_attach (grid, child, left, top, width, height);
    gtk_widget_set_margin_start (child, margin_start);
}

void
places_cfg_open_dialog(PlacesCfg *cfg)
{
    GtkAdjustment *adj;
    GtkWidget     *dlg;
    GtkWidget     *grid;
    GtkWidget     *label;
    GtkWidget     *widget;

    gint margin_size = 12;
    gint row = 0;

    DBG("configure plugin");

    dlg = pcfg_make_empty_dialog(cfg);

    grid = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
    gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), grid, FALSE, FALSE, 0);

    /* BUTTON */
    label = get_label (_("Button"), TRUE, FALSE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 3, 1, 0);
    row++;

    /* BUTTON: Show Icon/Label */
    label = get_label (_("_Show"), FALSE, FALSE);
    attach_to_grid (GTK_GRID(grid), label, 0, row, 1, 1, margin_size);

    widget = gtk_combo_box_text_new();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), _("Icon Only"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), _("Label Only"));
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), _("Icon and Label"));

    g_object_bind_property (G_OBJECT (cfg), "show-button-type",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 1, row, 2, 1, 0);
    row++;

    /* BUTTON: Label text entry */
    label = get_label (_("_Label"), FALSE, FALSE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 1, 1, margin_size);

    widget = gtk_entry_new();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
    g_object_bind_property (G_OBJECT (cfg), "button-label",
                            G_OBJECT (widget), "text",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 1, row, 2, 1, 0);
    row++;

    /* MENU */
    label = get_label (_("Menu"), TRUE, TRUE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 3, 1, 0);
    row++;

    /* MENU: Show Icons */
    widget = gtk_check_button_new_with_mnemonic (_("Show _icons in menu"));
    g_object_bind_property (G_OBJECT (cfg), "show-icons",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size);
    row++;

    /* MENU: Show Removable Media */
    widget = gtk_check_button_new_with_mnemonic(_("Show _removable media"));
    g_object_bind_property (G_OBJECT (cfg), "show-volumes",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size);
    row++;

    /* MENU: - Mount and Open (indented) */
    widget = gtk_check_button_new_with_mnemonic(_("Mount and _Open on click"));
    g_object_bind_property (G_OBJECT (cfg), "mount-open-volumes",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    /* Gray out this box when "Show removable media" is off */
    g_object_bind_property (G_OBJECT (cfg), "show-volumes",
                            G_OBJECT (widget), "sensitive",
                            G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size * 2);
    row++;

    /* MENU: Show GTK Bookmarks */
    widget = gtk_check_button_new_with_mnemonic(_("Show GTK _bookmarks"));
    g_object_bind_property (G_OBJECT (cfg), "show-bookmarks",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size);
    row++;

    /* MENU: Show Recent Documents */
    widget = gtk_check_button_new_with_mnemonic(_("Show recent _documents"));
    g_object_bind_property (G_OBJECT (cfg), "show-recent",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size);
    row++;

    /* RECENT DOCUMENTS */
    label = get_label (_("Recent Documents"), TRUE, TRUE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 3, 1, 0);
    row++;

    /* Gray out this box when "Show recent documents" is off */
    g_object_bind_property (G_OBJECT (cfg), "show-recent",
                            G_OBJECT (label), "sensitive",
                            G_BINDING_SYNC_CREATE);

    /* RECENT DOCUMENTS: Show clear option */
    widget = gtk_check_button_new_with_mnemonic(_("Show cl_ear option"));
    g_object_bind_property (G_OBJECT (cfg), "show-recent-clear",
                            G_OBJECT (widget), "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    /* Gray out this box when "Show recent documents" is off */
    g_object_bind_property (G_OBJECT (cfg), "show-recent",
                            G_OBJECT (widget), "sensitive",
                            G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 0, row, 3, 1, margin_size);
    row++;

    /* RECENT DOCUMENTS: Number to display */
    label = get_label (_("_Number to display"), FALSE, FALSE);

    /* Gray out this box when "Show recent documents" is off */
    g_object_bind_property (G_OBJECT (cfg), "show-recent",
                            G_OBJECT (label), "sensitive",
                            G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), label, 0, row, 2, 1, margin_size);

    adj = gtk_adjustment_new (cfg->show_recent_number, 1, 25, 1, 5, 0);

    widget = gtk_spin_button_new (adj, 1, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);

    g_object_bind_property (G_OBJECT (cfg), "show-recent-number",
                            G_OBJECT (adj), "value",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    /* Gray out this box when "Show recent documents" is off */
    g_object_bind_property (G_OBJECT (cfg), "show-recent",
                            G_OBJECT (widget), "sensitive",
                            G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 2, row, 1, 1, 0);
    row++;

    /* SEARCH */
    label = get_label (_("Search"), TRUE, TRUE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 3, 1, 0);
    row++;

    /* Search: command */
    label = get_label (_("Co_mmand"), FALSE, FALSE);
    attach_to_grid (GTK_GRID (grid), label, 0, row, 1, 1, margin_size);

    widget = gtk_entry_new();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
    g_object_bind_property (G_OBJECT (cfg), "search-cmd",
                            G_OBJECT (widget), "text",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    attach_to_grid (GTK_GRID (grid), widget, 1, row, 2, 1, 0);

    gtk_widget_show_all (dlg);
}

/********** Initialization & Finalization **********/

static void
places_cfg_finalize (GObject *object)
{
  PlacesCfg *cfg = XFCE_PLACES_CFG (object);
  DBG("PlacesCfg finalize called");

    if(cfg->label != NULL)
        g_free(cfg->label);
    if(cfg->search_cmd != NULL)
        g_free(cfg->search_cmd);

  xfconf_shutdown();
  G_OBJECT_CLASS (places_cfg_parent_class)->finalize (object);
}

PlacesCfg*
places_cfg_new(XfcePanelPlugin *plugin)
{
    PlacesCfg     *cfg;
    XfconfChannel *channel;
    gchar         *property;

    cfg             = g_object_new (XFCE_TYPE_PLACES_CFG, NULL);
    cfg->plugin     = plugin;

    xfconf_init(NULL);
    channel = xfconf_channel_get ("xfce4-panel");

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-button-type", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_INT, cfg, "show-button-type");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/button-label", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_STRING, cfg, "button-label");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-icons", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "show-icons");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-volumes", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "show-volumes");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/mount-open-volumes", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "mount-open-volumes");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-bookmarks", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "show-bookmarks");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-recent", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "show-recent");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-recent-clear", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, cfg, "show-recent-clear");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/show-recent-number", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_INT, cfg, "show-recent-number");
    g_free (property);

    property = g_strconcat (xfce_panel_plugin_get_property_base (plugin), "/search-cmd", NULL);
    xfconf_g_property_bind (channel, property, G_TYPE_STRING, cfg, "search-cmd");
    g_free (property);

    g_signal_connect_swapped(G_OBJECT(plugin), "configure-plugin",
                             G_CALLBACK(places_cfg_open_dialog), cfg);

    xfce_panel_plugin_menu_show_configure(plugin);

    return cfg;
}

/* vim: set ai et tabstop=4: */
