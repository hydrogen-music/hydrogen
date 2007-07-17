include(features.pri){    DISTFILES += FAQ.txt

    HEADERS += docs/docs.h

}

TEMPLATE = subdirs
SUBDIRS = plugins libs extra gui
