dnl xfce4-places-plugin

dnl version info
XDT_VERSION_INIT([1.8.3], [git])

dnl initialize autoconf
AC_COPYRIGHT([Copyright (c) 2007-2009 Diego Ongaro <ongardie@gmail.com>
              2010-2024 The Xfce development team. All rights reserved.])
AC_INIT([xfce4-places-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-places-plugin], [xfce4-places-plugin])

AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])
AC_CONFIG_MACRO_DIRS([m4])

dnl initialize automake
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-bzip2 tar-ustar foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl check for basic programs
AC_PROG_CC()
AM_PROG_CC_C_O
AC_PROG_INSTALL()

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl check for required packages
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GOBJECT], [gobject-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.12.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.13.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.14.0])
XDT_CHECK_PACKAGE([EXO], [exo-2], [0.11.0])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [4.12.0])

XDT_CHECK_OPTIONAL_PACKAGE([GIO_UNIX], [gio-unix-2.0], [2.50.0], [gio-unix],
    [gio-unix, for special treatment of mount points on UNIX])

dnl *** Optional support for libnotify ***
XDT_CHECK_OPTIONAL_PACKAGE([LIBNOTIFY], [libnotify], [0.4.0], [notifications],
                           [Mount notification support], [yes])

dnl check for i18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Debug Support:    $enable_debug"
echo
