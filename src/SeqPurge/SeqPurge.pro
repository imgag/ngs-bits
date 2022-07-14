#-------------------------------------------------
#
# Project created by QtCreator 2013-08-02T13:54:23
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
    AnalysisWorker.cpp \
    OutputWorker.cpp \
    ThreadCoordinator.cpp \
    InputWorker.cpp \
	FastqWriter.cpp

include("../app_cli.pri")

#include zlib library (inlined zlib functions make this necessary for each tool that uses FastqFileStream)
LIBS += -lz

HEADERS += \
    AnalysisWorker.h \
    Auxilary.h \
    OutputWorker.h \
    ThreadCoordinator.h \
    InputWorker.h \
	FastqWriter.h

