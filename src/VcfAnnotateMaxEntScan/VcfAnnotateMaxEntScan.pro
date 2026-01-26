include("../app_cli.pri")

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

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
