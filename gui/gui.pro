include(../features.pri)

TARGET = hydrogen
INCLUDEPATH += ../ ../libs/hydrogen/include
DESTDIR = ..

QT += network xml

CONFIG += qt warn_on precompile_header release
PRECOMPILED_HEADER  = src/Precompiled.h
LIBS += ../libs/libhydrogen.a

OBJECTS_DIR = objs
UI_DIR = objs
UI_HEADERS_DIR = objs
UI_SOURCES_DIR = objs
MOC_DIR = objs

PRE_TARGETDEPS = ../libs/libhydrogen.a


exists(/usr/bin/doxygen) {
#	message("Doxygen is available.")
#	# Crea la documentazione con Doxygen
#	doxygen.target = ../docs/html/dummy
#	doxygen.commands = cd ../docs; doxygen
#	QMAKE_EXTRA_TARGETS += doxygen
#	POST_TARGETDEPS = ../docs/html/dummy
}




# rebuild the translations QM files
#QMAKE_PRE_LINK = $$[QT_INSTALL_PREFIX]/bin/lupdate gui.pro; $$[QT_INSTALL_PREFIX]/bin/lrelease gui.pro

linux-g++ {
	message( *** LINUX BUILD *** )
	LIBS += -lsndfile
	LIBS += -ltar
#	QMAKE_CXXFLAGS_RELEASE += -fno-stack-protector
#	QMAKE_CXXFLAGS_DEBUG += -fno-stack-protector
}
linux-g++-64 {
	message( *** LINUX 64bit BUILD *** )
	LIBS += -lsndfile
	LIBS += -ltar
	QMAKE_CXXFLAGS_RELEASE += -fno-stack-protector
	QMAKE_CXXFLAGS_DEBUG += -fno-stack-protector
}

win32 {
	message( *** WIN32 BUILD *** )
	INCLUDEPATH += ../win32build/includes
	INCLUDEPATH += ../win32build/libs/libpthread
	INCLUDEPATH += ../win32build/libs/libsndfile
	INCLUDEPATH += ../win32build/libs/flac

	LIBS += ../win32build/libs/libsndfile/libsndfile.a
	LIBS += ../win32build/libs/libpthread/libpthreadGC1.a
	LIBS += ../win32build/libs/portaudio/libportaudio.a
	LIBS += ../win32build/libs/portmidi/libporttime.a
	LIBS += ../win32build/libs/portmidi/libportmidi.a
	LIBS += -lwinmm
}

