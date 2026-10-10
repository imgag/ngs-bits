TEMPLATE = app
TARGET = cppVISUAL-TEST
QT += widgets xml network testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle
DEFINES += CPPVISUAL_LIBRARY CPPGUI_LIBRARY
INCLUDEPATH += ../cppVISUAL ../cppGUI ../cppCORE ../cppNGS ../cppXML ../../htslib/include ../../libxml2/include
LIBS += -L$$PWD/../../bin -lcppGUI -lcppNGS -lcppXML -lcppCORE
LIBS += -L$$PWD/../../htslib/lib -lhts

SOURCES += GenomeVisualizationWidget_Test.cpp $$files(../cppVISUAL/*.cpp) ../cppGUI/ParameterEditor.cpp
HEADERS += $$files(../cppVISUAL/*.h) ../cppGUI/ParameterEditor.h
FORMS += ../cppVISUAL/GenomeVisualizationWidget.ui ../cppGUI/ParameterEditor.ui
RESOURCES += ../cppVISUAL/cppVISUAL.qrc
