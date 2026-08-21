include("../test.pri")

QT       += testlib gui widgets network sql xml printsupport charts svg
QTPLUGIN += QSQLMYSQL

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppXML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppGUI library
INCLUDEPATH += $$PWD/../cppGUI
LIBS += -L$$PWD/../../bin -lcppGUI

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include cppVISUAL library
INCLUDEPATH += $$PWD/../cppVISUAL
LIBS += -L$$PWD/../../bin -lcppVISUAL


#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#include GSvar
INCLUDEPATH += $$PWD/../GSvar

SOURCES += $$files($$PWD/../GSvar/*.cpp, true)
SOURCES -= $$PWD/../GSvar/main.cpp

HEADERS += $$files($$PROJECT_ROOT/src/GSvar/*.h, true)
FORMS += $$files($$PROJECT_ROOT/src/GSvar/*.ui, true)
RESOURCES += $$files($$PROJECT_ROOT/src/GSvar/*.qrc, true)

#test sources:
SOURCES += \
    AnalysisDataController_Test.cpp \
    main.cpp
