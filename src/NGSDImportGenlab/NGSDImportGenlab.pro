#-------------------------------------------------
#
# Project created by QtCreator 2013-08-02T13:54:23
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

RESOURCES += \
    resources.qrc
