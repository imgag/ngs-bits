#c++11 and c++14 support
CONFIG += c++11 

#base settings
QT       -= gui
QT       += sql
QT       += xml xmlpatterns
QTPLUGIN += QSQLMYSQL
TEMPLATE = lib
TARGET = cppNGSD
DEFINES += CPPNGSD_LIBRARY

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#copy DLL to bin folder
DESTDIR = ../../bin/

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppXML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

SOURCES += \
    SqlQuery.cpp\
    NGSD.cpp \
    GenLabDB.cpp \
    DBTable.cpp \
    ReportConfiguration.cpp \
    SomaticReportConfiguration.cpp \
    LoginManager.cpp \
    SomaticXmlReportGenerator.cpp \
    SomaticReportSettings.cpp \
    ReportSettings.cpp \
    GermlineReportGenerator.cpp \
    TumorOnlyReportWorker.cpp \
    SomaticReportHelper.cpp \
    SomaticRnaReport.cpp \

HEADERS += \
    SqlQuery.h \
    NGSD.h \
    GenLabDB.h \
    DBTable.h \
    ReportConfiguration.h \
    SomaticReportConfiguration.h \
    LoginManager.h \
    SomaticXmlReportGenerator.h \
    SomaticReportSettings.h \
    ReportSettings.h \
    GermlineReportGenerator.h \
    TumorOnlyReportWorker.h \
    SomaticReportHelper.h \
    SomaticRnaReport.h

RESOURCES += \
    cppNGSD.qrc

DISTFILES +=
