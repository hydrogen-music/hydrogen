TEMPLATE = lib
QT -= qt
QT -= gui
QT -= core
DESTDIR = ../../
INCLUDEPATH += ../include
SOURCES += xshaper.c

linux-g++* {
	CONFIG = plugin
	# there's something weird in linux qmake...this is a simple and ugly workaround
	QMAKE_LINK_SHLIB_CMD = ld -shared -o libwasp_xshaper.so xshaper.o
}

win32 {
	CONFIG = dll
}