TEMPLATE = app
TARGET = cppGUI-TEST
QT += widgets testlib
CONFIG += console testcase c++17
CONFIG -= app_bundle
DEFINES += CPPGUI_LIBRARY
INCLUDEPATH += ../cppGUI ../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

SOURCES += ParameterEditor_Test.cpp ../cppGUI/ParameterEditor.cpp
HEADERS += ../cppGUI/ParameterEditor.h
FORMS += ../cppGUI/ParameterEditor.ui
