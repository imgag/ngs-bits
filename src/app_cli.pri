include("base.pri")

#base settings
QT += network
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#include cppCORE library
INCLUDEPATH += $$PWD/cppCORE
LIBS += -L$$PWD/../bin -lcppCORE

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/cppNGS
LIBS += -L$$PWD/../bin -lcppNGS

#include htslib library
INCLUDEPATH += $$PWD/../htslib/include/
LIBS += -L$$PWD/../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../libxml2/include/
win32: LIBS += -L$$PWD/../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2
