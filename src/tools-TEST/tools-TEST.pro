#-------------------------------------------------
#
# Project created by QtCreator 2013-08-03T23:23:16
#
#-------------------------------------------------

QT       -= gui
QT       += sql

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#include zlib library
LIBS += -lz

#include cppTFW library
INCLUDEPATH += $$PWD/../cppTFW

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

HEADERS += \
    SampleAncestry_Test.h \
    UpdHunter_Test.h \
    NGSDImportQC_Test.h \
    BamHighCoverage_Test.h \
    NGSDExportGenes_Test.h \
    SampleOverview_Test.h \
    BedAnnotateGenes_Test.h \
    BedAnnotateGC_Test.h \
    BedCoverage_Test.h \
    BedLowCoverage_Test.h \
    BedMerge_Test.h \
    BedInfo_Test.h \
    BedExtend_Test.h \
    BedSort_Test.h \
    BedSubtract_Test.h \
    BedShrink_Test.h \
    SampleGender_Test.h \
    FastaInfo_Test.h \
    BedIntersect_Test.h \
    SampleSimilarity_Test.h \
    SampleDiff_Test.h \
    GenesToApproved_Test.h \
    BedAnnotateFreq_Test.h \
    GenesToBed_Test.h \
    VariantAnnotateFrequency_Test.h \
    VariantQC_Test.h \
    MappingQC_Test.h \
    FastqList_Test.h \
    FastqExtract_Test.h \
    FastqFormat_Test.h \
    ReadQC_Test.h \
    BedToFasta_Test.h \
    VariantAnnotateNGSD_Test.h \
    VariantFilterRegions_Test.h \
    FastqMidParser_Test.h \
    FastqTrim_Test.h \
    FastqConvert_Test.h \
    CnvHunter_Test.h \
    BedGeneOverlap_Test.h \
    SeqPurge_Test.h \
    VcfToTsv_Test.h \
    BedChunk_Test.h \
    VcfSort_Test.h \
    NGSDExportSamples_Test.h \
    TsvInfo_Test.h \
    TsvSlice_Test.h \
    TsvFilter_Test.h \
    FastqToFasta_Test.h \
    BamCleanHaloplex_Test.h \
    BedAdd_Test.h \
    NGSDImportHGNC_Test.h \
    NGSDImportEnsembl_Test.h \
    BamDownsample_Test.h \
    BedReadCount_Test.h \
    NGSDImportHPO_Test.h \
    BamClipOverlap_Test.h \
    FastqExtractBarcode_Test.h \
    BamToFastq_Test.h \
    VariantFilterAnnotations_Test.h \
    VcfLeftNormalize_Test.h \
    VcfStreamSort_Test.h \
    NGSDInit_Test.h \
    SomaticQC_Test.h \
    NGSDImportGeneInfo_Test.h \	
    VcfAnnotateFromBed_Test.h \
    NGSDMaintain_Test.h \
    TsvMerge_Test.h \
    BedAnnotateFromBed_Test.h \
    RohHunter_Test.h \
    FastqExtractUMI_Test.h \
    FastqAddBarcode_Test.h \
    BamFilter_Test.h \
    VcfCheck_Test.h \
    VcfBreakMulti_Test.h \
    VcfFilter_Test.h \
    NGSDImportOMIM_Test.h \
    VcfBreakComplexVariants_Test.h
    VcfExtractSamples_Test.h

SOURCES += \
    main.cpp \

include("../app_cli.pri")
