#-------------------------------------------------
#
# Project created by QtCreator 2015-03-24T09:09:18
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
        WriteWorker.cpp

include("../app_cli.pri")

#include zlib library (inlined zlib functions make this necessary for each tool that uses FastqFileStream)
LIBS += -lz

HEADERS += \
    WriteWorker.h
