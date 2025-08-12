[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/panel-plugins/xfce4-places-plugin/-/blob/master/COPYING)

# xfce4-places-plugin

The xfce4-places-plugin brings much of the functionality of GNOME's Places menu to Xfce.

This plugin adds a small button to the panel. Clicking on it opens a menu, which shows:
* System folders: home directory, trash, desktop, file system
* Removable media
* User-defined bookmarks
* Recent Documents

----

### Homepage

[Xfce4-places-plugin documentation](https://docs.xfce.org/panel-plugins/xfce4-places-plugin)

### Changelog

See [NEWS](https://gitlab.xfce.org/panel-plugins/xfce4-places-plugin/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Xfce4-places-plugin source code](https://gitlab.xfce.org/panel-plugins/xfce4-places-plugin)

### Download a Release Tarball

[Xfce4-places-plugin archive](https://archive.xfce.org/src/panel-plugins/xfce4-places-plugin)
    or
[Xfce4-places-plugin tags](https://gitlab.xfce.org/panel-plugins/xfce4-places-plugin/-/tags)

### Installation

From source code repository: 

    % cd xfce4-places-plugin
    % meson setup build
    % meson compile -C build
    % meson install -C build

From release tarball:

    % tar xf xfce4-places-plugin-<version>.tar.xz
    % cd xfce4-places-plugin-<version>
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Uninstallation

    % ninja uninstall -C build

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/panel-plugins/xfce4-places-plugin/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

