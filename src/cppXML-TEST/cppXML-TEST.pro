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
DESTDIR = ../../../../bin/

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include XML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

HEADERS += \
	XmlValidation_Test.h

SOURCES += \
        main.cpp

RESOURCES += \
    cppXML-TEST.qrc