macx-g++ {
	message( *** MAC BUILD *** )
	#CONFIG += x86 ppc

	ICON = ../macos/Hydrogen.icns
	LIBS += -L/opt/local/lib
	LIBS += -lsndfile
	LIBS += -ltar
	INCLUDEPATH += /System/Library/Frameworks/Carbon.framework/Headers
	QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
	QMAKE_POST_LINK = cd ..;macos/fixlibs.sh
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


# installs the hydrogen executable
target.path = $$prefix/bin
# data and docs
documentation.path = $$prefix/share/hydrogen/data
documentation.files = ../data/*
INSTALLS += target documentation



TRANSLATIONS = \
	../data/i18n/hydrogen.de.ts \
	../data/i18n/hydrogen.fr.ts \
	../data/i18n/hydrogen.it.ts \
	../data/i18n/hydrogen.nl.ts \
	../data/i18n/hydrogen.pt_BR.ts \
	../data/i18n/hydrogen.sv.ts \
	../data/i18n/hydrogen.es.ts \
	../data/i18n/hydrogen.hu_HU.ts \
	../data/i18n/hydrogen.ja.ts \
	../data/i18n/hydrogen.pl.ts \
	../data/i18n/hydrogen.ru.ts

FORMS    = \
	src/UI/AboutDialog_UI.ui \
	src/UI/AudioEngineInfoForm_UI.ui \
	src/UI/DrumkitManager_UI.ui \
	src/UI/ExportSongDialog_UI.ui \
	src/UI/LadspaFXSelector_UI.ui \
	src/UI/PatternFillDialog_UI.ui \
	src/UI/PatternPropertiesDialog_UI.ui \
	src/UI/PreferencesDialog_UI.ui \
	src/UI/SongPropertiesDialog_UI.ui \
	src/SoundLibrary/SoundLibraryImportDialog_UI.ui \
	src/SoundLibrary/SoundLibrarySaveDialog_UI.ui



SOURCES += \
	src/widgets/Button.cpp \
	src/widgets/CpuLoadWidget.cpp \
	src/widgets/ClickableLabel.cpp \
	src/widgets/Fader.cpp \
	src/widgets/LCD.cpp \
	src/widgets/MidiActivityWidget.cpp \
	src/widgets/PixmapWidget.cpp \
	src/widgets/Rotary.cpp \
	src/widgets/DownloadWidget.cpp \
	src/widgets/LCDCombo.cpp \
	\
	src/SoundLibrary/SoundLibraryTree.cpp\
	src/SoundLibrary/SoundLibraryPanel.cpp\
	src/SoundLibrary/FileBrowser.cpp\
	src/SoundLibrary/SoundLibraryImportDialog.cpp\
	src/SoundLibrary/SoundLibrarySaveDialog.cpp\
	\
	src/InstrumentEditor/InstrumentEditor.cpp \
	src/InstrumentEditor/InstrumentEditorPanel.cpp \
	src/InstrumentEditor/WaveDisplay.cpp \
	src/InstrumentEditor/LayerPreview.cpp \
	\
	src/SongEditor/SongEditor.cpp \
	src/SongEditor/SongEditorPanel.cpp \
	\
	src/PatternEditor/DrumPatternEditor.cpp \
	src/PatternEditor/PatternEditorRuler.cpp \
	src/PatternEditor/PatternEditorInstrumentList.cpp \
	src/PatternEditor/PatternEditorPanel.cpp \
	src/PatternEditor/NotePropertiesRuler.cpp \
	src/PatternEditor/PianoRollEditor.cpp \
	\
	src/Mixer/Mixer.cpp \
	src/Mixer/MixerLine.cpp \
	\
	src/AboutDialog.cpp \
	src/AudioEngineInfoForm.cpp \
	src/DrumkitManager.cpp \
	src/ExportSongDialog.cpp \
	src/HelpBrowser.cpp \
	src/HydrogenApp.cpp \
	src/InstrumentRack.cpp \
	src/LadspaFXProperties.cpp \
	src/LadspaFXSelector.cpp \
	src/MainForm.cpp \
	src/PatternFillDialog.cpp \
	src/PatternPropertiesDialog.cpp \
	src/PlayerControl.cpp \
	src/PreferencesDialog.cpp \
	src/SongPropertiesDialog.cpp \
	src/SplashScreen.cpp \
	src/main.cpp


HEADERS += \
	src/widgets/Button.h \
	src/widgets/CpuLoadWidget.h \
	src/widgets/ClickableLabel.h \
	src/widgets/Fader.h \
	src/widgets/LCD.h \
	src/widgets/MidiActivityWidget.h \
	src/widgets/PixmapWidget.h \
	src/widgets/Rotary.h \
	src/widgets/DownloadWidget.h \
	src/widgets/LCDCombo.h \
	\
	src/SoundLibrary/SoundLibraryTree.h \
	src/SoundLibrary/SoundLibraryPanel.h \
	src/SoundLibrary/FileBrowser.h \
	src/SoundLibrary/SoundLibraryImportDialog.h\
	src/SoundLibrary/SoundLibrarySaveDialog.h\
	\
	src/InstrumentEditor/InstrumentEditor.h \
	src/InstrumentEditor/InstrumentEditorPanel.h \
	src/InstrumentEditor/WaveDisplay.h \
	src/InstrumentEditor/LayerPreview.h \
	\
	src/SongEditor/SongEditor.h \
	src/SongEditor/SongEditorPanel.h \
	\
	src/PatternEditor/DrumPatternEditor.h \
	src/PatternEditor/PatternEditorRuler.h \
	src/PatternEditor/PatternEditorInstrumentList.h \
	src/PatternEditor/PatternEditorPanel.h \
	src/PatternEditor/NotePropertiesRuler.h \
	src/PatternEditor/PianoRollEditor.h \
	\
	src/Mixer/Mixer.h \
	src/Mixer/MixerLine.h \
	\
	src/AboutDialog.h \
	src/AudioEngineInfoForm.h \
	src/DrumkitManager.h \
	src/ExportSongDialog.h \
	src/EventListener.h \
	src/HelpBrowser.h \
	src/HydrogenApp.h \
	src/InstrumentRack.h \
	src/LadspaFXProperties.h \
	src/LadspaFXSelector.h \
	src/MainForm.h \
	src/PatternFillDialog.h \
	src/PatternPropertiesDialog.h \
	src/PlayerControl.h \
	src/PreferencesDialog.h \
	src/Skin.h \
	src/SongPropertiesDialog.h \
	src/SplashScreen.h


