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

INCLUDEPATH += $$PWD/../VcfToBedpe


HEADERS += NGSDAddVariantsSomatic_Test.h \
    BamRemoveVariants_Test.h \
    CnvReferenceCohort_Test.h \
    BedpeAnnotateBreakpointDensity_Test.h \
    NGSDExportGff_Test.h \
    FastqCheckUMI.h \
    BedpeExtractGenotype_Test.h \
    BedpeExtractInfoField_Test.h \
    NGSDExportIgvGeneTrack_Test.h \
    NGSDSameSample.h \
    SampleAncestry_Test.h \
    SvFilterAnnotations_Test.h \
    UpdHunter_Test.h \
    NGSDImportQC_Test.h \
    NGSDExportGenes_Test.h \
    BedAnnotateGenes_Test.h \
    BedAnnotateGC_Test.h \
    BedCoverage_Test.h \
    BedHighCoverage_Test.h \
    BedLowCoverage_Test.h \
    BedMerge_Test.h \
    BedInfo_Test.h \
    BedExtend_Test.h \
    BedSort_Test.h \
    BedSubtract_Test.h \
    BedShrink_Test.h \
    SampleGender_Test.h \
    FastaInfo_Test.h \
    FastaMask_Test.h \
    BedIntersect_Test.h \
    SampleSimilarity_Test.h \
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
    VariantFilterRegions_Test.h \
    FastqMidParser_Test.h \
    FastqTrim_Test.h \
    FastqConvert_Test.h \
    BedGeneOverlap_Test.h \
    SeqPurge_Test.h \
    VcfStrip_Test.h \
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
    VcfExtractSamples_Test.h \
    FastqConcat_Test.h \
    VcfToBedpe_Test.h \
    NGSDImportORPHA_Test.h \
    NGSDAddVariantsGermline_Test.h \
    NGSDExportAnnotationData_Test.h \
    VcfAnnotateFromVcf_Test.h \
    NGSDExportCnvTrack_Test.h \
    CnvGeneAnnotation_Test.h \
    BedpeGeneAnnotation_Test.h \
    BedpeAnnotateFromBed_Test.h \
    NGSDAnnotateSV_Test.h \
    NGSDAnnotateCNV_Test.h \
    BedpeToBed_Test.h \
    PhenotypesToGenes_Test.h \
    CnvFilterAnnotations_Test.h \
    PhenotypeSubtree_Test.h \
    BedpeFilter_Test.h \
    BedpeAnnotateCnvOverlap_Test.h \
    TrioMaternalContamination_Test.h \
    FastqDownsample_Test.h \
    VcfCalculatePRS_Test.h \
    VariantAnnotateASE_Test.h \
    SplicingToBed_Test.h \
    GraphStringDb_Test.h \
    GenePrioritization_Test.h \
    CfDnaQC_Test.h \
    VcfAnnotateFromBigWig_Test.h \
    BedLiftOver_Test.h \
    BedpeSort_Test.h \
    NGSDExportSV_Test.h \
    BedpeAnnotateCounts_Test.h \
    VcfAnnotateConsequence_Test.h \
    HgvsToVcf_Test.h\
    RnaQC_Test.h \
    VcfAnnotateHexplorer_Test.h \
    VcfAnnotateMaxEntScan_Test.h \
    NGSDAnnotateRNA_Test.h \
    NGSDAnnotateGeneExpression.h \
    NGSDImportExpressionData_Test.h \
    NGSDExtractRNACohort_Test.h \
    VcfToBed_Test.h \
    TsvToQC_Test.h \
    VcfAdd_Test.h \
    NGSDImportGenlab_Test.h \
    NGSDExportStudyGHGA_Test.h \
    VcfSubtract_Test.h \
    TranscriptsToBed_Test.h \
    GenesToTranscripts_Test.h \
    TranscriptComparison_Test.h \
    NGSDImportSampleQC_Test.h \
    SamplePath_Test.h \
    BamExtract_Test.h \
    VcfSplit_Test.h \
    VcfMerge_Test.h \
    ExtractMethylationData_Test.h \
    TsvDiff_Test.h \
	QcToTsv_Test.h

SOURCES += \
    main.cpp

include("../app_cli.pri")
