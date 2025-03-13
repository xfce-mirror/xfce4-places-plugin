plugin_sources = [
  'button.c',
  'button.h',
  'cfg.c',
  'cfg.h',
  'model.c',
  'model.h',
  'model_system.c',
  'model_system.h',
  'model_user.c',
  'model_user.h',
  'model_volumes.c',
  'model_volumes.h',
  'places.c',
  'places.h',
  'support.c',
  'support.h',
  'view.c',
  'view.h',
  xfce_revision_h,
]

if have_libnotify
  plugin_sources += [
    'model_volumes_notify.c',
    'model_volumes_notify.h',
  ]
endif

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_library(
  'places',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-places-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    exo,
    gio,
    gio_unix,
    glib,
    gmodule,
    gobject,
    gthread,
    gtk,
    libnotify,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'places.desktop.in',
  output: 'places.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)

configure_file(
  input: 'xfce4-popup-places.sh',
  output: 'xfce4-popup-places',
  configuration: {
    'bindir': libxfce4panel.get_variable(pkgconfig: 'prefix') / get_option('bindir'),
    'localedir': get_option('prefix') / get_option('localedir'),
  },
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)
