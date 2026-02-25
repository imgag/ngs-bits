include("../test.pri")

#include cppPLOTS library
INCLUDEPATH += $$PWD/../cppPLOTS
LIBS += -L$$PWD/../../bin -lcppPLOTS

SOURCES += \
        Plots_Test.cpp \
        main.cpp

