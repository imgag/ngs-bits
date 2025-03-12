#c++11 support
CONFIG += c++11

#base settings
QT += core
QT -= gui
QT += xml
QT += network

CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DESTDIR = ../../bin/

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include XML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../bin -lcppXML

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

HEADERS += \
	XmlValidation_Test.h

SOURCES += \
        main.cpp

RESOURCES += \
    cppXML-TEST.qrc

