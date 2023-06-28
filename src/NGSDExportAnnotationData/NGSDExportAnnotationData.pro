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

SOURCES += main.cpp \
    Auxilary.cpp \
    ThreadCoordinator.cpp \
    ExportWorker.cpp


HEADERS += \
    Auxilary.h \
    ExportWorker.h \
    ThreadCoordinator.h


include("../app_cli.pri")


#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../bin -lcppNGSD

