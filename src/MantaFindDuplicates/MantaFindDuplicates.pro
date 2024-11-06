#-------------------------------------------------
#
# Project created by QtCreator 2013-10-08T13:40:57
#
#-------------------------------------------------

TEMPLATE = app

QT       -= gui
QT       += sql
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp

include("../app_cli.pri")

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD
