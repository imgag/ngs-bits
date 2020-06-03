#-------------------------------------------------
#
# Project created by QtCreator 2020-05-13T16:06:06
#
#-------------------------------------------------

TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
    ShallowVariantCallerFunctions.cpp

include("../app_cli.pri")

HEADERS += \
    forward.h \
    ShallowVariantCallerFunctions.h
