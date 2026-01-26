include("../lib.pri")

#base settings
QT       -= gui
QT       += xml
TARGET = cppNGS
DEFINES += CPPNGS_LIBRARY

#include cppCORE library
INCLUDEPATH += $$PWD/../cppCORE
LIBS += -L$$PWD/../../bin -lcppCORE

#include cppCORE library
INCLUDEPATH += $$PWD/../cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include libxml2
win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2
unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += BedFile.cpp \
    ArribaFile.cpp \
    Chromosome.cpp \
    ClientHelper.cpp \
    RefGenomeService.cpp \
    RepeatLocusList.cpp \
    VariantImpact.cpp \
    VariantList.cpp \
    Statistics.cpp \
    Pileup.cpp \
    NGSHelper.cpp \
    FastqFileStream.cpp \
    FastaFileIndex.cpp \
    VariantAnnotationDescription.cpp \
    QCCollection.cpp \
    StatisticsReads.cpp \
    Sequence.cpp \
    BamReader.cpp \
    BamWriter.cpp \
    SampleSimilarity.cpp \
    CnvList.cpp \
    Phenotype.cpp \
    Transcript.cpp \
    GeneSet.cpp \
    RohList.cpp \
    OntologyTermCollection.cpp \
    FilterCascade.cpp \
    TabixIndexedFile.cpp \
    BedpeFile.cpp \
    MidCheck.cpp \
    VcfLine.cpp \
    VcfFile.cpp \
    PhenotypeList.cpp \
    VariantScores.cpp \
    SomaticVariantInterpreter.cpp \
    VariantType.cpp \
    SomaticCnvInterpreter.cpp \
    PrsTable.cpp \
    RtfDocument.cpp \
    GenomeBuild.cpp \
    ChainFileReader.cpp \
    BigWigReader.cpp \
    VariantHgvsAnnotator.cpp \
    WorkerAverageCoverage.cpp \
    WorkerLowOrHighCoverage.cpp \
    PipelineSettings.cpp

HEADERS += BedFile.h \
    ArribaFile.h \
    Chromosome.h \
    ClientHelper.h \
    FileInfo.h \
    RefGenomeService.h \
    RepeatLocusList.h \    
    VariantImpact.h \
    VariantList.h \
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
    FilterCascade.h \
    TabixIndexedFile.h \
    BedpeFile.h \
    KeyValuePair.h \
    VariantType.h \
    MidCheck.h \
    VcfLine.h \
    VcfFile.h \
    PhenotypeList.h \
    VariantScores.h \
    SomaticVariantInterpreter.h \
    VariantType.h \
    SomaticCnvInterpreter.h \
    PrsTable.h \
    RtfDocument.h \
    Graph.h \
    GraphNode.h \
    GraphEdge.h \
    GenomeBuild.h \
    ChainFileReader.h \
    BigWigReader.h \
    VariantHgvsAnnotator.h \
    WorkerAverageCoverage.h \
    WorkerLowOrHighCoverage.h \
    PipelineSettings.h

RESOURCES += \
    cppNGS.qrc
