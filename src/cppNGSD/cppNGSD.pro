#c++11 and c++14 support
CONFIG += c++14

#base settings
QT       -= gui
QT       += sql
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

#include cppCORE library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

SOURCES += \
    SqlQuery.cpp\
    NGSD.cpp \
    GPD.cpp

HEADERS += \
    SqlQuery.h \
    NGSD.h \
    GPD.h
