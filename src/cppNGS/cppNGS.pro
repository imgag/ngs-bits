#c++11 support
CONFIG += c++11 

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

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include zlib library
LIBS += -lz


SOURCES += BedFile.cpp \
    Chromosome.cpp \
    VariantList.cpp \
    VariantFilter.cpp \
    Statistics.cpp \
    Pileup.cpp \
    NGSHelper.cpp \
    FastqFileStream.cpp \
    FastaFileIndex.cpp \
    VariantAnnotationDescription.cpp \
    QCCollection.cpp \
    StatisticsReads.cpp \
    BamReader.cpp \
    BamWriter.cpp \
    SampleSimilarity.cpp \
    CnvList.cpp \
    Phenotype.cpp \
    Transcript.cpp \
    GeneSet.cpp \
    RohList.cpp \
    OntologyTermCollection.cpp \
    SvList.cpp


HEADERS += BedFile.h \
    Chromosome.h \
    VariantList.h \
    VariantFilter.h \
    ChromosomalIndex.h \
    Statistics.h \
    Pileup.h \
    NGSHelper.h \
    FastqFileStream.h \
    FastaFileIndex.h \
    VariantAnnotationDescription.h \
    QCCollection.h \
    StatisticsReads.h \
    Sequence.h \
    BamReader.h \
    BamWriter.h \
    SampleSimilarity.h \
    CnvList.h \
    Phenotype.h \
    Transcript.h \
    GeneSet.h \
    RohList.h \
    OntologyTermCollection.h \
    SvList.h


RESOURCES += \
    cppNGS.qrc
