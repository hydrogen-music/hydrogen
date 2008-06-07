include(../features.pri)

TARGET = hyd_unittest
INCLUDEPATH += ../ ../libs/hydrogen/include
DESTDIR = ..

QT += network xml

CONFIG += qt warn_on precompile_header release
LIBS += ../libs/libhydrogen.a -lcppunit

PRE_TARGETDEPS = ../libs/libhydrogen.a


linux-g++ {
	message( *** LINUX BUILD *** )
	LIBS += -lsndfile
	LIBS += -ltar
#	QMAKE_CXXFLAGS_RELEASE += -fno-stack-protector
#	QMAKE_CXXFLAGS_DEBUG += -fno-stack-protector
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
		LIBS += ../win32build/libs/flac/libFLAC++.a
		LIBS += ../win32build/libs/flac/libFLAC.a
	}
	macx {
		LIBS += -lFLAC -lFLAC++
	}
}

# the executable will be built in debug mode
QMAKE_CXXFLAGS_RELEASE += -g -Wall
QMAKE_CXXFLAGS_DEBUG += -g -Wall


SOURCES += \
	main.cpp \
	instrument_test.cpp \
	note_test.cpp


#HEADERS += \


