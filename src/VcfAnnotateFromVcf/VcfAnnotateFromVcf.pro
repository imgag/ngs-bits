#-------------------------------------------------
#
# Project created by QtCreator 2013-10-08T13:40:57
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
    ChunkProcessor.cpp \
    OutputWorker.cpp

include("../app_cli.pri")

HEADERS += \
    Auxilary.h \
    ChunkProcessor.h \
    OutputWorker.h
