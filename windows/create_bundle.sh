#!/bin/bash
LIB_DIR=libs
if [ ! -e $BUNDLE_DIR ]; then
	mkdir $BUNDLE_DIR
fi

cp $HYDROGEN_BUILD/src/gui/hydrogen.exe .
cp $HYDROGEN_BUILD/src/core/libhydrogen-core-0.9.7.dll .
cp $HYDROGEN_BUILD/src/cli/h2cli.exe .
cp $HYDROGEN_BUILD/src/player/h2player.exe .
cp $HYDROGEN_BUILD/src/synth/h2synth.exe .

cd $BUNDLE_DIR
cp $MXE/usr/i686-w64-mingw32.shared/qt/bin/QtCore4.dll .
cp $MXE/usr/i686-w64-mingw32.shared/qt/bin/QtXml4.dll .
cp $MXE/usr/i686-w64-mingw32.shared/qt/bin/QtXmlPatterns4.dll .
cp $MXE/usr/i686-w64-mingw32.shared/qt/bin/QtNetwork4.dll .
cp $MXE/usr/i686-w64-mingw32.shared/qt/bin/QtGui4.dll .

cp $MXE/usr/lib/gcc/i686-w64-mingw32.shared/5.1.0/libgcc_s_sjlj-1.dll .
cp $MXE/usr/lib/gcc/i686-w64-mingw32.shared/5.1.0/libstdc++-6.dll .

cp $MXE/usr/i686-w64-mingw32.shared/bin/libsndfile-1.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libFLAC-8.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libogg-0.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libvorbis-0.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libvorbisenc-2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/zlib1.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libwinpthread-1.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libeay32.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/ssleay32.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libarchive-13.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libbz2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/liblzma-5.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libnettle-4-6.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libxml2-2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libpng16-16.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libportmidi.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libportaudio-2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libiconv-2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libiconv-2.dll .
cp $MXE/usr/i686-w64-mingw32.shared/bin/libjack.dll .

if [ ! -e rubberband ]; then
	mkdir rubberband
fi
cp $MXE/usr/i686-w64-mingw32.shared/bin/libfftw3-3.dll rubberband/

#cp -r $HYDROGEN/data .