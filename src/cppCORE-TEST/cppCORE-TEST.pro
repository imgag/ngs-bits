#-------------------------------------------------
#
# Project created by QtCreator 2013-08-03T23:23:16
#
#-------------------------------------------------

QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += \
	Helper_Test.h \
	BasicStatistics_Test.h \
    TSVFileStream_Test.h

SOURCES += \
	main.cpp

include("../app_cli.pri")

