#-------------------------------------------------
#
# Project created by QtCreator 2016-01-28T13:10:11
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
