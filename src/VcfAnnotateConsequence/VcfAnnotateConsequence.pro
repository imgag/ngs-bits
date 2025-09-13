include("../app_cli.pri")

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


