#c++11 and c++14 support
CONFIG += c++11 

#base settings
QT       -= gui
QT       += sql
QT       += xml xmlpatterns
QTPLUGIN += QSQLMYSQL
TEMPLATE = lib
TARGET = cppREST
DEFINES += CPPREST_LIBRARY

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

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include cppXML library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include zlib library
LIBS += -lz

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

SOURCES += \
    EndpointManager.cpp \
    FileDbManager.cpp \
    HtmlEngine.cpp \
    HttpUtils.cpp \
    HttpRequest.cpp \
    HttpResponse.cpp \
    RequestParser.cpp \
    RequestWorker.cpp \
    ServerHelper.cpp \
    ServerWrapper.cpp \
    SessionManager.cpp \
    SslServer.cpp \
    UrlBackupWorker.cpp \
    UrlManager.cpp

HEADERS += \
    EndpointManager.h \
    FileDbManager.h \
    HtmlEngine.h \
    HttpParts.h \
    HttpUtils.h \
    HttpRequest.h \
    HttpResponse.h \
    RequestParser.h \
    RequestWorker.h \
    ServerHelper.h \
    ServerWrapper.h \
    Session.h \
    SessionManager.h \
    SslServer.h \
    UrlBackupWorker.h \
    UrlEntity.h \
    UrlManager.h

RESOURCES +=

DISTFILES +=
