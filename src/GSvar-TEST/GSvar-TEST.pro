include("../test.pri")

QT       += sql
QT       -= gui
QTPLUGIN += QSQLMYSQL

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include GSvar as library
INCLUDEPATH += $$PWD/../GSvar
LIBS += -L$$PWD/../../bin -lGSvar

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += \
    AnalysisDataController_Test.cpp \
    main.cpp
