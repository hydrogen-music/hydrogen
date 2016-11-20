#!/bin/bash

DMG_ROOT=`mktemp -d`

cp -r "hydrogen.app" "$DMG_ROOT"
macdeployqt "$DMG_ROOT/hydrogen.app"
ln -s /Applications "$DMG_ROOT/Applications"
cp macos/DS_Store "$DMG_ROOT/.DS_Store"
hdiutil create -srcfolder "$DMG_ROOT" -volname "Hydrogen" -fs HFS+ -format UDZO hydrogen.dmg
rm -rf "$DMG_ROOT"
