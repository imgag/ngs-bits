TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle



HEADERS = VcfToBedpe.h

SOURCES = VcfToBedpe.cpp \
            main.cpp
include("../app_cli.pri")
