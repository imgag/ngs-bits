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

SOURCES += \
    SqlQuery.cpp\
    NGSD.cpp \
    GDBO.cpp \
    DatabaseCache.cpp \
    GDBODialog.cpp \
    GBDOFKEdit.cpp \
    ProcessedSampleFKEdit.cpp

HEADERS += \
    SqlQuery.h \
    NGSD.h \
    GDBO.h \
    DatabaseCache.h \
    GDBODialog.h \
    GBDOFKEdit.h \
    ProcessedSampleFKEdit.h

FORMS += \
    GDBODialog.ui

RESOURCES += \
    resources.qrc
