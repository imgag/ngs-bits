include("../app_cli.pri")

SOURCES += main.cpp \
    AnalysisWorker.cpp \
    OutputWorker.cpp \
    ThreadCoordinator.cpp \
    InputWorker.cpp \
    FastqWriter.cpp

HEADERS += \
    AnalysisWorker.h \
    Auxilary.h \
    OutputWorker.h \
    ThreadCoordinator.h \
    InputWorker.h \
    FastqWriter.h

