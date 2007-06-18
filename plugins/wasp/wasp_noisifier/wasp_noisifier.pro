TEMPLATE = lib
QT -= qt
QT -= gui
QT -= core
DESTDIR = ../../
INCLUDEPATH += ../include
SOURCES += noisifier.c

linux-g++* {
	CONFIG = plugin
	
	# there's something weird in linux qmake...this is a simple and ugly workaround
	QMAKE_LINK_SHLIB_CMD = ld -shared -o libwasp_noisifier.so noisifier.o
}

win32 {
	CONFIG = dll
}