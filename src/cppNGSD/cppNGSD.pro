include("../lib.pri")

#base settings
QT       -= gui
QT       += sql xml
QTPLUGIN += QSQLMYSQL
TARGET = cppNGSD
DEFINES += CPPNGSD_LIBRARY

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

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += \
    ApiCaller.cpp \
    ExportCBioPortalStudy.cpp \
    FileLocationList.cpp \
    FileLocationProviderLocal.cpp \
    FileLocationProviderRemote.cpp \
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
    StatisticsServiceLocal.cpp \
    StatisticsServiceRemote.cpp \
    TumorOnlyReportWorker.cpp \
    SomaticReportHelper.cpp \
    SomaticRnaReport.cpp \
    SomaticcfDNAReport.cpp

HEADERS += \
    ApiCaller.h \
    ExportCBioPortalStudy.h \
    FileLocation.h \
    FileLocationList.h \
    FileLocationProvider.h \
    FileLocationProviderLocal.h \
    FileLocationProviderRemote.h \
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
    StatisticsService.h \
    StatisticsServiceLocal.h \
    StatisticsServiceRemote.h \
    TumorOnlyReportWorker.h \
    SomaticReportHelper.h \
    SomaticRnaReport.h \
    SomaticcfDNAReport.h \
    UserPermissionList.h

RESOURCES += \
    cppNGSD.qrc
