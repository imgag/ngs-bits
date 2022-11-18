TEMPLATE = app
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
    ChunkProcessor.cpp \
    OutputWorker.cpp \
    ThreadCoordinator.cpp \
    InputWorker.cpp

HEADERS += \
    Auxilary.h \
    ChunkProcessor.h \
    OutputWorker.h \
    ThreadCoordinator.h \
    InputWorker.h

include("../app_cli.pri")


