#c++11 support
CONFIG += c++11

#base settings
QT       -= gui
QT       += network
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DESTDIR = ../../bin/

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

HEADERS += \
        Helper_Test.h \
        BasicStatistics_Test.h \
        TSVFileStream_Test.h \
        Settings_Test.h

SOURCES += \
        main.cpp
