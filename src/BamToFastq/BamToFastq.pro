#-------------------------------------------------
#
# Project created by QtCreator 2015-11-18T09:30:22
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = BamToFastq
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    OutputWorker.cpp

include("../app_cli.pri")

HEADERS += \
    OutputWorker.h
