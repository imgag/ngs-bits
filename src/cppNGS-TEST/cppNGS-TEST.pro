#-------------------------------------------------
#
# Project created by QtCreator 2013-08-03T23:23:16
#
#-------------------------------------------------

QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#include zlib library
LIBS += -lz

HEADERS += \
        Chromosome_Test.h \
        BedLine_Test.h \
        BedFile_Test.h \
        VariantList_Test.h \
        VariantFilter_Test.h \
        ChromosomalIndex_Test.h \
        Statistics_Test.h \
        Variant_Test.h \
        ChromosomalFileIndex_Test.h \
        NGSHelper_Test.h \
        FastqFileStream_Test.h \
        FastaFileIndex_Test.h \
        QCCollection_Test.h \
        StatisticsReads_Test.h

SOURCES += \
	main.cpp

include("../app_cli.pri")

