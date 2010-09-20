# Check if compiling with MSVC
contains(QMAKE_CC, cl)
:CONFIG += msvc
macx-g++ { 
    LIBS += -framework \
        AudioUnit \
        -framework \
        AudioToolbox \
        -framework \
        CoreServices \
        -framework \
        CoreAudio \
        -framework \
        CoreMidi
    H2DEFINES += H2CORE_HAVE_LADSPA
    H2DEFINES += H2CORE_HAVE_FLAC
    H2DEFINES += H2CORE_HAVE_JACK
    H2DEFINES += H2CORE_HAVE_COREAUDIO
    H2DEFINES += H2CORE_HAVE_COREMIDI
    LIBS += /opt/local/lib/libFLAC.dylib
    LIBS += /opt/local/lib/libFLAC++.dylib
    LIBS += /usr/local/lib/libjack.dylib
    LIBS += /opt/local/lib/libtar.dylib
    LIBS += /opt/local/lib/libpng.dylib
    LIBS += /opt/local/lib/libsndfile.dylib
}
linux-g++ { 
    H2DEFINES += H2CORE_HAVE_ALSA
    H2DEFINES += H2CORE_HAVE_JACK
    H2DEFINES += H2CORE_HAVE_LASH
    H2DEFINES += H2CORE_HAVE_FLAC
    H2DEFINES += H2CORE_HAVE_LADSPA
    H2DEFINES += H2CORE_HAVE_LRDF
    H2DEFINES += H2CORE_HAVE_OSS
}
linux-g++-64 { 
    H2DEFINES += H2CORE_HAVE_ALSA
    H2DEFINES += H2CORE_HAVE_JACK
    H2DEFINES += H2CORE_HAVE_LASH
    H2DEFINES += H2CORE_HAVE_FLAC
    H2DEFINES += H2CORE_HAVE_LADSPA
    H2DEFINES += H2CORE_HAVE_LRDF
    H2DEFINES += H2CORE_HAVE_OSS
}
win32 { 
    # H2DEFINES += H2CORE_HAVE_FLAC
    # H2DEFINES += H2CORE_HAVE_LADSPA
    # H2DEFINES += PORTMIDI_SUPPOR
    H2DEFINES += H2CORE_HAVE_PORTAUDIO
    H2DEFINES += H2CORE_HAVE_LIBARCHIVE
}
HEADERS += 
SOURCES += ../../../../develop/stempeluhr-kompi/stempeluhr/stempelkartenerstellung.cpp
