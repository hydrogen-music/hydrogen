INCLUDEPATH += . src

TEMPLATE = app
CONFIG  += qt thread warn_off release
LANGUAGE = C++
OBJECTS_DIR=src
LIBS +=  -lsndfile   -lFLAC++ -lFLAC -llash -ljack -lasound -llrdf -lraptor -lxml2
QMAKE_CFLAGS+=
QMAKE_CXXFLAGS+= -I/usr/include/lash-1.0 -g
QMAKE_LFLAGS+=

SOURCES += \
		src/lib/xml/tinystr.cpp \
		src/lib/xml/tinyxml.cpp \
		src/lib/xml/tinyxmlerror.cpp \
		src/lib/xml/tinyxmlparser.cpp \
		\
		src/lib/lash/LashClient.cpp \
		\
		src/lib/drivers/AlsaMidiDriver.cpp \
		src/lib/drivers/DiskWriterDriver.cpp \
		src/lib/drivers/FakeDriver.cpp \
		src/lib/drivers/JackDriver.cpp \
		src/lib/drivers/NullDriver.cpp \
		src/lib/drivers/OssDriver.cpp \
		src/lib/drivers/TransportInfo.cpp \
		src/lib/drivers/AlsaAudioDriver.cpp \
		src/lib/drivers/MidiDriver.cpp \
		src/lib/drivers/PortMidiDriver.cpp \
		src/lib/drivers/PortAudioDriver.cpp \
		\
		src/lib/fx/LadspaFX.cpp \
		\
		src/lib/smf/SMF.cpp \
		src/lib/smf/SMFEvent.cpp \
		\
		src/lib/ADSR.cpp \
		src/lib/DataPath.cpp \
		src/lib/EventQueue.cpp \
		src/lib/FLACFile.cpp \
		src/lib/Hydrogen.cpp \
		src/lib/LocalFileMng.cpp \
		src/lib/Object.cpp \
		src/lib/Preferences.cpp \
		src/lib/Sample.cpp \
		src/lib/Song.cpp \
		\
		src/tools/HydrogenPlayer.cpp


HEADERS += \
		config.h \
		\
		src/lib/xml/tinystr.h \
		src/lib/xml/tinyxml.h \
		\
		src/lib/lash/LashClient.h \
		\
		src/lib/drivers/AlsaMidiDriver.h \
		src/lib/drivers/DiskWriterDriver.h \
		src/lib/drivers/GenericDriver.h \
		src/lib/drivers/FakeDriver.h \
		src/lib/drivers/JackDriver.h \
		src/lib/drivers/NullDriver.h \
		src/lib/drivers/OssDriver.h \
		src/lib/drivers/TransportInfo.h \
		src/lib/drivers/AlsaAudioDriver.h \
		src/lib/drivers/MidiDriver.h \
		src/lib/drivers/PortMidiDriver.h \
		src/lib/drivers/PortAudioDriver.h \
		\
		src/lib/fx/LadspaFX.h \
		src/lib/fx/ladspa.h \
		\
		src/lib/smf/SMF.h \
		src/lib/smf/SMFEvent.h \
		\
		src/lib/ADSR.h \
		src/lib/EventQueue.h \
		src/lib/Exception.h \ 
		src/lib/FLACFile.h \
		src/lib/Globals.h \
		src/lib/Hydrogen.h \
		src/lib/LocalFileMng.h \
		src/lib/Object.h \
		src/lib/Preferences.h \
		src/lib/Sample.h \
		src/lib/Song.h

