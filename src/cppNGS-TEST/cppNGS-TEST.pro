#c++11 support
CONFIG += c++11

#base settings
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app
DESTDIR = ../../bin/

#enable O3 optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE *= -O3

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

#include zlib library
LIBS += -lz

#make the executable search for .so-files in the same folder under linux
QMAKE_LFLAGS += "-Wl,-rpath,\'\$$ORIGIN\'"

HEADERS += \
        Chromosome_Test.h \
        BedLine_Test.h \
        BedFile_Test.h \
        VariantList_Test.h \
        FilterCascade_Test.h \
        ChromosomalIndex_Test.h \
        Statistics_Test.h \
        Variant_Test.h \
        NGSHelper_Test.h \
        FastqFileStream_Test.h \
        FastaFileIndex_Test.h \
        QCCollection_Test.h \
        StatisticsReads_Test.h \
        GeneSet_Test.h \
        OntologyTermCollection_Test.h \
        BamReader_Test.h \
        VcfFile_Test.h \
    CnvList_Test.h \
    StructuralVariantType_Test.h \
    Transcript_Test.h \
    ClinvarSubmissionGenerator_Test.h \
    BedpeLine_Test.h \
    BedpeFile_Test.h \
    Sequence_Test.h \
    VcfFileHandler_Test.h

SOURCES += \
        main.cpp
