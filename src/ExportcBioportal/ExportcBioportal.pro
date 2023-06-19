#-------------------------------------------------
#
# Project created by QtCreator 2013-08-02T13:54:23
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp

include("../app_cli.pri")

#include zlib library (inlined zlib functions make this necessary for each tool that uses FastqFileStream)
LIBS += -lz
