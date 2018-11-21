#-------------------------------------------------
#
# Project created by QtCreator 2013-10-08T13:40:57
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp

include("../app_cli.pri")

#include edlib library
INCLUDEPATH += $$PWD/../../edlib/edlib/include/
LIBS += -L$$PWD/../../edlib/lib/ -ledlib_static
