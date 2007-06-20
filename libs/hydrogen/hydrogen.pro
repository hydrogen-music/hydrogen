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

PRECOMPILED_HEADER  = src/precompiled.h

DEFINES += $$H2DEFINES
message( H2 defines: $$H2DEFINES )

HEADERS += \
		src/precompiled.h \
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
		include/hydrogen/globals.h \
		include/hydrogen/instrument.h \
		include/hydrogen/Pattern.h \
		include/hydrogen/Sample.h \
		include/hydrogen/data_path.h \
		include/hydrogen/event_queue.h \
		include/hydrogen/SoundLibrary.h \
		include/hydrogen/H2Exception.h \
		\
		include/hydrogen/fx/Effects.h \
		include/hydrogen/fx/LadspaFX.h \
		include/hydrogen/fx/ladspa.h \
		\
		include/hydrogen/IO/AudioOutput.h \
		include/hydrogen/IO/TransportInfo.h \
		include/hydrogen/IO/MidiInput.h \
		include/hydrogen/IO/CoreMidiDriver.h \
		include/hydrogen/IO/JackOutput.h \
		include/hydrogen/IO/NullDriver.h \
		\
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
		src/flac_file.h \




SOURCES += \
		src/xml/tinystr.cpp \
		src/xml/tinyxml.cpp \
		src/xml/tinyxmlerror.cpp \
		src/xml/tinyxmlparser.cpp \
		\
		src/IO/alsa_midi_driver.cpp \
		src/IO/disk_writer_driver.cpp \
		src/IO/fake_driver.cpp \
		src/IO/jack_output.cpp \
		src/IO/null_driver.cpp \
		src/IO/oss_driver.cpp \
		src/IO/transport_info.cpp \
		src/IO/alsa_audio_driver.cpp \
		src/IO/midi_input.cpp \
		src/IO/portmidi_driver.cpp \
		src/IO/portaudio_driver.cpp \
		src/IO/coreaudio_driver.cpp \
		src/IO/coremidi_driver.cpp \
		\
		src/fx/effects.cpp \
		src/fx/ladspa_fx.cpp \
		\
		src/smf/smf.cpp \
		src/smf/smf_event.cpp \
		\
		src/sampler/sampler.cpp \
		\
		src/sequencer/sequencer.cpp \
		\
		src/synth/synth.cpp \
		\
		src/adsr.cpp \
		src/audio_engine.cpp \
		src/data_path.cpp \
		src/event_queue.cpp \
		src/flac_file.cpp \
		src/hydrogen.cpp \
		src/instrument.cpp \
		src/local_file_mgr.cpp \
		src/note.cpp \
		src/object.cpp \
		src/pattern.cpp \
		src/preferences.cpp \
		src/sample.cpp \
		src/song.cpp \
		src/sound_library.cpp
