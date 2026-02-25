include("../test.pri")

#include cppXML library
INCLUDEPATH += $$PWD/cppXML
LIBS += -L$$PWD/../../bin -lcppXML

#include cppNGS library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppPLOTS library
INCLUDEPATH += $$PWD/../cppPLOTS
LIBS += -L$$PWD/../../bin -lcppPLOTS

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2

unix: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

SOURCES += \
        Chromosome_Test.cpp \
        BedLine_Test.cpp \
        BedFile_Test.cpp \
        VariantList_Test.cpp \
        FilterCascade_Test.cpp \
        ChromosomalIndex_Test.cpp \
        Statistics_Test.cpp \
        Variant_Test.cpp \
        NGSHelper_Test.cpp \
        FastqFileStream_Test.cpp \
        FastaFileIndex_Test.cpp \
        QCCollection_Test.cpp \
        StatisticsReads_Test.cpp \
        GeneSet_Test.cpp \
        OntologyTermCollection_Test.cpp \
        BamReader_Test.cpp \
        CnvList_Test.cpp \
        StructuralVariantType_Test.cpp \
        Transcript_Test.cpp \
        BedpeLine_Test.cpp \
        BedpeFile_Test.cpp \
        Sequence_Test.cpp \
        VCFLine_Test.cpp \
        VcfFile_Test.cpp \
        VariantScores_Test.cpp \
        BamWriter_Test.cpp \
        SomaticVariantInterpreter_Test.cpp \
        Graph_Test.cpp \
        ChainFileReader_Test.cpp \
        BigWigReader_Test.cpp \
        VariantHgvsAnnotator_Test.cpp \
        TabIndexedFile_Test.cpp \
        PipelineSettings_Test.cpp \
        RepeatLocusList_Test.cpp \
        main.cpp
