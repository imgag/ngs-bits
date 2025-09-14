include("../lib.pri")

#base settings
QT       += gui widgets xml network
TARGET = cppVISUAL
DEFINES += CPPVISUAL_LIBRARY

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppGUI library
INCLUDEPATH += $$PWD/../cppGUI
LIBS += -L$$PWD/../../bin -lcppGUI

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += \
    GenomeVisualizationWidget.cpp \
    GenePanel.cpp

HEADERS += \
    GenomeVisualizationWidget.h \
    GenePanel.h

FORMS += \
    GenomeVisualizationWidget.ui
