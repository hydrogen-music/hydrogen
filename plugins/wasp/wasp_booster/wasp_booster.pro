TEMPLATE = lib
QT -= qt
QT -= gui
QT -= core
DESTDIR = ../../
INCLUDEPATH += ../include
SOURCES += booster.c
QMAKE_LFLAGS_PLUGIN += -nostartfiles

linux-g++* {
	CONFIG = plugin

	# there's something weird in linux qmake...this is a simple and ugly workaround
        #QMAKE_LINK_SHLIB_CMD = gcc -nostartfiles -lm -shared booster.o -o libwasp_booster.so
}

win32 {
	CONFIG = dll
}
