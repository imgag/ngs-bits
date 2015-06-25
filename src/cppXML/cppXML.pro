#-------------------------------------------------
#
# Project created by QtCreator 2013-07-31T22:18:26
#
#-------------------------------------------------

#base settings
QT       -= gui
QT       += gui xml xmlpatterns
TEMPLATE = lib
TARGET = cppXML
DEFINES += CPPXML_LIBRARY

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

HEADERS += XMLHelper.h \

SOURCES += XMLHelper.cpp \

