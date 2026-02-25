include("../test.pri")

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppPLOTS library
INCLUDEPATH += $$PWD/../cppPLOTS
LIBS += -L$$PWD/../../bin -lcppPLOTS

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

SOURCES += \
        Plots_Test.cpp \
        main.cpp

