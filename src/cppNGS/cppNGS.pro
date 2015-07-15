#-------------------------------------------------
#
# Project created by QtCreator 2013-07-31T22:18:26
#
#-------------------------------------------------

#base settings
QT       -= gui
QT       += xml xmlpatterns
TEMPLATE = lib
TARGET = cppNGS
DEFINES += CPPNGS_LIBRARY

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

#include cppCORE library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include bamtools library
INCLUDEPATH += $$PWD/../../bamtools/src/
LIBS += -L$$PWD/../../bamtools/lib/ -l bamtools
QMAKE_CXXFLAGS += -Wno-attributes #suppress bamtools warnings

#include zlib library
LIBS += -lz


SOURCES += BedFile.cpp \
	Chromosome.cpp \
	VariantList.cpp \
    VariantFilter.cpp \
    Statistics.cpp \
    ChromosomalFileIndex.cpp \
    Pileup.cpp \
    NGSHelper.cpp \
    FastqFileStream.cpp \
    FastaFileIndex.cpp \
    VariantAnnotationDescription.cpp \
    QCCollection.cpp \
    StatisticsReads.cpp

HEADERS += BedFile.h \
	Chromosome.h \
	VariantList.h \
    VariantFilter.h \
    ChromosomalIndex.h \
    Statistics.h \
    ChromosomalFileIndex.h \
    Pileup.h \
    NGSHelper.h \
    FastqFileStream.h \
    FastaFileIndex.h \
    VariantAnnotationDescription.h \
    QCCollection.h \
    StatisticsReads.h \
    Sequence.h


RESOURCES += \
    resources.qrc
