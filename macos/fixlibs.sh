#!/bin/sh

#
# This script will prepare the application (.app) bundle for Mac OS X.

# Hydrogen
# Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
# Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
#
# http://www.hydrogen-music.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# prepare the .app package icon

#echo " *** Copying icon"
#mkdir -p hydrogen.app/Contents/Resources
#cp macos/Hydrogen.icns hydrogen.app/Contents/Resources/Hydrogen.icns
#sed -e "s/<string></<string>Hydrogen.icns</" hydrogen.app/Contents/Info.plist

echo " *** Copying data directory"
rm -rf hydrogen.app/Contents/Resources/data
cp -R data hydrogen.app/Contents/Resources/data

echo " *** Copying plugins directory"
rm -rf hydrogen.app/Contents/Resources/plugins
mkdir -p hydrogen.app/Contents/Resources/plugins
cp -R plugins/*.dylib hydrogen.app/Contents/Resources/plugins

# remove the unnecesssary subversion directories

rm -rf `find hydrogen.app/Contents/Resources/data | grep "\.svn"`

# remove all the old dynamic libraries

#rm -rf hydrogen.app/Contents/Frameworks
#mkdir hydrogen.app/Contents/Frameworks

# copy all dynamic libraries and frameworks

#echo " *** Processing libsndfile"
#cp -L /usr/lib/libsndfile.dylib hydrogen.app/Contents/Frameworks/libsndfile.dylib
#install_name_tool -id @executable_path/../Frameworks/libsndfile.dylib hydrogen.app/Contents/Frameworks/libsndfile.dylib
#install_name_tool -change /usr/local/lib/libsndfile.1.dylib @executable_path/../Frameworks/libsndfile.dylib hydrogen.app/Contents/MacOS/hydrogen

#echo " *** Processing libFLAC"
#cp -L /usr/lib/libFLAC.dylib hydrogen.app/Contents/Frameworks/libFLAC.dylib
#install_name_tool -id @executable_path/../Frameworks/libFLAC.dylib hydrogen.app/Contents/Frameworks/libFLAC.dylib
#install_name_tool -change /usr/local/lib/libFLAC.7.dylib @executable_path/../Frameworks/libFLAC.dylib hydrogen.app/Contents/MacOS/hydrogen

#echo " *** Processing libFLAC++"
#cp -L /usr/lib/libFLAC++.dylib hydrogen.app/Contents/Frameworks/libFLAC++.dylib
#install_name_tool -id @executable_path/../Frameworks/libFLAC++.dylib hydrogen.app/Contents/Frameworks/libFLAC++.dylib
#install_name_tool -change /usr/local/lib/libFLAC++.5.dylib @executable_path/../Frameworks/libFLAC++.dylib hydrogen.app/Contents/MacOS/hydrogen
#install_name_tool -change /usr/local/lib/libFLAC.7.dylib @executable_path/../Frameworks/libFLAC.dylib hydrogen.app/Contents/Frameworks/libFLAC++.dylib

#echo " *** Processing QT Gui lib"
#cp -L $QTDIR/lib/libQtGui.4.dylib hydrogen.app/Contents/Frameworks/libQtGui.dylib
#install_name_tool -id @executable_path/../Frameworks/libQtGui.dylib hydrogen.app/Contents/Frameworks/libQtGui.dylib
#install_name_tool -change $QTDIR/lib/libQtGui.4.dylib @executable_path/../Frameworks/libQtGui.dylib hydrogen.app/Contents/MacOS/hydrogen
#install_name_tool -change $QTDIR/lib/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/Frameworks/libQtGui.dylib

#echo " *** Processing QT Network lib"
#cp -L $QTDIR/lib/libQtNetwork.4.dylib hydrogen.app/Contents/Frameworks/libQtNetwork.dylib
#install_name_tool -id @executable_path/../Frameworks/libQtNetwork.dylib hydrogen.app/Contents/Frameworks/libQtNetwork.dylib
#install_name_tool -change $QTDIR/lib/libQtNetwork.4.dylib @executable_path/../Frameworks/libQtNetwork.dylib hydrogen.app/Contents/MacOS/hydrogen
#install_name_tool -change $QTDIR/lib/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/Frameworks/libQtNetwork.dylib

#echo " *** Processing QT Core lib"
#cp -L $QTDIR/lib/libQtCore.4.dylib hydrogen.app/Contents/Frameworks/libQtCore.dylib
#install_name_tool -id @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/Frameworks/libQtCore.dylib
#install_name_tool -change $QTDIR/lib/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/MacOS/hydrogen

#echo " *** Processing QT SVG lib"
#cp -L $QTDIR/lib/libQtSvg.4.dylib hydrogen.app/Contents/Frameworks/libQtSvg.dylib
#install_name_tool -id @executable_path/../Frameworks/libQtSvg.dylib hydrogen.app/Contents/Frameworks/libQtSvg.dylib
#install_name_tool -change $QTDIR/lib/libQtSvg.4.dylib @executable_path/../Frameworks/libQtSvg.dylib hydrogen.app/Contents/MacOS/hydrogen
#install_name_tool -change $QTDIR/lib/libQtGui.4.dylib @executable_path/../Frameworks/libQtGui.dylib hydrogen.app/Contents/Frameworks/libQtSvg.dylib
#install_name_tool -change $QTDIR/lib/libQtXml.4.dylib @executable_path/../Frameworks/libQtXml.dylib hydrogen.app/Contents/Frameworks/libQtSvg.dylib
#install_name_tool -change $QTDIR/lib/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/Frameworks/libQtSvg.dylib

#echo " *** Processing QT XML lib"
#cp -L $QTDIR/lib/libQtXml.4.dylib hydrogen.app/Contents/Frameworks/libQtXml.dylib
#install_name_tool -id @executable_path/../Frameworks/libQtXml.dylib hydrogen.app/Contents/Frameworks/libQtXml.dylib
#install_name_tool -change $QTDIR/lib/libQtXml.4.dylib @executable_path/../Frameworks/libQtXml.dylib hydrogen.app/Contents/MacOS/hydrogen
#install_name_tool -change $QTDIR/lib/libQtCore.4.dylib @executable_path/../Frameworks/libQtCore.dylib hydrogen.app/Contents/Frameworks/libQtXml.dylib

# strip the executable

strip hydrogen.app/Contents/MacOS/hydrogen

# just for info...

echo " *** Library dependencies"
otool -L hydrogen.app/Contents/MacOS/hydrogen
du -h hydrogen.app

echo ""
echo " *** Finished"
