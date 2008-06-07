include(../../features.pri)

TARGET = hydrogenSynth
INCLUDEPATH += ../../libs/hydrogen/include
QT += xml
CONFIG += qt warn_on precompile_header release
#PRECOMPILED_HEADER  = src/Precompiled.h
LIBS += ../../libs/libhydrogen.a


linux-g++ {
	message( LINUX BUILD )
	LIBS += -lsndfile
	LIBS += -ltar
}
linux-g++-64 {
	message( LINUX 64bit BUILD )
	LIBS += -lsndfile
	LIBS += -ltar
}

win32 {
	message( WIN32 BUILD )
	INCLUDEPATH += ../../win32build/includes
	INCLUDEPATH += ../../win32build/libs/libpthread
	INCLUDEPATH += ../../win32build/libs/libsndfile
	INCLUDEPATH += ../../win32build/libs/flac

	LIBS += ../../win32build/libs/libsndfile/libsndfile.a
	LIBS += ../../win32build/libs/libpthread/libpthreadGC1.a
	LIBS += ../../win32build/libs/portaudio/libportaudio.a
	LIBS += ../../win32build/libs/portmidi/libporttime.a
	LIBS += ../../win32build/libs/portmidi/libportmidi.a
	LIBS += -lwinmm
}

macx {
	message( MAC BUILD )
	LIBS += -L/opt/local/lib
	LIBS += -lsndfile
	LIBS += -ltar
	INCLUDEPATH += /System/Library/Frameworks/Carbon.framework/Headers
	QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
}


DEFINES += $$H2DEFINES
contains(H2DEFINES, LRDF_SUPPORT ){
	LIBS += -llrdf
}

contains(H2DEFINES, ALSA_SUPPORT ){
	LIBS += -lasound
}

contains(H2DEFINES, JACK_SUPPORT ) {
	LIBS += -ljack
}

contains(H2DEFINES, LASH_SUPPORT ) {
	LIBS += -llash
}

contains(H2DEFINES, FLAC_SUPPORT ) {
	linux-g++ {
		LIBS += -lFLAC -lFLAC++
	}
	linux-g++-64 {
		LIBS += -lFLAC -lFLAC++
	}
	win32 {
		LIBS += ../../win32build/libs/flac/libFLAC++.a
		LIBS += ../../win32build/libs/flac/libFLAC.a
	}
	macx {
		LIBS += -lFLAC -lFLAC++
	}
}

# the executable will be built in debug mode
QMAKE_CXXFLAGS += -g -Wall



SOURCES += \
	HydrogenSynth.cpp

