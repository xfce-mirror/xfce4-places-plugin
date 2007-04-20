#!/bin/sh
svn co http://svn.gnome.org/svn/gnome-panel/trunk/po/ gnome-panel
svn co http://svn.xfce.org/svn/xfce/thunar/trunk/po/ thunar

cd ../po/
intltool-update --pot

cd ../scripts/

./po_parser.py && rm -rf gnome-panel && rm -rf thunar

cd ../po/
intltool-update -r
svn add *.po
