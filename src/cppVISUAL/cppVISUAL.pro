#c++11 support
CONFIG += c++11 

#base settings
QT       += gui widgets
TEMPLATE = lib
TARGET = cppVISUAL
DEFINES += CPPVISUAL_LIBRARY

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#copy DLL to bin folder
DESTDIR = ../../bin/

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

SOURCES += \
    GenomeVisualizationWidget.cpp

HEADERS += \
    GenomeVisualizationWidget.h

FORMS += \
    GenomeVisualizationWidget.ui
