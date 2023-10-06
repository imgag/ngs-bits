
TEMPLATE = app
QT       -= gui
CONFIG   += c++11 console
CONFIG   -= app_bundle

SOURCES += main.cpp
include("../app_cli.pri")

#include zlib library (inlined zlib functions make this necessary for each tool that uses FastqFileStream)
LIBS += -lz
