include("../test.pri")

#include cppNGSD library
INCLUDEPATH += $$PWD/../cppNGS
LIBS += -L$$PWD/../../bin -lcppNGS

#include cppNGSD library
QT       += sql
INCLUDEPATH += $$PWD/../cppNGSD
LIBS += -L$$PWD/../../bin -lcppNGSD

#include htslib library
INCLUDEPATH += $$PWD/../../htslib/include/
LIBS += -L$$PWD/../../htslib/lib/ -lhts

#include libxml2
win32: INCLUDEPATH += $$PWD/../../libxml2/include/
win32: LIBS += -L$$PWD/../../libxml2/libs/ -lxml2
unix: INCLUDEPATH += $$system(pkg-config --cflags libxml-2.0)
unix: !macx: QMAKE_CXXFLAGS += $$system(pkg-config --cflags libxml-2.0)
unix: LIBS += -lxml2

#special treatment for VcfToBedpe
INCLUDEPATH += $$PWD/../VcfToBedpe

SOURCES += \
    BedToEpigen_Test.cpp \
    NGSDAddVariantsSomatic_Test.cpp \
    BamInfo_Test.cpp \
    BamRemoveVariants_Test.cpp \
    CnvReferenceCohort_Test.cpp \
    BedpeAnnotateBreakpointDensity_Test.cpp \
    NGSDExportGff_Test.cpp \
    FastqCheckUMI_Test.cpp \
    BedpeExtractGenotype_Test.cpp \
    BedpeExtractInfoField_Test.cpp \
    NGSDExportIgvGeneTrack_Test.cpp \
    NGSDSameSample_Test.cpp \
    SampleAncestry_Test.cpp \
    SnifflesVcfFix_Test.cpp \
    SvFilterAnnotations_Test.cpp \
    UpdHunter_Test.cpp \
    NGSDImportQC_Test.cpp \
    NGSDExportGenes_Test.cpp \
    BedAnnotateGenes_Test.cpp \
    BedAnnotateGC_Test.cpp \
    BedCoverage_Test.cpp \
    BedHighCoverage_Test.cpp \
    BedLowCoverage_Test.cpp \
    BedMerge_Test.cpp \
    BedInfo_Test.cpp \
    BedExtend_Test.cpp \
    BedSort_Test.cpp \
    BedSubtract_Test.cpp \
    BedShrink_Test.cpp \
    SampleGender_Test.cpp \
    FastaInfo_Test.cpp \
    FastaMask_Test.cpp \
    BedIntersect_Test.cpp \
    SampleSimilarity_Test.cpp \
    GenesToApproved_Test.cpp \
    BedAnnotateFreq_Test.cpp \
    GenesToBed_Test.cpp \
    VariantAnnotateFrequency_Test.cpp \
    VariantQC_Test.cpp \
    MappingQC_Test.cpp \
    FastqList_Test.cpp \
    FastqExtract_Test.cpp \
    FastqFormat_Test.cpp \
    ReadQC_Test.cpp \
    BedToFasta_Test.cpp \
    VariantFilterRegions_Test.cpp \
    FastqMidParser_Test.cpp \
    FastqTrim_Test.cpp \
    FastqConvert_Test.cpp \
    BedGeneOverlap_Test.cpp \
    SeqPurge_Test.cpp \
    VcfStrip_Test.cpp \
    VcfToTsv_Test.cpp \
    BedChunk_Test.cpp \
    VcfSort_Test.cpp \
    NGSDExportSamples_Test.cpp \
    TsvInfo_Test.cpp \
    TsvSlice_Test.cpp \
    TsvFilter_Test.cpp \
    FastqToFasta_Test.cpp \
    BamCleanHaloplex_Test.cpp \
    BedAdd_Test.cpp \
    NGSDImportHGNC_Test.cpp \
    NGSDImportEnsembl_Test.cpp \
    BamDownsample_Test.cpp \
    BedReadCount_Test.cpp \
    NGSDImportHPO_Test.cpp \
    BamClipOverlap_Test.cpp \
    FastqExtractBarcode_Test.cpp \
    BamToFastq_Test.cpp \
    VariantFilterAnnotations_Test.cpp \
    VcfLeftNormalize_Test.cpp \
    VcfStreamSort_Test.cpp \
    NGSDInit_Test.cpp \
    SomaticQC_Test.cpp \
    NGSDImportGeneInfo_Test.cpp \
    VcfAnnotateFromBed_Test.cpp \
    TsvMerge_Test.cpp \
    BedAnnotateFromBed_Test.cpp \
    RohHunter_Test.cpp \
    FastqExtractUMI_Test.cpp \
    FastqAddBarcode_Test.cpp \
    BamFilter_Test.cpp \
    VcfCheck_Test.cpp \
    VcfBreakMulti_Test.cpp \
    VcfFilter_Test.cpp \
    NGSDImportOMIM_Test.cpp \
    VcfExtractSamples_Test.cpp \
    FastqConcat_Test.cpp \
    VcfToBedpe_Test.cpp \
    NGSDImportORPHA_Test.cpp \
    NGSDAddVariantsGermline_Test.cpp \
    NGSDExportAnnotationData_Test.cpp \
    VcfAnnotateFromVcf_Test.cpp \
    NGSDExportCnvTrack_Test.cpp \
    CnvGeneAnnotation_Test.cpp \
    BedpeGeneAnnotation_Test.cpp \
    BedpeAnnotateFromBed_Test.cpp \
    NGSDAnnotateSV_Test.cpp \
    NGSDAnnotateCNV_Test.cpp \
    BedpeToBed_Test.cpp \
    PhenotypesToGenes_Test.cpp \
    CnvFilterAnnotations_Test.cpp \
    PhenotypeSubtree_Test.cpp \
    BedpeFilter_Test.cpp \
    BedpeAnnotateCnvOverlap_Test.cpp \
    TrioMaternalContamination_Test.cpp \
    FastqDownsample_Test.cpp \
    VcfCalculatePRS_Test.cpp \
    VariantAnnotateASE_Test.cpp \
    SplicingToBed_Test.cpp \
    GraphStringDb_Test.cpp \
    GenePrioritization_Test.cpp \
    CfDnaQC_Test.cpp \
    VcfAnnotateFromBigWig_Test.cpp \
    BedLiftOver_Test.cpp \
    BedpeSort_Test.cpp \
    NGSDExportSV_Test.cpp \
    BedpeAnnotateCounts_Test.cpp \
    VcfAnnotateConsequence_Test.cpp \
    HgvsToVcf_Test.cpp \
    RnaQC_Test.cpp \
    VcfAnnotateHexplorer_Test.cpp \
    VcfAnnotateMaxEntScan_Test.cpp \
    NGSDAnnotateRNA_Test.cpp \
    NGSDAnnotateGeneExpression_Test.cpp \
    NGSDImportExpressionData_Test.cpp \
    NGSDExtractRNACohort_Test.cpp \
    VcfToBed_Test.cpp \
    TsvToQC_Test.cpp \
    TsvTo_Test.cpp \
    VcfAdd_Test.cpp \
    NGSDImportGenlab_Test.cpp \
    NGSDExportStudyGHGA_Test.cpp \
    VcfSubtract_Test.cpp \
    TranscriptsToBed_Test.cpp \
    GenesToTranscripts_Test.cpp \
    TranscriptComparison_Test.cpp \
    NGSDImportSampleQC_Test.cpp \
    SamplePath_Test.cpp \
    BamExtract_Test.cpp \
    VcfSplit_Test.cpp \
    ExtractMethylationData_Test.cpp \
    TsvDiff_Test.cpp \
    QcToTsv_Test.cpp \
    TrioMendelianErrors_Test.cpp \
    MantaVcfFix_Test.cpp \
    VcfReplaceSamples_Test.cpp \
    GenlabInfo_Test.cpp \
    main.cpp
