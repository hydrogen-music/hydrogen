#message( --==( Building libhydrogen )=-- )
include(../../features.pri)

TEMPLATE = lib
CONFIG += qt warn_on thread staticlib precompile_header
QMAKE_CXXFLAGS_RELEASE += -g -Wall
QMAKE_CXXFLAGS_DEBUG += -g -Wall


DESTDIR = ..
INCLUDEPATH += ../.. include
OBJECTS_DIR = objs

win32 {
	INCLUDEPATH += ../../win32build/includes
	INCLUDEPATH += ../../win32build/libs/libpthread
	INCLUDEPATH += ../../win32build/libs/libsndfile
	INCLUDEPATH += ../../win32build/libs/flac
	INCLUDEPATH += ../../win32build/libs/portaudio
	INCLUDEPATH += ../../win32build/libs/portmidi
}

macx {
	# enable universal lib creation
	#CONFIG += x86 ppc

        INCLUDEPATH += /System/Library/Frameworks/Carbon.framework/Headers
        INCLUDEPATH += /opt/local/include
        QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
}

linux-g++ {
	#QMAKE_CXXFLAGS_RELEASE += -fno-stack-protector
	#QMAKE_CXXFLAGS_DEBUG += -fno-stack-protector
}

PRECOMPILED_HEADER  = src/Precompiled.h

DEFINES += $$H2DEFINES
message( H2 defines: $$H2DEFINES )

HEADERS += \
		src/Precompiled.h \
		\
		include/hydrogen/adsr.h \
		include/hydrogen/audio_engine.h \
		include/hydrogen/note.h \
		\
		include/hydrogen/Object.h \
		include/hydrogen/hydrogen.h \
		include/hydrogen/LocalFileMng.h \
		include/hydrogen/Preferences.h \
		include/hydrogen/Song.h \
		include/hydrogen/Globals.h \
		include/hydrogen/Instrument.h \
		include/hydrogen/Pattern.h \
		include/hydrogen/Sample.h \
		include/hydrogen/DataPath.h \
		include/hydrogen/EventQueue.h \
		include/hydrogen/SoundLibrary.h \
		include/hydrogen/H2Exception.h \
		include/hydrogen/fx/Effects.h \
		include/hydrogen/fx/LadspaFX.h \
		include/hydrogen/fx/ladspa.h \
		include/hydrogen/IO/AudioOutput.h \
		include/hydrogen/IO/TransportInfo.h \
		include/hydrogen/IO/MidiInput.h \
		include/hydrogen/IO/CoreMidiDriver.h \
		include/hydrogen/IO/JackOutput.h \
		include/hydrogen/IO/NullDriver.h \
		include/hydrogen/sampler/Sampler.h \
		include/hydrogen/sequencer/Sequencer.h \
		include/hydrogen/synth/Synth.h \
		include/hydrogen/smf/SMF.h \
		include/hydrogen/smf/SMFEvent.h \
		\
		\
		src/xml/tinystr.h \
		src/xml/tinyxml.h \
		\
		src/IO/AlsaMidiDriver.h \
		src/IO/DiskWriterDriver.h \
		src/IO/FakeDriver.h \
		src/IO/OssDriver.h \
		src/IO/AlsaAudioDriver.h \
		src/IO/PortMidiDriver.h \
		src/IO/PortAudioDriver.h \
		src/IO/CoreAudioDriver.h \
		\
		\
		\
		src/FLACFile.h \




SOURCES += \
		src/xml/tinystr.cpp \
		src/xml/tinyxml.cpp \
		src/xml/tinyxmlerror.cpp \
		src/xml/tinyxmlparser.cpp \
		\
		src/IO/AlsaMidiDriver.cpp \
		src/IO/DiskWriterDriver.cpp \
		src/IO/FakeDriver.cpp \
		src/IO/JackOutput.cpp \
		src/IO/NullDriver.cpp \
		src/IO/OssDriver.cpp \
		src/IO/TransportInfo.cpp \
		src/IO/AlsaAudioDriver.cpp \
		src/IO/MidiInput.cpp \
		src/IO/PortMidiDriver.cpp \
		src/IO/PortAudioDriver.cpp \
		src/IO/CoreAudioDriver.cpp \
		src/IO/CoreMidiDriver.cpp \
		\
		src/fx/Effects.cpp \
		src/fx/LadspaFX.cpp \
		\
		src/smf/SMF.cpp \
		src/smf/SMFEvent.cpp \
		\
		src/sampler/Sampler.cpp \
		\
		src/sequencer/Sequencer.cpp \
		\
		src/synth/Synth.cpp \
		\
		src/adsr.cpp \
		src/audio_engine.cpp \
		src/DataPath.cpp \
		src/EventQueue.cpp \
		src/FLACFile.cpp \
		src/Hydrogen.cpp \
		src/Instrument.cpp \
		src/LocalFileMng.cpp \
		src/note.cpp \
		src/Object.cpp \
		src/Pattern.cpp \
		src/Preferences.cpp \
		src/Sample.cpp \
		src/Song.cpp \
		src/SoundLibrary.cpp
