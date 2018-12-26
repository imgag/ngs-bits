#c++11 and c++14 support
CONFIG += c++11 

#base settings
QT       += sql gui widgets
QTPLUGIN += QSQLMYSQL
TEMPLATE = lib
TARGET = cppNGSD
DEFINES += CPPNGSD_LIBRARY

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
INCLUDEPATH += $$PWD/../cppGUI
LIBS += -L$$PWD/../../bin -lcppGUI

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

SOURCES += \
    SqlQuery.cpp\
    NGSD.cpp \
    GenLabDB.cpp \
    GDBO.cpp \
    DBTable.cpp \
    DBTableWidget.cpp \
    ProcessedSampleWidget.cpp \
    DBQCWidget.cpp

HEADERS += \
    SqlQuery.h \
    NGSD.h \
    GenLabDB.h \
    GDBO.h \
    DBTable.h \
    DBTableWidget.h \
    ProcessedSampleWidget.h \
    DBQCWidget.h

FORMS += \
    ProcessedSampleWidget.ui \
    DBQCWidget.ui

RESOURCES += \
    cppNGSD.qrc
