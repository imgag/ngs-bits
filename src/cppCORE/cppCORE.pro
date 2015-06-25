#-------------------------------------------------
#
# Project created by QtCreator 2013-07-31T22:18:26
#
#-------------------------------------------------

#base settings
QT       -= gui
TEMPLATE = lib
TARGET = cppCORE
DEFINES += CPPCORE_LIBRARY

#compose version string
SVN_VER= $$system(git describe --tags )
DEFINES += "CPPCORE_VERSION=$$SVN_VER"

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#copy DLL to bin folder
DESTDIR = ../../bin/

SOURCES += \
    Exceptions.cpp \
    Settings.cpp \
    Log.cpp \
    Helper.cpp \
    BasicStatistics.cpp \
    FileWatcher.cpp \
    LinePlot.cpp \
    WorkerBase.cpp \
    ToolBase.cpp \
    TSVFileStream.cpp

HEADERS += ToolBase.h \
    Exceptions.h \
    Settings.h \
    Log.h \
    Helper.h \
    BasicStatistics.h \
    FileWatcher.h \
    LinePlot.h \
    WorkerBase.h \
    TSVFileStream.h
